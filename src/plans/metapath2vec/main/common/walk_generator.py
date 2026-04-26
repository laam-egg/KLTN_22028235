import random

class WalkIterator:
    def __init__(self, graph, samples, num_walks=20, walk_length=20):
        self.graph = graph
        self.samples = list(samples)
        self.num_walks = num_walks
        self.walk_length = walk_length
        self.metapaths = [["S","C","S"], ["S","B","S"]]

    def __iter__(self):
        for s in self.samples:
            start = f"S:{s}"

            for _ in range(self.num_walks):
                for mp in self.metapaths:
                    yield self._walk(start, mp)

    def _walk(self, start_node, metapath):
        walk = [start_node]
        current = start_node

        path_idx = 0

        for _ in range(self.walk_length * len(metapath)):
            expected_type = metapath[(path_idx + 1) % len(metapath)]

            candidates = [
                n for n in self.graph.adj[current]
                if self.graph.node_type[n] == expected_type
            ]

            if not candidates:
                break

            current = random.choice(candidates)

            walk.append(current)

            path_idx = (path_idx + 1) % len(metapath)

        return walk
