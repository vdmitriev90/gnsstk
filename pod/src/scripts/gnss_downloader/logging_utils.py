import datetime
import os


def log(msg):
  print(f"[INFO] {msg}")

_log_file_path = None


def set_log_file(output_dir):
  global _log_file_path
  os.makedirs(output_dir, exist_ok=True)
  stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
  _log_file_path = os.path.join(output_dir, f"download_{stamp}.log")
  suffix = 1
  while os.path.exists(_log_file_path):
    _log_file_path = os.path.join(output_dir, f"download_{stamp}_{suffix}.log")
    suffix += 1
  return _log_file_path


def get_log_file():
  return _log_file_path


def _write_file(line):
  if _log_file_path:
    try:
      with open(_log_file_path, "a", encoding="utf-8") as f:
        f.write(f"{datetime.datetime.now():%Y-%m-%d %H:%M:%S} {line}\n")
    except OSError:
      pass


def log_file(msg):
  _write_file(f"[INFO] {msg}")


def error(msg):
  line = f"[ERROR] {msg}"
  print(line)
  _write_file(line)

def relpath(path):
  try:
    return os.path.relpath(path)
  except ValueError:
    return path
