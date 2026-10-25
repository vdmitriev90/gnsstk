import datetime
import os


def log(msg):
  print(f"[INFO] {msg}")

LOG_FILE_NAME = "download.log"
_log_file_path = None


def set_log_file(output_dir):
  global _log_file_path
  os.makedirs(output_dir, exist_ok=True)
  _log_file_path = os.path.join(output_dir, LOG_FILE_NAME)


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
