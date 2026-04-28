from pathlib import Path
SCRIPT_DIR = Path(__file__).resolve().parent
CORE_EXPLAINER_PATH = SCRIPT_DIR.parent / "core"

import sys
sys.path.append(str(CORE_EXPLAINER_PATH))

# def explain(file_path: str) -> str:
#     """
#     Analyze a PE file and return an HTML report.
#     """
from explainer import explain
