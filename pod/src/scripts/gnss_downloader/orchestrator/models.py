import os
from dataclasses import dataclass
from datetime import datetime
from typing import Optional, Tuple


@dataclass(frozen=True)
class TimeSpan:
  start: datetime
  end: datetime

  def __post_init__(self):
    if self.start >= self.end:
      raise ValueError("start must be earlier than end")


@dataclass(frozen=True)
class RoverSet:
  sites: Tuple[str, ...]
  base: Optional[str] = None

  def __post_init__(self):
    if not self.sites:
      raise ValueError("at least one rover is required")
    duplicates = sorted({r for r in self.sites if self.sites.count(r) > 1})
    if duplicates:
      raise ValueError(f"duplicate rovers: {', '.join(duplicates)}")
    if self.base is not None and self.base in self.sites:
      raise ValueError(f"base {self.base} must differ from every rover")

  def __iter__(self):
    return iter(self.sites)

  def __len__(self):
    return len(self.sites)

  def __contains__(self, site):
    return site in self.sites


@dataclass(frozen=True)
class Session:
  name: str
  time_span: TimeSpan
  rover_set: RoverSet
  base: Optional[str] = None

  @property
  def start(self):
    return self.time_span.start

  @property
  def end(self):
    return self.time_span.end

  @property
  def rovers(self):
    return self.rover_set.sites


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
  logs_dir: str
  data_dir: str

  @classmethod
  def from_config(cls, config):
    root = os.path.normpath(os.path.join(config.working_directory, config.project_name))
    return cls(
      root=root,
      statistics_db=os.path.join(root, "statistics.db"),
      solutions_dir=os.path.join(root, "solutions"),
      logs_dir=os.path.join(root, "logs"),
      data_dir=os.path.join(root, "data"),
    )

  def create_directories(self):
    for path in (self.root, self.solutions_dir, self.logs_dir, self.data_dir):
      os.makedirs(path, exist_ok=True)


@dataclass(frozen=True)
class ProcessingJob:
  session: Session
  rover: str
  base: Optional[str] = None
