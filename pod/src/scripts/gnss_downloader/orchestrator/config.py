import configparser
import datetime
import os
import re

from gnss_downloader.orchestrator.models import OrchestratorConfig, Session

DEFAULT_SECTION = "default"
_SAFE_NAME_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]*$")
_RESERVED_NAMES = {
  "CON", "PRN", "AUX", "NUL",
  *(f"COM{i}" for i in range(1, 10)),
  *(f"LPT{i}" for i in range(1, 10)),
}


class ConfigError(ValueError):
  pass


class _StrictConfigParser(configparser.ConfigParser):
  def optionxform(self, optionstr):
    return optionstr


def _require(section, key, section_name):
  value = section.get(key, "").strip()
  if not value:
    raise ConfigError(f"[{section_name}] {key} is required")
  return value


def _require_file(path, key):
  if not os.path.isfile(path):
    raise ConfigError(f"{key} file not found: {path}")
  return path


def _resolve_path(base_dir, value):
  path = value if os.path.isabs(value) else os.path.join(base_dir, value)
  return os.path.normpath(path)


def _parse_utc(value, key, section_name):
  text = value.strip()
  if text.endswith(("Z", "z")):
    text = text[:-1] + "+00:00"
  try:
    dt = datetime.datetime.fromisoformat(text)
  except ValueError as e:
    raise ConfigError(f"[{section_name}] invalid {key} '{value}': {e}") from None
  if dt.tzinfo is None:
    raise ConfigError(f"[{section_name}] {key} must include a timezone: '{value}'")
  return dt.astimezone(datetime.timezone.utc)


def _validate_project_name(name):
  if (not _SAFE_NAME_RE.match(name) or name.endswith(".")
      or name.split(".")[0].upper() in _RESERVED_NAMES):
    raise ConfigError(f"ProjectName is not a safe directory name: '{name}'")
  return name


def _parse_session(section_name, section):
  start = _parse_utc(_require(section, "SessionStart", section_name), "SessionStart", section_name)
  end = _parse_utc(_require(section, "SessionEnd", section_name), "SessionEnd", section_name)
  if start >= end:
    raise ConfigError(f"[{section_name}] SessionStart must be earlier than SessionEnd")

  rovers = tuple(r.upper() for r in section.get("Rovers", "").split())
  if not rovers:
    raise ConfigError(f"[{section_name}] at least one rover is required")
  duplicates = sorted({r for r in rovers if rovers.count(r) > 1})
  if duplicates:
    raise ConfigError(f"[{section_name}] duplicate rovers: {', '.join(duplicates)}")

  base = section.get("Base", "").strip().upper() or None
  if base is not None and base in rovers:
    raise ConfigError(f"[{section_name}] Base {base} must differ from every rover")

  return Session(name=section_name, start=start, end=end, rovers=rovers, base=base)


def load_orchestrator_config(path):
  _require_file(path, "Configuration")

  parser = _StrictConfigParser(default_section="__none__", interpolation=None, strict=True)
  try:
    with open(path, encoding="utf-8") as f:
      parser.read_file(f)
  except (configparser.DuplicateSectionError, configparser.DuplicateOptionError) as e:
    raise ConfigError(f"Invalid configuration: {e}") from None
  except configparser.Error as e:
    raise ConfigError(f"Cannot parse configuration: {e}") from None

  if not parser.has_section(DEFAULT_SECTION):
    raise ConfigError(f"Missing [{DEFAULT_SECTION}] section")

  defaults = parser[DEFAULT_SECTION]
  working_directory = _require(defaults, "WorkingDirectory", DEFAULT_SECTION)
  if not os.path.isabs(working_directory):
    working_directory = os.path.join(os.path.dirname(os.path.abspath(path)), working_directory)
  working_directory = os.path.normpath(working_directory)
  if os.path.exists(working_directory) and not os.path.isdir(working_directory):
    raise ConfigError(f"WorkingDirectory is not a directory: {working_directory}")

  processor_executable = _require_file(
    _resolve_path(working_directory, _require(defaults, "ProcessorExecutable", DEFAULT_SECTION)),
    "ProcessorExecutable")
  processor_ini = _require_file(
    _resolve_path(working_directory, _require(defaults, "ProcessorIni", DEFAULT_SECTION)),
    "ProcessorIni")
  project_name = _validate_project_name(_require(defaults, "ProjectName", DEFAULT_SECTION))

  sessions = []
  seen = set()
  for name in parser.sections():
    if name == DEFAULT_SECTION:
      continue
    key = name.lower()
    if key in seen:
      raise ConfigError(f"Duplicate session section: [{name}]")
    seen.add(key)
    sessions.append(_parse_session(name, parser[name]))

  if not sessions:
    raise ConfigError("No session sections defined")

  return OrchestratorConfig(
    processor_executable=processor_executable,
    processor_ini=processor_ini,
    project_name=project_name,
    working_directory=working_directory,
    sessions=tuple(sessions),
  )
