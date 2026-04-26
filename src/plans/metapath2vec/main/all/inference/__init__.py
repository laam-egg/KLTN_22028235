import numpy as np

def infer(sample_id, emb_model, gmm, lr):
    key = f"S:{sample_id}"
    if key not in emb_model.wv:
        return None, None

    z = emb_model.wv[key].reshape(1, -1)

    vcp = gmm.predict_proba(z)[0]
    score = lr.predict_proba(vcp.reshape(1, -1))[0,1]

    verdict = 1 if score > 0.5 else 0

    top_clusters = np.argsort(vcp)[::-1][:3]

    explanation = {
        "score": float(score),
        "top_clusters": top_clusters.tolist(),
        "cluster_probs": vcp[top_clusters].tolist()
    }

    return verdict, explanation
