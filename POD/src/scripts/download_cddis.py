import datetime
import os
import sys
import time
import argparse
import requests
import gzip
import shutil
import subprocess
from unlzw3 import unlzw

# ===================== DEBUG SUPPORT =====================

def setup_debug():
  import debugpy
  debugpy.listen(("localhost", 5678))
  print("[DEBUG] waiting for debugger on port 5678...")
  debugpy.wait_for_client()
  print("[DEBUG] debugger attached")

def log(msg):
  print(f"[INFO] {msg}")

def error(msg):
  print(f"[ERROR] {msg}")

def relpath(path):
  try:
    return os.path.relpath(path)
  except ValueError:
    return path

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


# === DEFAULT CONFIG ===
DEFAULT_OUTPUT_DIR = "./nav_files"
TIMEOUT = 10
RETRIES = 3

# ===================== DOWNLOAD =====================

def try_download(url, output_path):
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

      with open(output_path, "wb") as f:
        for chunk in r.iter_content(chunk_size=8192):
          if chunk:
            f.write(chunk)

      return True

    except requests.exceptions.RequestException as e:
      error(f"Download failed: {e}")
      time.sleep(2)

  return False


def download_and_extract(url, path):
  if not try_download(url, path):
    return False

  unzip_file(path)
  return True


def download_ionex(date, output_dir):
  year = date.year
  doy = date.timetuple().tm_yday
  yy = str(year)[-2:]

  yyyy = f"{year:04d}"
  ddd = f"{doy:03d}"

  os.makedirs(output_dir, exist_ok=True)

  # ===================== TRY NEW FORMAT =====================
  filename = f"COD0OPSFIN_{yyyy}{ddd}0000_01D_01H_GIM.INX.gz"
  extracted = os.path.join(output_dir, filename[:-3])
  if os.path.exists(extracted):
    log(f"IONEX already exists, skipping: {relpath(extracted)}")
    return

  # Check if legacy fallback already exists
  legacy_filename = f"codg{ddd}0.{yy}i"
  legacy_extracted = os.path.join(output_dir, legacy_filename)
  if os.path.exists(legacy_extracted):
    log(f"IONEX (legacy) already exists, skipping: {relpath(legacy_extracted)}")
    return

  url = f"https://cddis.nasa.gov/archive/gnss/products/ionex/{yyyy}/{ddd}/{filename}"
  path = os.path.join(output_dir, filename)

  log(f"Trying IONEX: {url}")

  if download_and_extract(url, path):
    return

  # ===================== FALLBACK TO LEGACY FORMAT =====================
  legacy_compressed = f"codg{ddd}0.{yy}i.Z"
  url = f"https://cddis.nasa.gov/archive/gnss/products/ionex/{yyyy}/{ddd}/{legacy_compressed}"
  path = os.path.join(output_dir, legacy_compressed)

  log(f"IONEX not found, trying legacy fallback: {url}")

  if download_and_extract(url, path):
    return

  error(f"IONEX download failed for {yyyy}-{ddd}")


def download_clk(date, output_dir):
    gps_week = int((date - datetime.date(1980, 1, 6)).days / 7)
    dow = (date - datetime.date(1980, 1, 6)).days % 7

    year = date.year
    doy = date.timetuple().tm_yday
    yyyy = f"{year:04d}"
    ddd = f"{doy:03d}"

    os.makedirs(output_dir, exist_ok=True)

    # ===================== TRY COD MGXFIN =====================
    cod_filename = f"COD0MGXFIN_{yyyy}{ddd}0000_01D_30S_CLK.CLK.gz"
    cod_extracted = os.path.join(output_dir, cod_filename[:-3])

    if os.path.exists(cod_extracted):
        log(f"CLK (COD) already exists, skipping: {relpath(cod_extracted)}")
        return

    # fallback already exists
    legacy_filename = f"cod{gps_week}{dow}.clk_05s"
    legacy_extracted = os.path.join(output_dir, legacy_filename)

    if os.path.exists(legacy_extracted):
        log(f"CLK (legacy) already exists, skipping: {relpath(legacy_extracted)}")
        return

    url = f"https://cddis.nasa.gov/archive/gnss/products/{gps_week}/{cod_filename}"
    path = os.path.join(output_dir, cod_filename)

    log(f"Trying CLK (COD): {url}")

    if download_and_extract(url, path):
        return

    # ===================== FALLBACK TO LEGACY =====================
    legacy_compressed = f"cod{gps_week}{dow}.clk_05s.Z"
    url = f"https://cddis.nasa.gov/archive/gnss/products/{gps_week}/{legacy_compressed}"
    path = os.path.join(output_dir, legacy_compressed)

    log(f"CLK (COD) not found, trying legacy fallback: {url}")

    if download_and_extract(url, path):
        return

    error(f"CLK download failed for {yyyy}-{ddd}")


