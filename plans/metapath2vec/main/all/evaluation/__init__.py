from sklearn.metrics import roc_auc_score, f1_score, precision_score, recall_score, confusion_matrix
import random
from ..inference import infer

def evaluate(samples, emb_model, gmm, lr, labels, max_eval=200000):
    import numpy as np

    eval_samples = samples
    # TODO
    # if len(samples) > max_eval:
    #     eval_samples = random.sample(list(samples), max_eval)

    y_true, y_pred, y_score = [], [], []

    for s in eval_samples:
        res = infer(s, emb_model, gmm, lr)
        if res[0] is None:
            continue

        verdict, expl = res

        y_true.append(labels[s])
        y_pred.append(verdict)
        y_score.append(expl["score"])

    y_true = np.array(y_true)
    y_pred = np.array(y_pred)
    y_score = np.array(y_score)

    roc = roc_auc_score(y_true, y_score)
    f1 = f1_score(y_true, y_pred)
    prec = precision_score(y_true, y_pred)
    rec = recall_score(y_true, y_pred)

    tn, fp, fn, tp = confusion_matrix(y_true, y_pred).ravel()
    fpr = fp / (fp + tn + 1e-8)

    return {
        "ROC-AUC": roc,
        "F1": f1,
        "Precision": prec,
        "Recall": rec,
        "FPR": fpr
    }
