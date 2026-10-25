from gnss_downloader.logging_utils import log, log_file
from gnss_downloader.orchestrator.models import ProcessingJob


def expand_jobs(config):
  return tuple(
    ProcessingJob(session=session, rover=rover, base=session.base)
    for session in config.sessions
    for rover in session.rovers
  )


def _info(msg):
  log(msg)
  log_file(msg)


def run_orchestrator(config):
  jobs = expand_jobs(config)

  _info(f"Project: {config.project_name}")
  _info(f"Processor executable: {config.processor_executable}")
  _info(f"Processor INI template: {config.processor_ini}")
  _info(f"Working directory: {config.working_directory}")
  _info(f"Execution plan:

  for i, job in enumerate(jobs, 1):
    s = job.session
    _info(
      f"  [{i}] session={s.name} "
      f"start={s.start.isoformat()} end={s.end.isoformat()} "
      f"rover={job.rover} base={job.base or '-'}"
    )

  return jobs
