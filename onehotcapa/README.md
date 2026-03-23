# Idea

Baseline: concatenate original EMBER2024 features with
one-hot encoding of Capa-identified capabilities and
behaviors.

Later: use another algorithm.

## Setup

### Dependencies

```sh
pip install -r requirements.txt
```

### Extract feature vectors (PE+Capa)

```sh
export DS=/path/to/EMBER2024/JSONL/dataset/dir # no trailing slash!!!
cd scripts

python enumerate_capa_features.py $DS capa_features.json
python vectorize.py $DS capa_features.json
```
