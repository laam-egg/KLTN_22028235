from pathlib import Path
import sys
DIR = Path(__file__).resolve().parent
sys.path.append(DIR)
from common_utils import hstack_memmap

from thrember import PEFeatureExtractor

import lightgbm as lgb
import json
from typing import Callable
import numpy as np
import os

# See thrember's model.py, function train_model
EMBER_CATEGORICAL = [2, 3, 4, 5, 6, 701, 702]

def l(*args):
    print(*args)

def train_LGBM(
    data_dir: Path | str,
    vector_reader: Callable[[Path|str, str], tuple[np.ndarray, np.ndarray]],
    lgbm_config_path: Path | str,
    model_save_path: Path | str,
):
    l("Loading LGBM config/best params...")
    with open(lgbm_config_path, 'r', encoding='utf-8') as f:
        best_params = json.load(f)
    
    l("Loading original train set...")
    data_path = Path(data_dir)
    X_train_original, y_train_original = vector_reader(data_path, 'train')

    l("Splitting original train set into train and val sets...")
    # 90/10 split for early stopping validation
    def split_fast(X_train_original, y_train_original):
        n = len(X_train_original)
        val_size = int(n * 0.1)
        train_size = n - val_size

        X_train = X_train_original[:train_size]
        X_val   = X_train_original[train_size:]
        y_train = y_train_original[:train_size]
        y_val   = y_train_original[train_size:]

        return X_train, X_val, y_train, y_val
    
    X_train, X_val, y_train, y_val = split_fast(X_train_original, y_train_original)
    X_train = X_train[:450_000]
    X_val   = X_val[:50_000]
    y_train = y_train[:450_000]
    y_val   = y_val[:50_000]

    l("Establishing LGBM-specific train and val sets...")
    train_set = lgb.Dataset(X_train, y_train, categorical_feature=EMBER_CATEGORICAL)
    val_set = lgb.Dataset(X_val, y_val, reference=train_set, categorical_feature=EMBER_CATEGORICAL)

    l("Establishing training params...")
    final_params = {
        **best_params,
        "objective": "binary",
        "metric": "auc",
        "verbosity": -1,
    }

    l("Training...")
    model = lgb.train(
        final_params,
        train_set,
        valid_sets=[val_set],
        callbacks=[lgb.early_stopping(50), lgb.log_evaluation(100)],
    )

    l(f"Saving model to: {model_save_path}...")
    model.save_model(model_save_path, num_iteration=model.best_iteration)

    l("Loading test set...")
    X_test, y_test = vector_reader(data_path, 'test')

    l("Evaluating model on test set...")
    y_prob = model.predict(X_test)
    from sklearn.metrics import roc_auc_score
    pauc = roc_auc_score(y_test, y_prob, max_fpr=5e-3)
    full_auc = roc_auc_score(y_test, y_prob)
    print(f"Partial AUC (FPR≤0.5%): {pauc:.4f}")
    print(f"Full AUC:                {full_auc:.4f}")

def train_LGBM_EMBER2024(
    data_dir: Path | str,
    lgbm_config_path: Path | str,
    model_save_path: Path | str,
):
    from common_utils import read_vectorized_features_EMBER2024
    train_LGBM(
        data_dir=data_dir,
        vector_reader=read_vectorized_features_EMBER2024,
        lgbm_config_path=lgbm_config_path,
        model_save_path=model_save_path,
    )

def train_LGBM_EMBER2024_and_OneHotCapa(
    data_dir: Path | str,
    capa_features_path: Path | str,
    lgbm_config_path: Path | str,
    model_save_path: Path | str,
):
    from common_utils import read_vectorized_features, PEAndOneHotCapaFeatureExtractor
    l("Loading Capa features...")
    with open(capa_features_path, 'r', encoding='utf-8') as f:
        capa_features = json.load(f)
    extractor = PEAndOneHotCapaFeatureExtractor(capa_features=capa_features)
    ndim: int = extractor.dim

    train_LGBM(
        data_dir=data_dir,
        vector_reader=lambda data_dir, split: read_vectorized_features(data_dir, ndim, split),
        lgbm_config_path=lgbm_config_path,
        model_save_path=model_save_path,
    )

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description="Train LGBM")
    parser.add_argument("type", help="'EMBER2024', or 'EMBER2024+Capa1'")
    parser.add_argument("data_dir", help="Path to EMBER2024 JSONL dataset/directory containing extracted .dat files")
    parser.add_argument("--capa-features-path", help="Path to enumerated Capa features file (JSON)")
    parser.add_argument("lgbm_config_path", help="Path to LGBM config file (JSON)")
    parser.add_argument("model_save_path", help="Path to save trained model")
    args = parser.parse_args()

    t = args.type
    if t == 'EMBER2024':
        train_LGBM_EMBER2024(
            data_dir=args.data_dir,
            lgbm_config_path=args.lgbm_config_path,
            model_save_path=args.model_save_path,
        )
    elif t == 'EMBER2024+Capa1':
        if not args.capa_features_path:
            raise RuntimeError("No capa features path specified")
        train_LGBM_EMBER2024_and_OneHotCapa(
            data_dir=args.data_dir,
            capa_features_path=args.capa_features_path,
            lgbm_config_path=args.lgbm_config_path,
            model_save_path=args.model_save_path,
        )
    else:
        print(f"Invalid training type: {t}")
        sys.exit(1)
