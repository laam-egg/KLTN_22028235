import numpy as np

def subsample(Z, y, max_samples=200000):
    if len(Z) <= max_samples:
        return Z, y

    idx = np.random.choice(len(Z), max_samples, replace=False)
    return Z[idx], y[idx]
