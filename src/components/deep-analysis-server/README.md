# Deep Analysis Server

Allows automatic PE sample submission (via API)
for deeper analysis. Also supports manual uploads.

Model used: EMBER2024+TruncatedSVD256

## Setup

### Install OpenSSL libraries

For example, on Ubuntu:

```sh
sudo apt-get install openssl libssl-dev
```

On Windows, the required OpenSSL DLLs
are already included in the repository
(`libcrypto-3-x64.dll` and
`libssl-3-x64.dll`).

### Install Python packages

```sh
virtualenv venv
pip install -r requirements.txt
```

### Environment variables

```sh
cp .env.example .env
```

then edit the variables.

## Run

```sh
source ./venv/bin/activate
uvicorn main.__main__:app --reload
```
