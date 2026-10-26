import datetime
import os
import shutil

from gnss_downloader.logging_utils import log, log_file, relpath
from gnss_downloader.download.products import (
  download_ionex,
  download_clk,
  download_sp3,
  download_erp,
  download_osb,
  download_brdc_gps,
  download_brdc_glo,
)
from gnss_downloader.download.observations import download_obs_day, clean_obs_dir


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
        names.add(f"COD0R03FIN_{yyyy}{ddd}0000_01D_05M_ORB.SP3")

    return names




def _expected_clk_filenames(start_date, end_date):
    names = set()

    for d in date_range(start_date, end_date):
        gps_week = int((d - datetime.date(1980, 1, 6)).days / 7)
        dow = (d - datetime.date(1980, 1, 6)).days % 7
        yyyy = f"{d.year:04d}"
        ddd = f"{d.timetuple().tm_yday:03d}"

        names.add(f"COD0MGXFIN_{yyyy}{ddd}0000_01D_30S_CLK.CLK")
        names.add(f"cod{gps_week}{dow}.clk_05s")
        names.add(f"COD0R03FIN_{yyyy}{ddd}0000_01D_30S_CLK.CLK")

    return names


def _expected_erp_filenames(start_date, end_date):
    names = set()

    for d in date_range(start_date, end_date):
        gps_week = int((d - datetime.date(1980, 1, 6)).days / 7)
        dow = (d - datetime.date(1980, 1, 6)).days % 7
        yyyy = f"{d.year:04d}"
        ddd = f"{d.timetuple().tm_yday:03d}"

        names.add(f"COD0MGXFIN_{yyyy}{ddd}0000_03D_12H_ERP.ERP")
        names.add(f"COD0MGXFIN_{yyyy}{ddd}0000_01D_12H_ERP.ERP")
        names.add(f"COD0R03FIN_{yyyy}{ddd}0000_01D_01D_ERP.ERP")

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
        names.add(f"COD0R03FIN_{yyyy}{ddd}0000_01D_01D_OSB.BIA")
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
  os.makedirs(osb_dir, exist_ok=True)

  download_sp3(date, sp3_dir)
  download_clk(date, clk_dir)
  download_erp(date, erp_dir)
  download_osb(date, osb_dir)
  download_ionex(date, inx_dir)


def clean_all(base_dir):
  for subdir in ("nav", "sp3", "clk", "inx", "erp"):
    path = os.path.join(base_dir, subdir)
    if os.path.exists(path):
      log(f"Cleaning: {relpath(path)}")
      shutil.rmtree(path)
      os.makedirs(path)


def process_range(start_date, end_date, base_dir, site_ids=None, rovers=None, base=None, on_rover_ready=None):
  log(f"Processing range: {start_date} -> {end_date}")

  os.makedirs(base_dir, exist_ok=True)
  clean_all(base_dir)

  for current in date_range(start_date, end_date):
    window = get_window_dates(current)
    window_start, window_end = window[0], window[-1]

    log_file(f"Processing date: {current}")
    log(f"--- Window for {window_start} {current} {window_end}")
    clean_outside_range(base_dir, window_start, window_end)

    for d in window:
      download_products_day(d, base_dir)

    nav_dir = os.path.join(base_dir, "nav")
    os.makedirs(nav_dir, exist_ok=True)
    download_brdc_gps(current, nav_dir)
    download_brdc_glo(current, nav_dir)

    if rovers:
      clean_obs_dir(base_dir)
      if base:
        download_obs_day(current, base_dir, [base])
      for rover in rovers:
        download_obs_day(current, base_dir, [rover])
        if on_rover_ready:
          on_rover_ready(current, rover, base)
    elif site_ids:
      clean_obs_dir(base_dir)
      download_obs_day(current, base_dir, site_ids)

