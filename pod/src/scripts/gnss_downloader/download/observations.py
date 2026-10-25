import os

from gnss_downloader.logging_utils import log, error, relpath
from gnss_downloader.download.common import download_and_extract, fail


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

  fail(f"OBS not found: {site_id} {yyyy}-{ddd}")


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
