import argparse
import shutil
import sys
import os

from gnss_downloader.logging_utils import error, set_log_file, log_file
from gnss_downloader.download.common import TIMEOUT, RETRIES
from gnss_downloader.debug_utils import setup_debug
from gnss_downloader.orchestrator import load_orchestrator_config, run_orchestrator, ProjectLayout


def parse_args():
  parser = argparse.ArgumentParser(description="GNSS session-based processing orchestrator")
  parser.add_argument("--config", required=True, help="Path to orchestrator.ini")
  parser.add_argument("--debug", action="store_true", help="Wait for debugger attach")
  return parser.parse_args()


def main(args):

  config_path = os.path.abspath(args.config)
  config = load_orchestrator_config(config_path)

  layout = ProjectLayout.from_config(config)
  layout.create_directories()
  set_log_file(layout.logs_dir)
  log_file("=== Start ===")
  log_file(f"Command: {' '.join(sys.argv)}")
  log_file(f"Arguments: config={config_path} debug={args.debug}")
  log_file(f"Settings: TIMEOUT={TIMEOUT} RETRIES={RETRIES} cwd={os.getcwd()}")
  log_file(f"Project directory: {layout.root}")

  config_copy = os.path.join(layout.configs_dir, os.path.basename(config_path))
  if os.path.normcase(os.path.abspath(config_copy)) != os.path.normcase(config_path):
    shutil.copyfile(config_path, config_copy)
    log_file(f"Config copied to: {config_copy}")

  run_orchestrator(config, layout)


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
    if sys.stdin is not None and sys.stdin.isatty():
      input("Press Enter to exit...")
    sys.exit(1)