def download_sp3(date, output_dir):
    gps_week = int((date - datetime.date(1980, 1, 6)).days / 7)
    dow = (date - datetime.date(1980, 1, 6)).days % 7

    year = date.year
    doy = date.timetuple().tm_yday
    yyyy = f"{year:04d}"
    ddd = f"{doy:03d}"

    os.makedirs(output_dir, exist_ok=True)

    # ===================== TRY COD MGXFIN =====================
    cod_filename = f"COD0MGXFIN_{yyyy}{ddd}0000_01D_05M_ORB.SP3.gz"
    cod_extracted = os.path.join(output_dir, cod_filename[:-3])

    if os.path.exists(cod_extracted):
        log(f"SP3 (COD) already exists, skipping: {relpath(cod_extracted)}")
        return

    # fallback already exists
    legacy_filename = f"cod{gps_week}{dow}.eph"
    legacy_extracted = os.path.join(output_dir, legacy_filename)

    if os.path.exists(legacy_extracted):
        log(f"SP3 (legacy) already exists, skipping: {relpath(legacy_extracted)}")
        return

    url = f"https://cddis.nasa.gov/archive/gnss/products/{gps_week}/{cod_filename}"
    path = os.path.join(output_dir, cod_filename)

    log(f"Trying SP3 (COD): {url}")

    if download_and_extract(url, path):
        return

    # ===================== FALLBACK TO LEGACY =====================
    legacy_compressed = f"cod{gps_week}{dow}.eph.Z"
    url = f"https://cddis.nasa.gov/archive/gnss/products/{gps_week}/{legacy_compressed}"
    path = os.path.join(output_dir, legacy_compressed)

    log(f"SP3 (COD) not found, trying legacy fallback: {url}")

    if download_and_extract(url, path):
        return

    error(f"SP3 download failed for {yyyy}-{ddd}")


def download_erp(date, output_dir):
    gps_week = int((date - datetime.date(1980, 1, 6)).days / 7)
    dow = (date - datetime.date(1980, 1, 6)).days % 7

    year = date.year
    doy = date.timetuple().tm_yday
    yyyy = f"{year:04d}"
    ddd = f"{doy:03d}"

    os.makedirs(output_dir, exist_ok=True)

    # ===================== TRY COD MGXFIN ERP =====================
    cod_filename = f"COD0MGXFIN_{yyyy}{ddd}0000_03D_12H_ERP.ERP.gz"
    cod_extracted = os.path.join(output_dir, cod_filename[:-3])

    if os.path.exists(cod_extracted):
        log(f"ERP (COD) already exists, skipping: {relpath(cod_extracted)}")
        return

    url = f"https://cddis.nasa.gov/archive/gnss/products/{gps_week}/{cod_filename}"
    path = os.path.join(output_dir, cod_filename)

    log(f"Trying ERP (COD): {url}")

    if download_and_extract(url, path):
        return

    error(f"ERP download failed for {yyyy}-{ddd}")

def download_osb(date, output_dir):
    gps_week = int((date - datetime.date(1980, 1, 6)).days / 7)
    dow = (date - datetime.date(1980, 1, 6)).days % 7

    year = date.year
    doy = date.timetuple().tm_yday
    yyyy = f"{year:04d}"
    ddd = f"{doy:03d}"

    os.makedirs(output_dir, exist_ok=True)

    # ===================== COD0OPSFIN =====================
    cod_filename = f"COD0OPSFIN_{yyyy}{ddd}0000_01D_01D_OSB.BIA.gz"
    cod_extracted = os.path.join(output_dir, cod_filename[:-3])

    if os.path.exists(cod_extracted):
        log(f"OSB (COD0OPSFIN) already exists, skipping: {relpath(cod_extracted)}")
        return

    url = f"https://cddis.nasa.gov/archive/gnss/products/{gps_week}/{cod_filename}"
    path = os.path.join(output_dir, cod_filename)

    log(f"Trying OSB (COD0OPSFIN): {url}")

    if download_and_extract(url, path):
        return

    # ===================== COD0MGXFIN fallback =====================
    legacy_filename = f"COD0MGXFIN_{yyyy}{ddd}0000_01D_01D_OSB.BIA.gz"
    legacy_extracted = os.path.join(output_dir, legacy_filename[:-3])

    if os.path.exists(legacy_extracted):
        log(f"OSB (COD0MGXFIN) already exists, skipping: {relpath(legacy_extracted)}")
        return

    legacy_url = f"https://cddis.nasa.gov/archive/gnss/products/{gps_week}/{legacy_filename}"
    legacy_path = os.path.join(output_dir, legacy_filename)

    log(f"OSB not found, trying fallback: {legacy_url}")

    if not download_and_extract(legacy_url, legacy_path):
        error(f"OSB download failed for {yyyy}-{ddd}")
