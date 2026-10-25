import datetime
import argparse
import sys
import os

from gnss_downloader.logging_utils import error, set_log_file, log, log_file
from gnss_downloader.download.common import TIMEOUT, RETRIES
from gnss_downloader.debug_utils import setup_debug
from gnss_downloader.download.manager import process_range

DEFAULT_OUTPUT_DIR = "./nav_files"


def main():
  parser = argparse.ArgumentParser(description="Download GNSS data range")

  parser.add_argument("--from", dest="date_from", required=True, help="Start date (YYYY-MM-DD)")
  parser.add_argument("--to", dest="date_to", required=True, help="End date (YYYY-MM-DD)")
  parser.add_argument("--sites", dest="sites", required=True, help="Comma-separated list of station IDs (e.g. ALGO,BAKO)")
  parser.add_argument("--out", default=DEFAULT_OUTPUT_DIR)

  args = parser.parse_args()

  set_log_file(args.out)
  log_file("=== Start ===")
  log_file(f"Command: {' '.join(sys.argv)}")
  log_file(
    f"Arguments: from={args.date_from} to={args.date_to} sites={args.sites} "
    f"out={os.path.abspath(args.out)}"
  )
  log_file(f"Settings: TIMEOUT={TIMEOUT} RETRIES={RETRIES} cwd={os.getcwd()}")

  date_from = datetime.datetime.strptime(args.date_from, "%Y-%m-%d").date()
  date_to = datetime.datetime.strptime(args.date_to, "%Y-%m-%d").date()

  if date_from > date_to:
    raise ValueError("--from > --to")

  site_ids = [s.strip() for s in args.sites.split(",") if s.strip()]
  if not site_ids:
    raise ValueError("--sites must contain at least one station ID")
  
  log(f"Output directory: {args.out}")

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
