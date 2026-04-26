from collections import defaultdict
from tqdm import tqdm

class Graph:
    def __init__(self):
        self.adj = defaultdict(list)
        self.node_type = {}

    def add_node(self, n, t):
        self.node_type[n] = t

    def add_edge(self, u, v):
        self.adj[u].append(v)
        self.adj[v].append(u)


def build_graph(samples, sample_to_caps, sample_to_behaviors):
    g = Graph()

    for s in tqdm(samples):
        s_node = f"S:{s}"
        g.add_node(s_node, "S")

        for c in set(sample_to_caps[s]):
            c_node = f"C:{c}"
            if c_node not in g.node_type:
                g.add_node(c_node, "C")
            g.add_edge(s_node, c_node)

        for b in set(sample_to_behaviors[s]):
            b_node = f"B:{b}"
            if b_node not in g.node_type:
                g.add_node(b_node, "B")
            g.add_edge(s_node, b_node)

    return g
