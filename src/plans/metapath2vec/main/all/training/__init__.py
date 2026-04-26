from ...common.graph import build_graph
from ...metapath2vec.training import train_embedding
from ...metapath2vec.inference import get_Z

from ...gmm.common import subsample
from ...gmm.training import train_gmm
from ...gmm.inference import get_vcp

from ...lr.training import train_lr

import numpy as np
from tqdm import tqdm

def train_pipeline(BUILD, samples, sample_to_caps, sample_to_behaviors, labels):
    if BUILD:
        print("Building graph...")
        graph = build_graph(samples, sample_to_caps, sample_to_behaviors)

        from ...common.walk_generator import WalkIterator
        with open("walks.txt", "w") as f:
            for walk in tqdm(WalkIterator(graph, samples)):
                f.write(" ".join(walk) + "\n")
        import sys
        sys.exit(0)
        return
    # import time
    # t0 = time.time()
    # walk = next(iter(walks))
    # print(walk)
    # print(time.time() - t0)
    # return 0

    from gensim.models.word2vec import LineSentence
    sentences = LineSentence("walks.txt")

    print("Training metapath2vec...")
    emb_model = train_embedding(sentences)

    print("Postprocessing metapath2vec...")
    Z, valid_samples = get_Z(emb_model, samples)
    y = np.array([labels[s] for s in valid_samples])

    print("Subsampling for GMM...")
    Z_sub, y_sub = subsample(Z, y)

    print("Training GMM...")
    gmm = train_gmm(Z_sub)

    print("Postprocessing GMM...")
    VCP = get_vcp(gmm, Z)

    print("Training LR...")
    lr = train_lr(VCP, y)

    print("Done training.")
    return emb_model, gmm, lr
