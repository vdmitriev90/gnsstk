import datetime

from gnss_downloader.logging_utils import log, log_file
from gnss_downloader.download.manager import process_range
from gnss_downloader.orchestrator.models import ProcessingJob


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


def run_orchestrator(config, layout):
  jobs = expand_jobs(config)

  _info(f"Working directory: {config.working_directory}")
  _info(f"Project: {config.project_name}")
  _info(f"Project directory: {layout.root}")
  _info(f"Processor executable: {config.processor_executable}")
  _info(f"Processor INI template: {config.processor_ini}")
  _info(f"Execution plan: {len(config.sessions)} session(s), {len(jobs)} job(s)")


  for session in config.sessions:
    start_date, end_date = session_dates(session)
    sites = session_sites(session)
    _info(f"Downloading session {session.name}: {start_date} -> {end_date} sites={' '.join(sites)}")
    process_range(start_date, end_date, layout.data_dir, sites)
    _info(f"Session {session.name} downloaded")

  return jobs
