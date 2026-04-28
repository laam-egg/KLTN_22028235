from datetime import datetime, timezone
from sqlalchemy import String, Text, DateTime
from sqlalchemy.orm import Mapped, mapped_column
from .database import Base
from sqlalchemy.dialects.mysql import MEDIUMTEXT


class Job(Base):
    __tablename__ = "jobs"

    sha256: Mapped[str] = mapped_column(String(64), primary_key=True)
    file_path: Mapped[str] = mapped_column(String(512), nullable=False)
    status: Mapped[str] = mapped_column(String(16), nullable=False, default="Pending")
    result_html: Mapped[str | None] = mapped_column(MEDIUMTEXT, nullable=True)
    error_message: Mapped[str | None] = mapped_column(MEDIUMTEXT, nullable=True)
    created_at: Mapped[datetime] = mapped_column(
        DateTime, default=lambda: datetime.now(timezone.utc)
    )
    updated_at: Mapped[datetime] = mapped_column(
        DateTime,
        default=lambda: datetime.now(timezone.utc),
        onupdate=lambda: datetime.now(timezone.utc),
    )
