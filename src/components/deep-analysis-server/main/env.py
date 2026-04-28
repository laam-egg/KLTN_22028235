import dotenv
dotenv.load_dotenv()

import os
from typing import Any

def required_env(name: str):
    try:
        return os.environ[name]
    except:
        raise RuntimeError(f"Environment variable {name} is invalid or missing.")

def optional_env(name: str, default_value: Any):
    return os.environ.get(name, default_value)

MYSQL_HOST = optional_env("MYSQL_HOST", "db")
MYSQL_PORT = optional_env("MYSQL_PORT", "3306")
MYSQL_USER = required_env("MYSQL_USER")
MYSQL_PASSWORD = required_env("MYSQL_PASSWORD")
MYSQL_DATABASE = required_env("MYSQL_DATABASE")