def download_brdc_gps(date, output_dir):
  year = date.year
  doy = date.timetuple().tm_yday
  yy = str(year)[-2:]

  yyyy = f"{year:04d}"
  ddd = f"{doy:03d}"

  os.makedirs(output_dir, exist_ok=True)

  filename = f"brdc{ddd}0.{yy}n.gz"
  extracted = os.path.join(output_dir, filename[:-3])

  if os.path.exists(extracted):
    log(f"BRDC nav already exists, skipping: {relpath(extracted)}")
    return

  url = f"https://cddis.nasa.gov/archive/gnss/data/daily/{yyyy}/{ddd}/{yy}n/{filename}"
  path = os.path.join(output_dir, filename)

  log(f"Trying BRDC nav: {url}")

  if download_and_extract(url, path):
    return

  # ===================== FALLBACK TO LEGACY .Z FORMAT =====================
  legacy_filename = f"brdc{ddd}0.{yy}n.Z"
  legacy_extracted = os.path.join(output_dir, legacy_filename[:-2])
  if os.path.exists(legacy_extracted):
    log(f"BRDC nav (legacy) already exists, skipping: {relpath(legacy_extracted)}")
    return

  legacy_url = f"https://cddis.nasa.gov/archive/gnss/data/daily/{yyyy}/{ddd}/{yy}n/{legacy_filename}"
  legacy_path = os.path.join(output_dir, legacy_filename)

  log(f"BRDC nav not found, trying legacy fallback: {legacy_url}")

  if not download_and_extract(legacy_url, legacy_path):
    error(f"BRDC nav download failed for {yyyy}-{ddd}")


def download_brdc_glo(date, output_dir):
  year = date.year
  doy = date.timetuple().tm_yday
  yy = str(year)[-2:]

  yyyy = f"{year:04d}"
  ddd = f"{doy:03d}"

  os.makedirs(output_dir, exist_ok=True)

  filename = f"brdc{ddd}0.{yy}g.gz"
  extracted = os.path.join(output_dir, filename[:-3])

  if os.path.exists(extracted):
    log(f"BRDC GLO nav already exists, skipping: {relpath(extracted)}")
    return

  url = f"https://cddis.nasa.gov/archive/gnss/data/daily/{yyyy}/{ddd}/{yy}g/{filename}"
  path = os.path.join(output_dir, filename)

  log(f"Trying BRDC GLO nav: {url}")

  if download_and_extract(url, path):
    return

  # ===================== FALLBACK TO LEGACY .Z FORMAT =====================
  legacy_filename = f"brdc{ddd}0.{yy}g.Z"
  legacy_extracted = os.path.join(output_dir, legacy_filename[:-2])
  if os.path.exists(legacy_extracted):
    log(f"BRDC GLO nav (legacy) already exists, skipping: {relpath(legacy_extracted)}")
    return

  legacy_url = f"https://cddis.nasa.gov/archive/gnss/data/daily/{yyyy}/{ddd}/{yy}g/{legacy_filename}"
  legacy_path = os.path.join(output_dir, legacy_filename)

  log(f"BRDC GLO nav not found, trying legacy fallback: {legacy_url}")

  if not download_and_extract(legacy_url, legacy_path):
    error(f"BRDC GLO nav download failed for {yyyy}-{ddd}")


