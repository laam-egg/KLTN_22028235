import hashlib
import shutil
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
from typing import Literal

import aiofiles
from fastapi import Depends, FastAPI, File, HTTPException, Request, UploadFile
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.templating import Jinja2Templates
from pydantic import BaseModel, Field
from sqlalchemy.orm import Session

from .database import get_db, init_db
from .models import Job
from .worker import run_analysis

SCRIPT_DIR = Path(__file__).resolve().parent
MODULE_DIR = SCRIPT_DIR.parent
UPLOAD_DIR = MODULE_DIR / "uploads"
UPLOAD_DIR.mkdir(exist_ok=True)

executor = ThreadPoolExecutor(max_workers=4)

# ---------------------------------------------------------------------------
# App
# ---------------------------------------------------------------------------

app = FastAPI(
    title="PE Analysis API",
    description="""
Analyze Portable Executable (PE) files via static analysis.

## Workflow

1. **Upload** a PE file via `POST /lookup` → receive an analysis page URL.
2. **Poll** `GET /status/{sha256}` to track progress (`Pending → Success | Failed`).
3. **Retry** a failed job via `POST /retry/{sha256}` — no re-upload needed.

The homepage (`GET /`) provides a drag-and-drop upload UI and a list of recent samples.
""",
    version="1.0.0",
    contact={"name": "PE Analysis"},
    license_info={"name": "MIT"},
)

templates = Jinja2Templates(directory=SCRIPT_DIR / "templates")


@app.on_event("startup")
def startup():
    import time
    from sqlalchemy.exc import OperationalError
    for attempt in range(10):
        try:
            init_db()
            return
        except OperationalError:
            if attempt == 9:
                raise
            time.sleep(3)


# ---------------------------------------------------------------------------
# Pydantic response schemas
# ---------------------------------------------------------------------------

class LookupResponse(BaseModel):
    sha256: str = Field(
        ...,
        description="SHA256 hex digest of the uploaded file.",
        examples=["3b4c..."],
    )
    url: str = Field(
        ...,
        description="Public URL of the analysis page for this sample.",
        examples=["http://localhost:8000/analysis/3b4c..."],
    )


class StatusResponse(BaseModel):
    sha256: str = Field(..., description="SHA256 hex digest of the sample.")
    status: Literal["Pending", "Success", "Failed"] = Field(
        ..., description="Current analysis status."
    )
    result_html: str | None = Field(
        None, description="HTML analysis report. Populated when status is `Success`."
    )
    error_message: str | None = Field(
        None, description="Error details. Populated when status is `Failed`."
    )
    updated_at: str = Field(..., description="ISO-8601 timestamp of the last status change.")

    model_config = {
        "json_schema_extra": {
            "examples": [
                {
                    "sha256": "3b4c5d...",
                    "status": "Success",
                    "result_html": "<h2>Analysis complete</h2>...",
                    "error_message": None,
                    "updated_at": "2024-01-15T10:30:00+00:00",
                },
                {
                    "sha256": "3b4c5d...",
                    "status": "Pending",
                    "result_html": None,
                    "error_message": None,
                    "updated_at": "2024-01-15T10:29:55+00:00",
                },
                {
                    "sha256": "3b4c5d...",
                    "status": "Failed",
                    "result_html": None,
                    "error_message": "Not a valid PE file.",
                    "updated_at": "2024-01-15T10:30:02+00:00",
                },
            ]
        }
    }


class RetryResponse(BaseModel):
    sha256: str = Field(..., description="SHA256 hex digest of the sample.")
    url: str = Field(..., description="Public URL of the analysis page.")


class ErrorResponse(BaseModel):
    detail: str = Field(..., description="Human-readable error message.")


# ---------------------------------------------------------------------------
# Homepage
# ---------------------------------------------------------------------------

