import datetime
import os

from gnss_downloader.logging_utils import log, log_file, get_log_file
from gnss_downloader.download.manager import process_range
from gnss_downloader.orchestrator.models import ProcessingJob
from gnss_downloader.orchestrator.processor import (
  copy_processor_template, write_job_ini, run_processor, read_key)


def expand_jobs(config):
  return tuple(
    ProcessingJob(session=session, rover=rover, base=session.base)
    for session in config.sessions
    for rover in session.rovers
  )


def session_sites(session):
  sites = list(session.rovers)
  if session.base:
    sites.append(session.base)
  return sites


def session_dates(session):
  start_date = session.start.date()
  end_date = session.end.date()
  if session.end.time() == datetime.time(0, 0) and end_date > start_date:
    end_date -= datetime.timedelta(days=1)
  return start_date, end_date


def _info(msg):
  log(msg)
  log_file(msg)


def run_orchestrator(config, layout, no_download=False):
  jobs = expand_jobs(config)

  _info(f"Working directory: {config.working_directory}")
  _info(f"Project: {config.project_name}")
  _info(f"Project directory: {layout.root}")
  _info(f"Processor executable: {config.processor_executable}")
  _info(f"Processor INI template: {config.processor_ini}")
  _info(f"Execution plan: {len(config.sessions)} session(s), {len(jobs)} job(s)")


  template_copy = copy_processor_template(config.processor_ini, layout)
  use_ionex = read_key(template_copy, "CodeIonoCorrType") == "2"
  _info(f"Ionex download: {use_ionex}")

  for session in config.sessions:
    start_date, end_date = session_dates(session)
    sites = session_sites(session)
    _info(f"Downloading session {session.name}: {start_date} -> {end_date} sites={' '.join(sites)}")

    def on_rover_ready(day, rover, base, session=session):
      stamp = f"{session.name}_{rover}_{day:%Y%m%d}"
      ini_path = template_copy
      write_job_ini(ini_path, rover, base)
      _info(f"Processing {stamp} with {ini_path}")
      run_processor(config.processor_executable, ini_path, layout.root, get_log_file())

    if no_download:
      _info(f"Download skipped, Run processing session")
      run_processor(config.processor_executable, template_copy, layout.root, get_log_file())
    else:
      process_range(
        start_date, end_date, layout.data_dir, sites,
        rovers=list(session.rovers), base=session.base, on_rover_ready=on_rover_ready,
        use_ionex=use_ionex)
    _info(f"Session {session.name} done")

  return jobs
