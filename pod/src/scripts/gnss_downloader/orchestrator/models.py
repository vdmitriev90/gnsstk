from dataclasses import dataclass
from datetime import datetime
from typing import Optional, Tuple


@dataclass(frozen=True)
class Session:
  name: str
  start: datetime
  end: datetime
  rovers: Tuple[str, ...]
  base: Optional[str] = None


@dataclass(frozen=True)
class OrchestratorConfig:
  processor_executable: str
  processor_ini: str
  project_name: str
  working_directory: str
  sessions: Tuple[Session, ...]


@dataclass(frozen=True)
class ProcessingJob:
  session: Session
  rover: str
  base: Optional[str] = None
