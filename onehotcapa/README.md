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

## Training and Evaluation Results

|       Iteration        | EMBER2024                                                 | EMBER2024+Capa1                |
| :--------------------: | --------------------------------------------------------- | ------------------------------ |
|          100           | ROC-AUC = 0.991358                                        | ROC-AUC = 0.991191             |
|          200           | ROC-AUC = 0.992569                                        | ROC-AUC = 0.992588             |
|          300           | ROC-AUC = 0.992794                                        | ROC-AUC = 0.992931             |
|          400           | (Early stopped at iteration 251, with ROC-AUC = 0.992813) | ROC-AUC = 0.993129             |
|          500           |                                                           | ROC-AUC = 0.993244             |
| Evaluation on test set | Partial AUC (FPR≤0.5%): 0.8816                            | Partial AUC (FPR≤0.5%): 0.8832 |
|                        | Full AUC: 0.9922                                          | Full AUC: 0.9927               |

(with `lgb.early_stopping(50)` - stop training if validation score doesn't improve for 50 consecutive rounds)
