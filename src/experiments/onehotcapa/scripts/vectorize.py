from pathlib import Path
import sys
DIR = Path(__file__).resolve().parent
sys.path.append(DIR)
from common_utils import is_JSONL_PE, PEAndOneHotCapaFeatureExtractor

import json
from tqdm import tqdm

from thrember.model import vectorize_subset, gather_feature_paths

# Source - https://stackoverflow.com/a/27518377
# Posted by Michael Bacon, modified by community. See post 'Timeline' for change history
# Retrieved 2026-05-17, License - CC BY-SA 4.0

from itertools import (takewhile, repeat)

def rawincount(filename):
    f = open(filename, 'rb')
    bufgen = takewhile(lambda x: x, (f.raw.read(1024*1024) for _ in repeat(None)))
    return sum(buf.count(b'\n') for buf in bufgen)
#####################################################

def create_vectorized_features_PE(data_dir: Path | str, capa_features_path: Path | str) -> None:
    with open(capa_features_path, 'r', encoding='utf-8') as f:
        capa_features = json.load(f)
    extractor = PEAndOneHotCapaFeatureExtractor(capa_features)
    data_path: Path = Path(data_dir)

    # Map string labels/tags to numeric labels
    label_map = {}
    label_type = 'label' # binary: 0, 1


    print("Preparing to vectorize raw features - TRAIN")
    X_train_path = data_path / "X_train_onehotcapa.dat"
    y_train_path = data_path / "y_train_onehotcapa.dat"
    train_feature_paths = list(filter(
        is_JSONL_PE,
        gather_feature_paths(data_path, "train"),
    ))
    train_nrows = sum(rawincount(fp) for fp in tqdm(train_feature_paths))

    print("Vectorizing training set")
    vectorize_subset(X_train_path, y_train_path, train_feature_paths, extractor, train_nrows, label_type, label_map)

    print("Preparing to vectorize raw features - TEST")

    X_test_path = data_path / "X_test_onehotcapa.dat"
    y_test_path = data_path / "y_test_onehotcapa.dat"
    test_feature_paths = list(filter(
        is_JSONL_PE,
        gather_feature_paths(data_path, "test"),
    ))
    # test_nrows = sum(1 for fp in tqdm(test_feature_paths) for _ in fp.open('rb'))
    test_nrows = sum(rawincount(fp) for fp in tqdm(test_feature_paths))

    print("Vectorizing test set")
    vectorize_subset(X_test_path, y_test_path, test_feature_paths, extractor, test_nrows, label_type, label_map)

    print("Preparing to vectorize raw features - CHAL")
    X_chal_path = data_path / "X_chal_onehotcapa.dat"
    y_chal_path = data_path / "y_chal_onehotcapa.dat"
    chal_feature_paths = filter(
        is_JSONL_PE,
        gather_feature_paths(data_path, "challenge"),
    )
    # chal_nrows = sum([1 for fp in chal_feature_paths for _ in fp.open()])
    chal_nrows = sum(rawincount(fp) for fp in tqdm(chal_feature_paths))
    print("Vectorizing challenge set")
    vectorize_subset(X_chal_path, y_chal_path, chal_feature_paths, extractor, chal_nrows, label_type, label_map)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description="Vectorize EMBER2024 with one-hot-encoded Capa features")
    parser.add_argument("data_dir", help="Path to EMBER2024 JSONL dataset")
    parser.add_argument("capa_features_path", help="Path to enumerated Capa features file (JSON)")
    args = parser.parse_args()

    create_vectorized_features_PE(
        data_dir=args.data_dir,
        capa_features_path=args.capa_features_path,
    )
