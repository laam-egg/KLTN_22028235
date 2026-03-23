from thrember import PEFeatureExtractor
from typing import Literal, override
from pathlib import Path
import numpy as np
import os

class PEAndOneHotCapaFeatureExtractor(PEFeatureExtractor):
    def __init__(self, capa_features: dict[str, int]):
        super().__init__()
        self.capa_features = capa_features
        self.extended_dim = len(self.capa_features)
        self.dim += self.extended_dim

    @override
    def process_raw_features(self, raw_obj):
        feature_vectors = [fe.process_raw_features(raw_obj[fe.name]) for fe in self.features]
        extension_vector = np.zeros(self.extended_dim)
        feature_vectors.append(extension_vector)

        caps = [x['Namespace'] + ':' + x['Capability'] for x in raw_obj['caps']]
        ttps = [x['Tactic'] + ':' + x['Technique'] for x in raw_obj['ttps']]
        mbcs = [x['Objective'] + ':' + x['Behavior'] for x in raw_obj['mbc']]
        for x in (*caps, *ttps, *mbcs):
            index = self.capa_features.get(x, -1)
            if index >= 0:
                extension_vector[index] = 1

        return np.hstack(feature_vectors).astype(np.float32)

def is_JSONL_PE(jsonl_path: Path | str) -> bool:
    jsonl_path = Path(jsonl_path)
    if any(
        x in str(jsonl_path).lower()
        for x in ('dot_net', 'win32', 'win64')
    ):
        return True
    return False

def read_vectorized_features(
    data_dir: Path | str,
    ndim: int,
    subset: Literal['train'] | Literal['test'] | Literal['chal'],
) -> tuple[np.ndarray, np.ndarray]:
    """
    Read vectorized features into memory mapped numpy arrays
    """
    data_path: Path = Path(data_dir)
    X_path = data_path / f"X_{subset}_onehotcapa.dat"
    y_path = data_path / f"y_{subset}_onehotcapa.dat"

    if not os.path.isfile(X_path):
        raise ValueError(f"Invalid subset file: {X_path}")
    if not os.path.isfile(y_path):
        raise ValueError(f"Invalid subset file: {y_path}")

    X = np.memmap(X_path, dtype=np.float32, mode="r")
    # X = np.array(X).reshape(-1, ndim)
    X.shape = (-1, ndim)
    N: int = X.shape[0]
    y = np.memmap(y_path, dtype=np.int32, mode="r")
    y = np.array(y)
    if y.shape[0] > N:
        y = y.reshape(N, -1)

    return X, y

def read_vectorized_features_EMBER2024(
    data_dir: Path | str,
    subset: Literal['train'] | Literal['test'] | Literal['chal'],
) -> tuple[np.ndarray, np.ndarray]:
    """
    Read vectorized features into memory mapped numpy arrays
    """
    data_path: Path = Path(data_dir)
    X_path = data_path / f"X_{subset}.dat"
    y_path = data_path / f"y_{subset}.dat"

    if not os.path.isfile(X_path):
        raise ValueError(f"Invalid subset file: {X_path}")
    if not os.path.isfile(y_path):
        raise ValueError(f"Invalid subset file: {y_path}")

    from thrember import PEFeatureExtractor
    ndim = PEFeatureExtractor().dim

    X = np.memmap(X_path, dtype=np.float32, mode="r")
    # X = np.array(X).reshape(-1, ndim)
    X.shape = (-1, ndim)
    N: int = X.shape[0]
    y = np.memmap(y_path, dtype=np.int32, mode="r")
    y = np.array(y)
    if y.shape[0] > N:
        y = y.reshape(N, -1)

    return X, y

import numpy as np

def hstack_memmap(A, B, out_path, dtype=np.float32, chunk_size=10_000):
    """
    Horizontally stack two arrays A and B using memmap to avoid RAM spikes.
    Returns the resulting memmap array.
    """
    assert A.shape[0] == B.shape[0], "A and B must have the same number of rows"
    
    N   = A.shape[0]
    DIM = A.shape[1] + B.shape[1]
    
    out = np.memmap(out_path, dtype=dtype, mode="w+", shape=(N, DIM))
    
    for start in range(0, N, chunk_size):
        end = min(start + chunk_size, N)
        out[start:end, :A.shape[1]] = A[start:end]
        out[start:end, A.shape[1]:] = B[start:end]
    
    out.flush()
    return np.memmap(out_path, dtype=dtype, mode="r", shape=(N, DIM))
