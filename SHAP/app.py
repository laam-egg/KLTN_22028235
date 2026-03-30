import traceback

import multiprocessing as mp
mp.set_start_method("spawn", force=True)

import os
import uuid
import hmac
import hashlib
import tempfile
import shutil
from pathlib import Path
from enum import Enum
from typing import Dict, Optional
from concurrent.futures import ProcessPoolExecutor, Future

from fastapi import (
    FastAPI,
    UploadFile,
    File,
    Form,
    Depends,
    HTTPException,
    Request,
)
from fastapi.responses import HTMLResponse, RedirectResponse
from fastapi.middleware.cors import CORSMiddleware

from explainer import explain


# ==================================================
# Configuration
# ==================================================

MAX_FILE_SIZE = 150 * 1024 * 1024  # 150 MB

MASTER_PASSWORD = os.environ.get("XAI_MASTER_PASSWORD", "changeme")
SECRET_KEY = os.environ.get("XAI_SECRET_KEY", "dev-secret").encode()

TMP_ROOT = Path(tempfile.gettempdir()) / "xai_jobs"
TMP_ROOT.mkdir(exist_ok=True)

CPU_WORKERS = max(os.cpu_count() - 1, 1)


# ==================================================
# App & Executor
# ==================================================

from contextlib import asynccontextmanager

@asynccontextmanager
async def lifespan(app: FastAPI):
    executor = ProcessPoolExecutor(max_workers=CPU_WORKERS)
    app.state.executor = executor
    try:
        yield
    finally:
        executor.shutdown(wait=True, cancel_futures=True)

app = FastAPI(title="xAI Explainer", lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ==================================================
# Auth
# ==================================================

def _sign(value: str) -> str:
    sig = hmac.new(SECRET_KEY, value.encode(), hashlib.sha256).hexdigest()
    return f"{value}.{sig}"

def _verify(token: str) -> bool:
    try:
        value, sig = token.rsplit(".", 1)
        expected = hmac.new(SECRET_KEY, value.encode(), hashlib.sha256).hexdigest()
        return hmac.compare_digest(sig, expected)
    except Exception:
        return False

def require_auth(request: Request):
    token = request.cookies.get("auth")
    if not token or not _verify(token):
        raise HTTPException(status_code=401, detail="Not authenticated")


# ==================================================
# Job System (parent-process only state)
# ==================================================

class JobStatus(str, Enum):
    RUNNING = "running"
    DONE = "done"
    ERROR = "error"

class Job:
    def __init__(self, job_id: str, workdir: Path, future: Future):
        self.id = job_id
        self.workdir = workdir
        self.future = future
        self.status = JobStatus.RUNNING
        self.error: Optional[str] = None

jobs: Dict[str, Job] = {}


# ==================================================
# Worker (pure function, no shared state)
# ==================================================

def worker(input_path: str) -> str:
    return explain(input_path)


# ==================================================
# Routes
# ==================================================

@app.get("/", response_class=HTMLResponse)
def index():
    return """
    <h2>xAI Explainer</h2>
    <form action="/login" method="post">
        <input type="password" name="password" placeholder="Master password"/>
        <button type="submit">Login</button>
    </form>
    """

@app.post("/login")
def login(password: str = Form(...)):
    if password != MASTER_PASSWORD:
        raise HTTPException(status_code=403, detail="Wrong password")

    token = _sign("ok")
    resp = RedirectResponse("/upload", status_code=302)
    resp.set_cookie("auth", token, httponly=True)
    return resp


@app.get("/upload", response_class=HTMLResponse)
def upload_page(_: None = Depends(require_auth)):
    return """
    <h3>Upload file</h3>
    <form action="/explain" method="post" enctype="multipart/form-data">
        <input type="file" name="file"/>
        <button type="submit">Explain</button>
    </form>
    """


@app.post("/explain")
async def explain_file(
    request: Request,
    file: UploadFile = File(...),
    _: None = Depends(require_auth),
):
    job_id = uuid.uuid4().hex
    workdir = TMP_ROOT / job_id
    workdir.mkdir(parents=True)

    input_path = workdir / file.filename

    size = 0
    with input_path.open("wb") as f:
        while chunk := await file.read(1024 * 1024):
            size += len(chunk)
            if size > MAX_FILE_SIZE:
                shutil.rmtree(workdir, ignore_errors=True)
                raise HTTPException(status_code=413, detail="File too large")
            f.write(chunk)

    executor = request.app.state.executor
    future = executor.submit(worker, str(input_path))
    jobs[job_id] = Job(job_id, workdir, future)

    return RedirectResponse(f"/status/{job_id}", status_code=302)


@app.get("/status/{job_id}", response_class=HTMLResponse)
def job_status(job_id: str, _: None = Depends(require_auth)):
    job = jobs.get(job_id)
    if not job:
        raise HTTPException(status_code=404)

    if job.future.done():
        return RedirectResponse(f"/result/{job_id}", status_code=302)

    return f"""
    <p>Status: running</p>
    <script>
        setTimeout(() => location.reload(), 2000);
    </script>
    """


@app.get("/result/{job_id}", response_class=HTMLResponse)
def job_result(job_id: str, _: None = Depends(require_auth)):
    job = jobs.get(job_id)
    if not job:
        raise HTTPException(status_code=404)

    if not job.future.done():
        raise HTTPException(status_code=409, detail="Job still running")

    try:
        html = job.future.result()
        job.status = JobStatus.DONE
    except Exception as e:
        job.error = traceback.format_exc()
        job.status = JobStatus.ERROR
        raise HTTPException(status_code=500, detail=job.error)
    finally:
        # 🔥 IMMEDIATE CLEANUP
        shutil.rmtree(job.workdir, ignore_errors=True)
        jobs.pop(job_id, None)

    return HTMLResponse(html)


# ==================================================
# Entry Point (Windows-safe)
# ==================================================

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("app:app", host="0.0.0.0", port=8000)
