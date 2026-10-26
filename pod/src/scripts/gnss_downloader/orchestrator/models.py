import os
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
class ProjectLayout:
  root: str
  statistics_db: str
  solutions_dir: str
  configs_dir: str
  logs_dir: str
  state_dir: str
  data_dir: str

  @classmethod
  def from_config(cls, config):
    root = os.path.normpath(os.path.join(config.working_directory, config.project_name))
    return cls(
      root=root,
      statistics_db=os.path.join(root, "statistics.db"),
      solutions_dir=os.path.join(root, "solutions"),
      configs_dir=os.path.join(root, "configs"),
      logs_dir=os.path.join(root, "logs"),
      state_dir=os.path.join(root, "state"),
      data_dir=os.path.join(root, "data"),
    )

  def create_directories(self):
    for path in (self.root, self.solutions_dir, self.configs_dir,
                 self.logs_dir, self.state_dir, self.data_dir):
      os.makedirs(path, exist_ok=True)


@dataclass(frozen=True)
class ProcessingJob:
  session: Session
  rover: str
  base: Optional[str] = None