def download_brdc_glo(date, output_dir):
  year = date.year
  doy = date.timetuple().tm_yday
  yy = str(year)[-2:]

  yyyy = f"{year:04d}"
  ddd = f"{doy:03d}"

  os.makedirs(output_dir, exist_ok=True)

  filename = f"brdc{ddd}0.{yy}g.gz"
  extracted = os.path.join(output_dir, filename[:-3])

  if os.path.exists(extracted):
    log(f"BRDC GLO nav already exists, skipping: {relpath(extracted)}")
    return

  url = f"https://cddis.nasa.gov/archive/gnss/data/daily/{yyyy}/{ddd}/{yy}g/{filename}"
  path = os.path.join(output_dir, filename)

  log(f"Trying BRDC GLO nav: {url}")

  if download_and_extract(url, path):
    return

  # ===================== FALLBACK TO LEGACY .Z FORMAT =====================
  legacy_filename = f"brdc{ddd}0.{yy}g.Z"
  legacy_extracted = os.path.join(output_dir, legacy_filename[:-2])
  if os.path.exists(legacy_extracted):
    log(f"BRDC GLO nav (legacy) already exists, skipping: {relpath(legacy_extracted)}")
    return

  legacy_url = f"https://cddis.nasa.gov/archive/gnss/data/daily/{yyyy}/{ddd}/{yy}g/{legacy_filename}"
  legacy_path = os.path.join(output_dir, legacy_filename)

  log(f"BRDC GLO nav not found, trying legacy fallback: {legacy_url}")

  if not download_and_extract(legacy_url, legacy_path):
    error(f"BRDC GLO nav download failed for {yyyy}-{ddd}")


def download_brdm(date, output_dir):
  year = date.year
  doy = date.timetuple().tm_yday
  yy = str(year)[-2:]

  yyyy = f"{year:04d}"
  ddd = f"{doy:03d}"

  os.makedirs(output_dir, exist_ok=True)

  # ===================== TRY BRDM =====================
  filename = f"BRDM00DLR_S_{yyyy}{ddd}0000_01D_MN.rnx.gz"
  extracted = os.path.join(output_dir, filename[:-3])
  if os.path.exists(extracted):
    log(f"BRDM already exists, skipping: {relpath(extracted)}")
    return

  url = f"https://cddis.nasa.gov/archive/gnss/data/daily/{yyyy}/{ddd}/{yy}p/{filename}"
  output_path = os.path.join(output_dir, filename)

  log(f"Trying BRDM: {url}")

  if download_and_extract(url, output_path):
    return

  # ===================== FALLBACK TO BRDC =====================
  filename = f"BRDC00IGS_R_{yyyy}{ddd}0000_01D_MN.rnx.gz"
  url = f"https://cddis.nasa.gov/archive/gnss/data/daily/{yyyy}/{ddd}/{yy}p/{filename}"
  output_path = os.path.join(output_dir, filename)

  log("BRDM not found, trying BRDC fallback...")

  if download_and_extract(url, output_path):
    return

  raise RuntimeError("Both BRDM and BRDC download failed")


def get_date_str(d):
  return d.strftime("%Y-%m-%d")


def get_window_dates(date):
  return [
    date - datetime.timedelta(days=1),
    date,
    date + datetime.timedelta(days=1),
  ]


def date_range(start_date, end_date):
  current = start_date
  while current <= end_date:
    yield current
    current += datetime.timedelta(days=1)


def _expected_nav_filenames(start_date, end_date):
  names = set()
  for d in date_range(start_date, end_date):
    yyyy = f"{d.year:04d}"
    ddd = f"{d.timetuple().tm_yday:03d}"
    yy = str(d.year)[-2:]
    names.add(f"BRDM00DLR_S_{yyyy}{ddd}0000_01D_MN.rnx")
    names.add(f"BRDC00IGS_R_{yyyy}{ddd}0000_01D_MN.rnx")
    names.add(f"brdc{ddd}0.{yy}n")
    names.add(f"brdc{ddd}0.{yy}g")
  return names


def _expected_sp3_filenames(start_date, end_date):
    names = set()

    for d in date_range(start_date, end_date):
        gps_week = int((d - datetime.date(1980, 1, 6)).days / 7)
        dow = (d - datetime.date(1980, 1, 6)).days % 7
        yyyy = f"{d.year:04d}"
        ddd = f"{d.timetuple().tm_yday:03d}"

        names.add(f"COD0MGXFIN_{yyyy}{ddd}0000_01D_05M_ORB.SP3")
        names.add(f"cod{gps_week}{dow}.eph")

    return names




