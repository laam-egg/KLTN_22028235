import random
from collections import defaultdict
from gensim.models import Word2Vec
from typing import Any

# =========================
# 1. BUILD GRAPH
# =========================

class MetaPathGraph:
    def __init__(self):
        self.adj = defaultdict(list)
        self.node_type = {}  # node_id -> "S" / "C" / "B"

    def add_node(self, node, ntype):
        self.node_type[node] = ntype

    def add_edge(self, u, v):
        self.adj[u].append(v)
        self.adj[v].append(u)


def build_graph(
    samples: list[Any],
    sample_to_caps: dict[list[Any]],
    sample_to_behaviors: dict[list[Any]],
):
    g = MetaPathGraph()

    for s in samples:
        s_node = f"S:{s}"
        g.add_node(s_node, "S")

        # --- capabilities ---
        for c in sample_to_caps[s]:
            c_node = f"C:{c}"
            if c_node not in g.node_type:
                g.add_node(c_node, "C")
            g.add_edge(s_node, c_node)

        # --- behaviors ---
        for b in sample_to_behaviors[s]:
            b_node = f"B:{b}"
            if b_node not in g.node_type:
                g.add_node(b_node, "B")
            g.add_edge(s_node, b_node)

    return g


# =========================
# 2. META-PATH WALK
# =========================

def metapath_walk(graph, start_node, metapath, walk_length):
    """
    start_node: must be an "S" node
    metapath: e.g. ["S", "C", "S"]
    """
    walk = [start_node]
    current = start_node

    for step in range(1, walk_length):
        expected_type = metapath[step % len(metapath)]

        neighbors = graph.adj[current]

        # filter by required node type
        candidates = [
            n for n in neighbors
            if graph.node_type[n] == expected_type
        ]

        if not candidates:
            break

        current = random.choice(candidates)
        walk.append(current)

    return walk

def metapath_walk_S_to_S_only(graph, start_node, metapath, walk_length):
    walk = [start_node]          # only S nodes stored
    current = start_node

    for step in range(1, walk_length * 2):  
        # *2 because we skip C/B in output

        expected_type = metapath[step % len(metapath)]

        neighbors = graph.adj[current]

        candidates = [
            n for n in neighbors
            if graph.node_type[n] == expected_type
        ]

        if not candidates:
            break

        current = random.choice(candidates)

        # only keep S nodes
        if graph.node_type[current] == "S":
            walk.append(current)
            if len(walk) >= walk_length:
                break

    return walk

# =========================
# 3. GENERATE WALKS
# =========================

def generate_walks(
    graph,
    samples,
    num_walks,
    walk_length,
    metapath_walker, # either metapath_walk or metapath_walk_S_to_S_only
):
    walks = []

    # Define meta-paths
    METAPATHS = [
        ["S", "C", "S"],  # capability similarity
        ["S", "B", "S"],  # behavior similarity
    ]

    for s in samples:
        start_node = f"S:{s}"

        for _ in range(num_walks):
            for mp in METAPATHS:
                walk = metapath_walker(graph, start_node, mp, walk_length)
                walks.append(walk)

    return walks


# =========================
# 4. TRAIN EMBEDDING
# =========================

def train_embedding(walks, dim=128, window=5):
    """
    YOU can tune:
        dim
        window
    """
    # Word2Vec expects strings
    walks_str = [[str(n) for n in walk] for walk in walks]

    model = Word2Vec(
        sentences=walks_str,
        vector_size=dim,
        window=window,
        sg=1,            # skip-gram
        negative=10,
        min_count=1,
        workers=8,
        epochs=5,
        sample=0 # 1e-3      # VERY IMPORTANT (downsample frequent nodes)
    )

    return model


# =========================
# 5. EXTRACT SAMPLE EMBEDDINGS
# =========================

def get_sample_embeddings(model, samples):
    """
    Returns:
        dict: sample_id -> embedding vector
    """
    Z = {}

    for s in samples:
        node = f"S:{s}"
        Z[s] = model.wv[node]

    return Z


# =========================
# ======= USAGE ===========
# =========================

samples = list(range(1, 11))

sample_to_caps = {
    1: ["A", "B"],
    2: ["A", "B"],
    3: ["A", "C"],

    4: ["D", "E"],
    5: ["D", "E"],
    6: ["D"],

    7: ["F"],
    8: ["F", "C"],

    9: ["A", "D"],   # mixed / bridge
    10: []           # edge case (no caps)
}

sample_to_behaviors = {
    1: ["p", "q"],
    2: ["p", "q"],
    3: ["p"],

    4: ["r", "s"],
    5: ["r", "s"],
    6: ["s"],

    7: ["t"],
    8: ["t", "p"],

    9: ["q", "r"],   # mixed
    10: ["q"]        # weak signal
}

# 1. Build graph
graph = build_graph(samples, sample_to_caps, sample_to_behaviors)

# 2. Generate walks
walks = generate_walks(graph, samples, num_walks=100, walk_length=40, metapath_walker=metapath_walk_S_to_S_only)

# 3. Train embedding
model = train_embedding(walks, dim=128)

# 4. Extract embeddings
# Z = get_sample_embeddings(model, samples)

# 5. Sanity checks
print(1, model.wv.most_similar("S:1", topn=5)) # top results should be S:2, S:3
print(2, model.wv.similarity("S:1", "S:2")) # high
print(3, model.wv.similarity("S:1", "S:4")) # low
print(4, model.wv.most_similar("S:9", topn=5)) # both S:1, S:2 (cluster A) and S:4, S:5 (cluster B)
print(5, model.wv.similarity("S:3", "S:8")) # moderately high
print(6, model.wv.most_similar("S:10")) # unstable neighbors, lower similarity scores

from sklearn.decomposition import PCA
import matplotlib.pyplot as plt

Z = [model.wv[f"S:{s}"] for s in samples]

Z2 = PCA(n_components=2).fit_transform(Z)

for i, (x, y) in enumerate(Z2):
    plt.scatter(x, y)
    plt.text(x, y, str(samples[i]))

plt.show()
