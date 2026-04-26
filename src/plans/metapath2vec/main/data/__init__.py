"""
USAGE

1. Load: 
    (
        samples,
        sample_to_caps,
        sample_to_behaviors,
        sample_to_tactics,
        labels
    ) = load_dataset(
        "dataset.db",
        split="train"
    )

2. Query:

    for sid in samples:

        caps = sample_to_caps[sid]
        behs = sample_to_behaviors[sid]
        label = labels[sid]
"""

import sqlite3
from collections import OrderedDict


# ============================================================
# Simple LRU Cache
# ============================================================

import sqlite3

from functools import lru_cache


# ============================================================
# Samples iterable
# ============================================================

class SamplesIterable:
    def __init__(self, conn, split=None):
        self.conn = conn
        self.split = split

    def __iter__(self):

        cur = self.conn.cursor()

        if self.split is None:

            query = """
                SELECT sample_id
                FROM samples
                ORDER BY sample_id
                LIMIT 1000000
            """ # TODO

            for (sid,) in cur.execute(query):
                yield sid

        else:

            query = """
                SELECT sample_id
                FROM samples
                WHERE split = ?
                ORDER BY sample_id
                LIMIT 1000000
            """ # TODO

            for (sid,) in cur.execute(
                query,
                (self.split,)
            ):
                yield sid


# ============================================================
# Labels proxy
# ============================================================

class LabelsProxy:
    def __init__(self, conn, cache_size=10000):

        self.conn = conn
        self.cur = conn.cursor()

        self._get_label = lru_cache(
            maxsize=cache_size
        )(self._get_label_uncached)

    def _get_label_uncached(self, sample_id):

        row = self.cur.execute("""
            SELECT label
            FROM samples
            WHERE sample_id = ?
        """, (sample_id,)).fetchone()

        if row is None:
            raise KeyError(sample_id)

        return row[0]

    def __getitem__(self, sample_id):
        return self._get_label(sample_id)


# ============================================================
# Caps proxy
# ============================================================

class CapsProxy:
    def __init__(self, conn, cache_size=10000):

        self.conn = conn
        self.cur = conn.cursor()

        self._get_caps = lru_cache(
            maxsize=cache_size
        )(self._get_caps_uncached)

    def _get_caps_uncached(self, sample_id):

        rows = self.cur.execute("""
            SELECT cap_id
            FROM sample_caps
            WHERE sample_id = ?
        """, (sample_id,)).fetchall()

        return [r[0] for r in rows]

    def __getitem__(self, sample_id):
        return self._get_caps(sample_id)


# ============================================================
# Behaviors proxy
# ============================================================

class BehaviorsProxy:
    def __init__(self, conn, cache_size=10000):

        self.conn = conn
        self.cur = conn.cursor()

        self._get_behaviors = lru_cache(
            maxsize=cache_size
        )(self._get_behaviors_uncached)

    def _get_behaviors_uncached(self, sample_id):

        rows = self.cur.execute("""
            SELECT behavior_id
            FROM sample_behaviors
            WHERE sample_id = ?
        """, (sample_id,)).fetchall()

        return [r[0] for r in rows]

    def __getitem__(self, sample_id):
        return self._get_behaviors(sample_id)


# ============================================================
# Tactics proxy
# ============================================================

class TacticsProxy:
    def __init__(self, conn, cache_size=10000):

        self.conn = conn
        self.cur = conn.cursor()

        self._get_tactics = lru_cache(
            maxsize=cache_size
        )(self._get_tactics_uncached)

    def _get_tactics_uncached(self, sample_id):

        rows = self.cur.execute("""
            SELECT tactic_id
            FROM sample_tactics
            WHERE sample_id = ?
        """, (sample_id,)).fetchall()

        return [r[0] for r in rows]

    def __getitem__(self, sample_id):
        return self._get_tactics(sample_id)


# ============================================================
# Public API
# ============================================================

def load_dataset(
    db_path,
    split=None,
    cache_size=10000
):
    """
    Returns:

        samples
        sample_to_caps
        sample_to_behaviors
        sample_to_tactics
        labels
    """

    conn = sqlite3.connect(db_path)

    samples = SamplesIterable(
        conn=conn,
        split=split
    )

    sample_to_caps = CapsProxy(
        conn=conn,
        cache_size=cache_size
    )

    sample_to_behaviors = BehaviorsProxy(
        conn=conn,
        cache_size=cache_size
    )

    sample_to_tactics = TacticsProxy(
        conn=conn,
        cache_size=cache_size
    )

    labels = LabelsProxy(
        conn=conn,
        cache_size=cache_size
    )

    return (
        samples,
        sample_to_caps,
        sample_to_behaviors,
        sample_to_tactics,
        labels
    )
