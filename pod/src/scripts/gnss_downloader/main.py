import argparse
import sys
import os

from gnss_downloader.logging_utils import error, set_log_file, log_file
from gnss_downloader.download.common import TIMEOUT, RETRIES
from gnss_downloader.debug_utils import setup_debug
from gnss_downloader.orchestrator import load_orchestrator_config, run_orchestrator


def parse_args():
  parser = argparse.ArgumentParser(description="GNSS session-based processing orchestrator")
  parser.add_argument("--config", required=True, help="Path to orchestrator.ini")
  parser.add_argument("--debug", action="store_true", help="Wait for debugger attach")
  return parser.parse_args()


def main(args):
  config_path = os.path.abspath(args.config)
  set_log_file(os.path.dirname(config_path))
  log_file("=== Start ===")
  log_file(f"Command: {' '.join(sys.argv)}")
  log_file(f"Arguments: config={config_path} debug={args.debug}")
  log_file(f"Settings: TIMEOUT={TIMEOUT} RETRIES={RETRIES} cwd={os.getcwd()}")

  config = load_orchestrator_config(config_path)
  run_orchestrator(config)


if __name__ == "__main__":
  try:
    args = parse_args()
    if args.debug:
      setup_debug()
    main(args)
  except KeyboardInterrupt:
    error("Cancelled by user")
  except Exception as e:
    error(f"Fatal error: {e}")
    import traceback
    traceback.print_exc()
    input("Press Enter to exit...")
