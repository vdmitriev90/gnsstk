import datetime
import os
import re

try:
  import tomllib
except ModuleNotFoundError:  # Python < 3.11
  import tomli as tomllib

from gnss_downloader.orchestrator.models import OrchestratorConfig, RoverSet, Session, TimeSpan

ORCHESTRATOR_TABLE = "orchestrator"
TIME_SPANS_TABLE = "time_spans"
ROVER_SETS_TABLE = "rover_sets"
SESSIONS_TABLE = "sessions"
_SAFE_NAME_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]*$")
_RESERVED_NAMES = {
  "CON", "PRN", "AUX", "NUL",
  *(f"COM{i}" for i in range(1, 10)),
  *(f"LPT{i}" for i in range(1, 10)),
}


class ConfigError(ValueError):
  pass


def _require(table, key, table_name, expected_type=str):
  if key not in table:
    raise ConfigError(f"[{table_name}] {key} is required")
  value = table[key]
  if not isinstance(value, expected_type) or isinstance(value, bool) and expected_type is not bool:
    raise ConfigError(f"[{table_name}] {key} must be of type {expected_type.__name__}")
  if isinstance(value, str) and not value.strip():
    raise ConfigError(f"[{table_name}] {key} is required")
  return value.strip() if isinstance(value, str) else value


def _require_file(path, key):
  if not os.path.isfile(path):
    raise ConfigError(f"{key} file not found: {path}")
  return path


def _resolve_path(base_dir, value):
  path = value if os.path.isabs(value) else os.path.join(base_dir, value)
  return os.path.normpath(path)


def _utc(table, key, table_name):
  value = _require(table, key, table_name, datetime.datetime)
  if value.tzinfo is None:
    raise ConfigError(f"[{table_name}] {key} must include a timezone offset: '{value}'")
  return value.astimezone(datetime.timezone.utc)


def _validate_project_name(name):
  if (not _SAFE_NAME_RE.match(name) or name.endswith(".")
      or name.split(".")[0].upper() in _RESERVED_NAMES):
    raise ConfigError(f"project_name is not a safe directory name: '{name}'")
  return name


def _parse_time_spans(root):
  spans = root.get(TIME_SPANS_TABLE)
  if not isinstance(spans, dict) or not spans:
    raise ConfigError(f"No [{TIME_SPANS_TABLE}.*] tables defined")
  result = {}
  for name, table in spans.items():
    table_name = f"{TIME_SPANS_TABLE}.{name}"
    if not isinstance(table, dict):
      raise ConfigError(f"[{table_name}] must be a table")
    start = _utc(table, "start", table_name)
    end = _utc(table, "end", table_name)
    try:
      result[name] = TimeSpan(start=start, end=end)
    except ValueError as e:
      raise ConfigError(f"[{table_name}] {e}") from None
  return result


def _make_rover_set(raw_rovers, base, table_name):
  if not all(isinstance(r, str) and r.strip() for r in raw_rovers):
    raise ConfigError(f"[{table_name}] rovers must be an array of non-empty strings")
  if base is not None:
    if not isinstance(base, str):
      raise ConfigError(f"[{table_name}] base must be a string")
    base = base.strip().upper() or None
  try:
    return RoverSet(sites=tuple(r.strip().upper() for r in raw_rovers), base=base)
  except ValueError as e:
    raise ConfigError(f"[{table_name}] {e}") from None


def _parse_rover_sets(root):
  sets = root.get(ROVER_SETS_TABLE)
  if not isinstance(sets, dict) or not sets:
    raise ConfigError(f"No [{ROVER_SETS_TABLE}.*] tables defined")
  result = {}
  for name, table in sets.items():
    table_name = f"{ROVER_SETS_TABLE}.{name}"
    if not isinstance(table, dict):
      raise ConfigError(f"[{table_name}] must be a table")
    raw_rovers = _require(table, "rovers", table_name, list)
    result[name] = _make_rover_set(raw_rovers, table.get("base"), table_name)
  return result


def _build_sessions(root):
  spans = _parse_time_spans(root)
  rover_sets = _parse_rover_sets(root)

  sessions_table = root.get(SESSIONS_TABLE)
  if not isinstance(sessions_table, dict) or not sessions_table:
    raise ConfigError(f"No [{SESSIONS_TABLE}.*] tables defined")

  sessions = []
  for name, table in sessions_table.items():
    table_name = f"{SESSIONS_TABLE}.{name}"
    if not isinstance(table, dict):
      raise ConfigError(f"[{table_name}] must be a table")

    span_name = _require(table, "time_span", table_name)
    if span_name not in spans:
      raise ConfigError(f"[{table_name}] unknown time_span '{span_name}'")

    if "rovers" not in table:
      raise ConfigError(f"[{table_name}] rovers is required")
    rovers = table["rovers"]
    if isinstance(rovers, str):
      if rovers not in rover_sets:
        raise ConfigError(f"[{table_name}] unknown rover set '{rovers}'")
      rover_set = rover_sets[rovers]
    elif isinstance(rovers, list):
      rover_set = _make_rover_set(rovers, table.get("base"), table_name)
    else:
      raise ConfigError(f"[{table_name}] rovers must be a rover set name or an array of strings")

    sessions.append(Session(name=name, time_span=spans[span_name],
                            rover_set=rover_set, base=rover_set.base))
  return sessions


def load_orchestrator_config(path):
  _require_file(path, "Configuration")

  try:
    with open(path, "rb") as f:
      root = tomllib.load(f)
  except tomllib.TOMLDecodeError as e:
    raise ConfigError(f"Cannot parse configuration: {e}") from None

  table = root.get(ORCHESTRATOR_TABLE)
  if not isinstance(table, dict):
    raise ConfigError(f"Missing [{ORCHESTRATOR_TABLE}] table")

  working_directory = _require(table, "working_directory", ORCHESTRATOR_TABLE)
  if not os.path.isabs(working_directory):
    working_directory = os.path.join(os.path.dirname(os.path.abspath(path)), working_directory)
  working_directory = os.path.normpath(working_directory)
  if os.path.exists(working_directory) and not os.path.isdir(working_directory):
    raise ConfigError(f"working_directory is not a directory: {working_directory}")

  processor_executable = _require_file(
    _resolve_path(working_directory, _require(table, "processor_executable", ORCHESTRATOR_TABLE)),
    "processor_executable")
  processor_config = _require_file(
    _resolve_path(working_directory, _require(table, "processor_config", ORCHESTRATOR_TABLE)),
    "processor_config")
  project_name = _validate_project_name(_require(table, "project_name", ORCHESTRATOR_TABLE))

  sessions = _build_sessions(root)

  return OrchestratorConfig(
    processor_executable=processor_executable,
    processor_ini=processor_config,
    project_name=project_name,
    working_directory=working_directory,
    sessions=tuple(sessions),
  )