def _expected_clk_filenames(start_date, end_date):
    names = set()

    for d in date_range(start_date, end_date):
        gps_week = int((d - datetime.date(1980, 1, 6)).days / 7)
        dow = (d - datetime.date(1980, 1, 6)).days % 7
        yyyy = f"{d.year:04d}"
        ddd = f"{d.timetuple().tm_yday:03d}"

        names.add(f"COD0MGXFIN_{yyyy}{ddd}0000_01D_30S_CLK.CLK")
        names.add(f"cod{gps_week}{dow}.eph")

    return names


def _expected_erp_filenames(start_date, end_date):
    names = set()

    for d in date_range(start_date, end_date):
        gps_week = int((d - datetime.date(1980, 1, 6)).days / 7)
        dow = (d - datetime.date(1980, 1, 6)).days % 7
        yyyy = f"{d.year:04d}"
        ddd = f"{d.timetuple().tm_yday:03d}"

        names.add(f"COD0MGXFIN_{yyyy}{ddd}0000_01D_12H_ERP.ERP")

    return names


def _expected_ionex_filenames(start_date, end_date):
  names = set()
  for d in date_range(start_date, end_date):
    yyyy = f"{d.year:04d}"
    ddd = f"{d.timetuple().tm_yday:03d}"
    yy = str(d.year)[-2:]
    names.add(f"COD0OPSFIN_{yyyy}{ddd}0000_01D_01H_GIM.INX")
    names.add(f"codg{ddd}0.{yy}i")
  return names


def _expected_osb_filenames(start_date, end_date):
    names = set()
    for d in date_range(start_date, end_date):
        yyyy = f"{d.year:04d}"
        ddd = f"{d.timetuple().tm_yday:03d}"
        names.add(f"COD0OPSFIN_{yyyy}{ddd}0000_01D_01D_OSB.BIA")
        names.add(f"COD0MGXFIN_{yyyy}{ddd}0000_01D_01D_OSB.BIA")
    return names

def clean_outside_range(base_dir, start_date, end_date):
  if not os.path.exists(base_dir):
    return

  checks = [
    (os.path.join(base_dir, "nav"), _expected_nav_filenames(start_date, end_date)),
    (os.path.join(base_dir, "sp3"), _expected_sp3_filenames(start_date, end_date)),
    (os.path.join(base_dir, "clk"), _expected_clk_filenames(start_date, end_date)),
    (os.path.join(base_dir, "inx"), _expected_ionex_filenames(start_date, end_date)),
    (os.path.join(base_dir, "erp"), _expected_erp_filenames(start_date, end_date)),
    (os.path.join(base_dir, "osb"), _expected_osb_filenames(start_date, end_date)),

  ]

  for subdir, valid_names in checks:
    if not os.path.exists(subdir):
      continue
    for name in os.listdir(subdir):
      path = os.path.join(subdir, name)
      if os.path.isfile(path) and name not in valid_names:
        log(f"Removing outdated file: {relpath(path)}")
        os.remove(path)


def download_obs_site(date, output_dir, site_id):
  year = date.year
  doy = date.timetuple().tm_yday
  yy = str(year)[-2:]

  yyyy = f"{year:04d}"
  ddd = f"{doy:03d}"

  site4 = site_id[:4].lower()
  site4_upper = site_id[:4].upper()
  station_full = site_id.upper()

  site_dir = os.path.join(output_dir, site4_upper)
  log(f"Trying RINEX3: {site_dir}")
  os.makedirs(site_dir, exist_ok=True)

  # ===================== RINEX3 =====================
  rnx3_name = f"{station_full}_R_{yyyy}{ddd}0000_01D_30S_MO.crx.gz"

  url = f"https://cddis.nasa.gov/archive/gnss/data/daily/{yyyy}/{ddd}/{yy}d/{rnx3_name}"

  path = os.path.join(site_dir, rnx3_name)

  log(f"Trying RINEX3: {url}")

  if download_and_extract(url, path):
    return True

  # ===================== LEGACY RINEX2 =====================
  legacy_name = f"{site4}{ddd}0.{yy}d.Z"

  url = f"https://cddis.nasa.gov/archive/gnss/data/daily/{yyyy}/{ddd}/{yy}d/{legacy_name}"

  path = os.path.join(site_dir, legacy_name)

  log(f"Trying RINEX2 legacy: {url}")

  if download_and_extract(url, path):
    return True

  error(f"OBS not found: {site_id} {yyyy}-{ddd}")

  return False


