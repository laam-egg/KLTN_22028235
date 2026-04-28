from datetime import datetime, timezone
from .analyzer import explain
from .database import SessionLocal
from .models import Job


def run_analysis(sha256: str, file_path: str) -> None:
    """
    Called in a worker thread. Runs explain() and updates the DB with the result.
    """
    db = SessionLocal()
    try:
        result_html = explain(file_path)
        job = db.get(Job, sha256)
        if job:
            job.status = "Success"
            job.result_html = result_html
            job.error_message = None
            job.updated_at = datetime.now(timezone.utc)
            db.commit()
    except Exception as exc:
        db.rollback()
        job = db.get(Job, sha256)
        if job:
            job.status = "Failed"
            job.error_message = str(exc)
            job.updated_at = datetime.now(timezone.utc)
            db.commit()
    finally:
        db.close()
