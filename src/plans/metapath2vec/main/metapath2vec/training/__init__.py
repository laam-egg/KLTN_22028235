from gensim.models import Word2Vec

from ...common.walk_generator import WalkIterator

def train_embedding(sentences, dim=128):
    model = Word2Vec(
        sentences=sentences,
        vector_size=dim,
        window=5,
        sg=1,
        min_count=1,
        workers=8, # 1
        epochs=20,
        negative=10,
        sample=1e-5,
    )
    return model
