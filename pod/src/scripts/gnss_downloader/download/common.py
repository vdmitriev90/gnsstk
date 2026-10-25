import os
import time
import requests
import gzip
import shutil
import subprocess

from unlzw3 import unlzw

from gnss_downloader.logging_utils import log, error, relpath, log_file

TIMEOUT = 10
RETRIES = 3


class DownloadError(RuntimeError):
  pass


def fail(msg):
  error(msg)
  raise DownloadError(msg)


def _is_hatanaka(path):
  if path.endswith(".crx"):
    return True

  ext = os.path.splitext(path)[1]  # e.g. ".24d"

  return len(ext) == 4 and ext[1:3].isdigit() and ext.endswith("d")


def crx_to_rnx(crx_path):
  log(f"Converting CRX to RNX: {relpath(crx_path)}")

  subprocess.run(["crx2rnx.exe", crx_path], check=True)

  if crx_path.endswith(".crx"):
    rnx_path = crx_path[:-4] + ".rnx"
  else:
    # .{yy}d -> .{yy}o
    rnx_path = crx_path[:-1] + "o"

  os.remove(crx_path)

  return rnx_path


def clean_directory(path):
  if not os.path.exists(path):
    return

  log(f"Cleaning directory: {relpath(path)}")

  for filename in os.listdir(path):
    file_path = os.path.join(path, filename)

    try:
      if os.path.isfile(file_path):
        os.remove(file_path)
      elif os.path.isdir(file_path):
        shutil.rmtree(file_path)
    except Exception as e:
      error(f"Failed to delete {relpath(file_path)}: {e}")


def unzip_gz(gz_path):
  out_path = gz_path[:-3]  # убираем .gz

  with gzip.open(gz_path, 'rb') as f_in:
    with open(out_path, 'wb') as f_out:
      shutil.copyfileobj(f_in, f_out)

  os.remove(gz_path)

  log(f"Saved: {relpath(out_path)}")

  return out_path

def unzip_Z_file(path):
  with open(path, 'rb') as f:
    data = unlzw(f.read())

  out_path = path[:-2]  # убрать .Z

  with open(out_path, 'wb') as f:
    f.write(data)

  os.remove(path)

  log(f"Saved: {relpath(out_path)}")

  return out_path


def unzip_file(path):
  unzip_path = None
  if path.endswith(".gz"):
    unzip_path = unzip_gz(path)
  elif path.endswith(".Z"):
    unzip_path = unzip_Z_file(path)
  else:
    raise RuntimeError(f"Unknown compression: {path}")

  if _is_hatanaka(unzip_path):
    unzip_path = crx_to_rnx(unzip_path)

  return unzip_path


def try_download(url, output_path):
  partial_path = output_path + ".partial"
  for attempt in range(RETRIES):
    try:
      if attempt > 0:
        log(f"Retrying {attempt + 1}/{RETRIES}: {url}")

      r = requests.get(url, stream=True, timeout=TIMEOUT)

      if r.status_code == 404:
        log(f"HTTP status: {r.status_code} (not found)")
        return False  # fallback trigger

      if not r.ok:
        log(f"HTTP status: {r.status_code}")

      r.raise_for_status()

      with open(partial_path, "wb") as f:
        for chunk in r.iter_content(chunk_size=8192):
          if chunk:
            f.write(chunk)

      os.replace(partial_path, output_path)
      return True

    except requests.exceptions.RequestException as e:
      error(f"Download failed: {e}")
      if os.path.exists(partial_path):
        os.remove(partial_path)
      time.sleep(2)

  return False


def download_and_extract(url, path):
  if not try_download(url, path):
    return False

  unzip_path = unzip_file(path)
  log_file(f"Downloaded: {os.path.basename(unzip_path)} ({url})")
  return True
