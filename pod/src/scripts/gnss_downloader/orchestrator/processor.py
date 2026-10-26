import os
import re
import shutil
import subprocess
import sys

from gnss_downloader.logging_utils import log, log_file


class ProcessorError(RuntimeError):
  pass


def _set_key(lines, key, value):
  pattern = re.compile(rf"^(\s*{re.escape(key)}\s*=)[^\r\n]*?(\s*(#.*)?)$", re.IGNORECASE)
  for i, line in enumerate(lines):
    stripped = line.rstrip("\r\n")
    if stripped.lstrip().startswith("#"):
      continue
    m = pattern.match(stripped)
    if m:
      newline = line[len(stripped):]
      lines[i] = f"{m.group(1)} {value}{newline}"
      return
  raise ProcessorError(f"Key '{key}' not found in processor INI")


def copy_processor_template(template_path, layout):
  os.makedirs(layout.root, exist_ok=True)
  copy_path = os.path.join(layout.root, os.path.basename(template_path))
  if os.path.normcase(os.path.abspath(copy_path)) != os.path.normcase(os.path.abspath(template_path)):
    shutil.copyfile(template_path, copy_path)
  return copy_path


def write_job_ini(template_copy, out_path, rover, base):
  with open(template_copy, encoding="utf-8", newline="") as f:
    lines = f.readlines()
  _set_key(lines, "SiteRover", rover)
  _set_key(lines, "SiteBase", base or "")
  with open(out_path, "w", encoding="utf-8", newline="") as f:
    f.writelines(lines)
  return out_path


def run_processor(executable, ini_path, cwd, log_path):
  log(f"Running processor: {executable} {ini_path}")
  log_file(f"Running processor: {executable} {ini_path} (cwd={cwd})")
  with open(log_path, "w", encoding="utf-8") as out:
    proc = subprocess.Popen(
      [executable, ini_path], cwd=cwd, stdin=subprocess.DEVNULL,
      stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    for raw in iter(proc.stdout.readline, b""):
      text = raw.decode("utf-8", errors="replace")
      sys.stdout.write(text)
      sys.stdout.flush()
      out.write(text)
      out.flush()
    proc.stdout.close()
    returncode = proc.wait()
  log_file(f"Processor exit code: {returncode} (output: {log_path})")
  if returncode != 0:
    raise ProcessorError(
      f"Processor failed with exit code {returncode} for {ini_path} (see {log_path})")
