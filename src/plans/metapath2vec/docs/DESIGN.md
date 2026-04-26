# Design

## 1. Problem Statement

**Goal:**

- Given malware samples represented by:
  - C: capa capabilities
  - B: MBC behaviors (MBC = Malware Behavior Catalog)
- Learn a function: $(C,B) → z ∈ R^d$
- Such that:
  - similar behavioral patterns → nearby vectors
  - different operational goals → separable clusters
- Then fits into pipeline: `z → clusters → Intent`

**Key idea:** Learn intent as a latent structure via graph embeddings + clustering

## 2. Data Representation

Inputs per sample $s_i$

From EMBER/capa:

- $C_i = \{c_1, c_2, ...\}$
- $B_i = \{b_1, b_2 ,...\}$

## 3. Graph Construction

**Node types:** Define 3 disjoint sets:
1. $S$: sample nodes
2. $C$: capability nodes
3. $B$: behavior nodes

**Edges** Undirected graph, contains:

1. Sample-Capability edges: $(s_i, c_j)$ if $c_j \in C_i$
2. Sample-Behavior edges: $(s_i, b_k)$ if $b_k \in B_i$

No edges between:
- $C ↔ B$
- $S ↔ S$

**Graph summary:** $G = (V, E)$ where:
- $V = S \cup C \cup B$
- $E = E_{SC} \cup E_{SB}$

## 4. Embedding Method: metapath2vec

**Output:** For each node $v ∈ V$:

$h_v \in R^d$

But only extract embeddings for nodes that
are samples, i.e. for $v = s_i \in S$ we extract
$z = h_v = h_{s_i} \in R^d$ - one vector
per sample.

## 5. Clustering Method: HDBSCAN

For each cluster:
1. Aggregate: What are the most frequent capabilities, most frequent behaviors?
2. Inspect Patterns: Take the top K most frequent C and B, then label the likely intent of each cluster.

## 6. The Inference Pipeline in Action

    Sample
    --(metapath2vec)--
        --> Graph Embedding Vector
    --(Gaussian Mixture Model (GMM))--
        --> Vector of Cluster Probabilities (VCP)
    --(Logistic Regression)--
        --> Binary label (explainable via VCP)
