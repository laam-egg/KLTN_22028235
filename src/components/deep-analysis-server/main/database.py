import os
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker, DeclarativeBase
from .env import (
    MYSQL_HOST, MYSQL_PORT,
    MYSQL_USER, MYSQL_PASSWORD,
    MYSQL_DATABASE,
)

def _build_url() -> str:
    return f"mysql+pymysql://{MYSQL_USER}:{MYSQL_PASSWORD}@{MYSQL_HOST}:{MYSQL_PORT}/{MYSQL_DATABASE}"

engine = create_engine(
    _build_url(),
    pool_pre_ping=True,        # detect stale connections
    pool_recycle=1800,         # recycle connections every 30 min
)

SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)


class Base(DeclarativeBase):
    pass


def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()


def init_db():
    from .models import Job
    Base.metadata.create_all(bind=engine)