def clean_obs_dir(base_dir):
  obs_dir = os.path.join(base_dir, "OBS")
  if not os.path.exists(obs_dir):
    return

  for site_name in os.listdir(obs_dir):
    site_path = os.path.join(obs_dir, site_name)
    if os.path.isdir(site_path):
      for name in os.listdir(site_path):
        path = os.path.join(site_path, name)
        if os.path.isfile(path):
          log(f"Removing obs file: {relpath(path)}")
          os.remove(path)


def download_obs_day(date, base_dir, site_ids):
  obs_dir = os.path.join(base_dir, "OBS")
  os.makedirs(obs_dir, exist_ok=True)

  for site_id in site_ids:
    download_obs_site(date, obs_dir, site_id)


def download_products_day(date, base_dir):
  date_str = date.strftime("%Y-%m-%d")

  nav_dir = os.path.join(base_dir, "nav")
  sp3_dir = os.path.join(base_dir, "sp3")
  clk_dir = os.path.join(base_dir, "clk")
  inx_dir = os.path.join(base_dir, "inx")
  erp_dir = os.path.join(base_dir, "erp")
  osb_dir = os.path.join(base_dir, "osb")

  os.makedirs(nav_dir, exist_ok=True)
  os.makedirs(sp3_dir, exist_ok=True)
  os.makedirs(clk_dir, exist_ok=True)
  os.makedirs(inx_dir, exist_ok=True)
  os.makedirs(erp_dir, exist_ok=True)

  log(f"Processing date: {date_str}")

  download_brdc_gps(date, nav_dir)
  download_sp3(date, sp3_dir)
  download_clk(date, clk_dir)
  download_ionex(date, inx_dir)
  download_erp(date, erp_dir)
  download_osb(date, osb_dir)

def clean_all(base_dir):
  for subdir in ("nav", "sp3", "clk", "inx", "erp"):
    path = os.path.join(base_dir, subdir)
    if os.path.exists(path):
      log(f"Cleaning: {relpath(path)}")
      shutil.rmtree(path)
      os.makedirs(path)


def process_range(start_date, end_date, base_dir, site_ids=None):
  log(f"Processing range: {start_date} -> {end_date}")

  os.makedirs(base_dir, exist_ok=True)
  clean_all(base_dir)

  for current in date_range(start_date, end_date):
    window = get_window_dates(current)
    window_start, window_end = window[0], window[-1]

    log(f"--- Window for {window_start} {current} {window_end}")
    clean_outside_range(base_dir, window_start, window_end)

    nav_dir = os.path.join(base_dir, "nav")
    os.makedirs(nav_dir, exist_ok=True)
    #download_brdm(current, nav_dir)
    download_brdc_glo(current, nav_dir)

    if site_ids:
      clean_obs_dir(base_dir)
      download_obs_day(current, base_dir, site_ids)

    for d in window:
      download_products_day(d, base_dir)


def main():
  parser = argparse.ArgumentParser(description="Download GNSS data range")

  parser.add_argument("--from", dest="date_from", required=True, help="Start date (YYYY-MM-DD)")
  parser.add_argument("--to", dest="date_to", required=True, help="End date (YYYY-MM-DD)")
  parser.add_argument("--sites", dest="sites", required=True, help="Comma-separated list of station IDs (e.g. ALGO,BAKO)")
  parser.add_argument("--out", default=DEFAULT_OUTPUT_DIR)

  args = parser.parse_args()

  date_from = datetime.datetime.strptime(args.date_from, "%Y-%m-%d").date()
  date_to = datetime.datetime.strptime(args.date_to, "%Y-%m-%d").date()

  if date_from > date_to:
    raise ValueError("--from > --to")

  site_ids = [s.strip() for s in args.sites.split(",") if s.strip()]
  if not site_ids:
    raise ValueError("--sites must contain at least one station ID")

  process_range(date_from, date_to, args.out, site_ids)


if __name__ == "__main__":
  try:
    #setup_debug()
    main()
  except KeyboardInterrupt:
    error("Cancelled by user")
  except Exception as e:
    error(f"Fatal error: {e}")
    import traceback
    traceback.print_exc()
    input("Press Enter to exit...")
