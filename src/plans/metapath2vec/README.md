# metapath2vec

## Design Document

[Here](./docs/DESIGN.md)

## Steps

### Setup

```sh
virtualenv venv
source ./venv/bin/activate
pip install -r requirements.txt
```

### Data Sanitization

[Here](./docs/SANITIZE_DATA.md)

### Training and Evaluation

```sh
python -m main ./data/metapath2vec.sqlite3
```
