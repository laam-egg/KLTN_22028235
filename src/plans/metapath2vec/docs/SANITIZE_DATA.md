# Sanitizing Data

## Training Set

```sh
cd scripts
mkdir -p ../data
export DS=/path/to/EMBER2024/JSONL/dataset/dir
python sanitize.py \
    --inputs-train \
        $DS/*_Win32_train.jsonl \
        $DS/*_Win64_train.jsonl \
        $DS/*_Dot_Net_train.jsonl \
    --inputs-test \
        $DS/*_Win32_test.jsonl \
        $DS/*_Win64_test.jsonl \
        $DS/*_Dot_Net_test.jsonl \
    --output ../data/metapath2vec.sqlite3
```

Testing arguments beforehand:

```sh
bash -c 'echo "$@"' -- \
    --inputs-train \
        $DS/*_Win32_train.jsonl \
        $DS/*_Win64_train.jsonl \
        $DS/*_Dot_Net_train.jsonl \
    --inputs-test \
        $DS/*_Win32_test.jsonl \
        $DS/*_Win64_test.jsonl \
        $DS/*_Dot_Net_test.jsonl \
    --output ../data/metapath2vec.sqlite3
```
