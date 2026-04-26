import numpy as np

def get_Z(model, samples):
    Z = []
    valid_samples = []

    for s in samples:
        key = f"S:{s}"
        if key in model.wv:
            Z.append(model.wv[key])
            valid_samples.append(s)

    return np.array(Z), valid_samples
