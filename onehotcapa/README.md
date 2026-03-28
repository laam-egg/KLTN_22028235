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

## Training

### Train on original EMBER2024

```sh
python train_lgbm.py EMBER2024 $DS ./lgbm_config.json ./EMBER2024.model
```

### Train on EMBER2024 + one-hot Capa

```sh
python train_lgbm.py EMBER2024+Capa1 $DS ./lgbm_config.json ./EMBER2024+Capa1.model --capa-features-path capa_features.json
```

### Train on EMBER2024 + Truncated SVD (components = 64)

```sh
python train_lgbm.py EMBER2024+CapaTruncatedSVD $DS ./lgbm_config.json ./EMBER2024+CapaTruncatedSVD.model --capa-features-path capa_features.json
```

### Train on EMBER2024 + Truncated SVD (components = 128)

```sh
python train_lgbm.py EMBER2024+CapaTruncatedSVD128 $DS ./lgbm_config.json ./EMBER2024+CapaTruncatedSVD128.model --capa-features-path capa_features.json
```

## Training and Evaluation Results

|       Iteration        | EMBER2024                                                 | EMBER2024+Capa1                | EMBER2024+CapaTruncatedSVD (64) | EMBER2024+CapaTruncatedSVD128                             |
| :--------------------: | --------------------------------------------------------- | ------------------------------ | ------------------------------- | --------------------------------------------------------- |
|          100           | ROC-AUC = 0.991358                                        | ROC-AUC = 0.991224             | ROC-AUC = 0.991288              | ROC-AUC = 0.991317                                        |
|          200           | ROC-AUC = 0.992569                                        | ROC-AUC = 0.992687             | ROC-AUC = 0.992706              | ROC-AUC = 0.992782                                        |
|          300           | ROC-AUC = 0.992794                                        | ROC-AUC = 0.993042             | ROC-AUC = 0.992997              | ROC-AUC = 0.993162                                        |
|          400           | (Early stopped at iteration 251, with ROC-AUC = 0.992813) | ROC-AUC = 0.993130             | ROC-AUC = 0.993082              | ROC-AUC = 0.993283                                        |
|          500           |                                                           | ROC-AUC = 0.993251             | ROC-AUC = 0.993194              | (Early stopped at iteration 446, with ROC-AUC = 0.993314) |
| Evaluation on test set | Partial AUC (FPR≤0.5%): 0.8816                            | Partial AUC (FPR≤0.5%): 0.8843 | Partial AUC (FPR≤0.5%): 0.8830  | Partial AUC (FPR≤0.5%): 0.8838                            |
|                        | Full AUC: 0.9922                                          | Full AUC: 0.9927               | Full AUC: 0.9927                | Full AUC: 0.9925                                          |

(with `lgb.early_stopping(50)` - stop training if validation score doesn't improve for 50 consecutive rounds)