@app.get(
    "/",
    response_class=HTMLResponse,
    summary="Homepage",
    description="Drag-and-drop file upload UI and a paginated list of recently analyzed samples.",
    tags=["UI"],
    include_in_schema=False,   # Pure UI — not useful in Swagger
)
def homepage(request: Request, page: int = 1, db: Session = Depends(get_db)):
    per_page = 10
    offset = (page - 1) * per_page
    total = db.query(Job).count()
    jobs = (
        db.query(Job)
        .order_by(Job.created_at.desc())
        .offset(offset)
        .limit(per_page)
        .all()
    )
    total_pages = max(1, (total + per_page - 1) // per_page)
    return templates.TemplateResponse(
        "index.html",
        {
            "request": request,
            "jobs": jobs,
            "page": page,
            "total_pages": total_pages,
        },
    )


# ---------------------------------------------------------------------------
# Lookup
# ---------------------------------------------------------------------------

@app.post(
    "/lookup",
    response_model=LookupResponse,
    summary="Submit a PE sample for analysis",
    description="""
Upload a PE file. The server computes its SHA256 digest and returns the URL of its analysis page.

- If the file has **never been seen before**, a new analysis job is queued and the file is saved to disk.
- If the file **was already submitted** (same SHA256), the existing analysis page URL is returned immediately — no reprocessing occurs.

If the upload itself fails, no database entry is created and the sample cannot be retried.
""",
    tags=["Analysis"],
    responses={
        200: {"description": "File accepted (new or duplicate). Analysis page URL returned."},
        500: {"model": ErrorResponse, "description": "File could not be saved to disk."},
    },
)
async def lookup(request: Request, file: UploadFile = File(..., description="The PE file to analyze."), db: Session = Depends(get_db)):
    tmp_path = UPLOAD_DIR / f"__tmp_{file.filename}"
    sha256 = hashlib.sha256()

    try:
        async with aiofiles.open(tmp_path, "wb") as f:
            while chunk := await file.read(1024 * 1024):
                sha256.update(chunk)
                await f.write(chunk)
    except Exception:
        tmp_path.unlink(missing_ok=True)
        raise HTTPException(status_code=500, detail="Failed to save uploaded file.")

    digest = sha256.hexdigest()
    final_path = UPLOAD_DIR / digest

    if not final_path.exists():
        shutil.move(str(tmp_path), str(final_path))
    else:
        tmp_path.unlink(missing_ok=True)

    analysis_url = str(request.url_for("analysis_page", sha256=digest))

    existing = db.get(Job, digest)
    if existing:
        return JSONResponse({"url": analysis_url, "sha256": digest})

    job = Job(
        sha256=digest,
        file_path=str(final_path),
        status="Pending",
        created_at=datetime.now(timezone.utc),
        updated_at=datetime.now(timezone.utc),
    )
    db.add(job)
    db.commit()

    executor.submit(run_analysis, digest, str(final_path))

    return JSONResponse({"url": analysis_url, "sha256": digest})


# ---------------------------------------------------------------------------
# Analysis page
# ---------------------------------------------------------------------------

@app.get(
    "/analysis/{sha256}",
    response_class=HTMLResponse,
    summary="Analysis page",
    description="Returns the HTML analysis page for a sample. The page polls `/status/{sha256}` and renders the current result.",
    tags=["UI"],
    include_in_schema=False,   # Pure UI
)
def analysis_page(sha256: str, request: Request, db: Session = Depends(get_db)):
    job = db.get(Job, sha256)
    if not job:
        raise HTTPException(status_code=404, detail="Sample not found.")
    return templates.TemplateResponse(
        "analysis.html",
        {"request": request, "sha256": sha256, "status": job.status},
    )


# ---------------------------------------------------------------------------
# Status
# ---------------------------------------------------------------------------

@app.get(
    "/status/{sha256}",
    response_model=StatusResponse,
    summary="Get analysis status",
    description="""
Poll this endpoint to track the progress of an analysis job.

| `status`  | Meaning |
|-----------|---------|
| `Pending` | Analysis is queued or running. |
| `Success` | Analysis completed. `result_html` is populated. |
| `Failed`  | Analysis failed. `error_message` is populated. Use `POST /retry/{sha256}` to retry. |
""",
    tags=["Analysis"],
    responses={
        200: {"description": "Job found. Status and result (if available) returned."},
        404: {"model": ErrorResponse, "description": "No job exists for this SHA256."},
    },
)
def get_status(sha256: str = ..., db: Session = Depends(get_db)):
    job = db.get(Job, sha256)
    if not job:
        raise HTTPException(status_code=404, detail="Sample not found.")
    return {
        "sha256": job.sha256,
        "status": job.status,
        "result_html": job.result_html,
        "error_message": job.error_message,
        "updated_at": job.updated_at.isoformat(),
    }


# ---------------------------------------------------------------------------
# Retry
# ---------------------------------------------------------------------------

@app.post(
    "/retry/{sha256}",
    response_model=RetryResponse,
    summary="Retry a failed analysis",
    description="""
Re-trigger analysis for a sample that has already been uploaded.

- The file **must** have been successfully uploaded in a prior `POST /lookup` call.
- No file re-upload is required — the server uses the file already on disk.
- The job status is reset to `Pending` and analysis is queued again.

This endpoint is typically called from the "Retry" button on the analysis page after a `Failed` result.
""",
    tags=["Analysis"],
    responses={
        200: {"description": "Retry accepted. Analysis page URL returned."},
        404: {"model": ErrorResponse, "description": "No record of this SHA256 — file was never uploaded."},
        410: {"model": ErrorResponse, "description": "Job record exists but the file is no longer on disk."},
    },
)
def retry(sha256: str, request: Request, db: Session = Depends(get_db)):
    job = db.get(Job, sha256)
    if not job:
        raise HTTPException(status_code=404, detail="Sample not found. Was it ever uploaded?")

    file_path = Path(job.file_path)
    if not file_path.exists():
        raise HTTPException(status_code=410, detail="File no longer exists on server.")

    job.status = "Pending"
    job.result_html = None
    job.error_message = None
    job.updated_at = datetime.now(timezone.utc)
    db.commit()

    executor.submit(run_analysis, sha256, str(file_path))

    analysis_url = str(request.url_for("analysis_page", sha256=sha256))
    return JSONResponse({"url": analysis_url, "sha256": sha256})
