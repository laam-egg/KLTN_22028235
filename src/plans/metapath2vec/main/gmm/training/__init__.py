from sklearn.mixture import GaussianMixture

def train_gmm(Z, n_components=30):
    gmm = GaussianMixture(
        n_components=n_components,
        covariance_type="diag",   # CRITICAL for scale
        max_iter=100
    )
    gmm.fit(Z)
    return gmm
