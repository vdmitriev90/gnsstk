import datetime
import os

from gnss_downloader.logging_utils import log, error, relpath
from gnss_downloader.download.common import download_and_extract, fail


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

  fail(f"IONEX download failed for {date:%Y-%m-%d}")


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

    fail(f"CLK download failed for {date:%Y-%m-%d}")


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

    fail(f"SP3 download failed for {date:%Y-%m-%d}")


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

    fail(f"ERP download failed for {date:%Y-%m-%d}")

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
        fail(f"OSB download failed for {date:%Y-%m-%d}")
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
    fail(f"BRDC nav download failed for {date:%Y-%m-%d}")


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
    fail(f"BRDC GLO nav download failed for {date:%Y-%m-%d}")

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

  fail("Both BRDM and BRDC download failed")
