#!/usr/bin/env python3

import argparse
import json
import sqlite3
from pathlib import Path

from tqdm import tqdm


# ============================================================
# Helpers
# ============================================================

def normalize(s: str) -> str:
    return s.strip()


def load_vocab(cur, table_name):
    """
    Load existing vocabulary from DB.

    Returns:
        mapping: dict[name] -> id
        next_id: next available integer ID
    """

    mapping = {}

    rows = cur.execute(
        f"SELECT id, name FROM {table_name}"
    ).fetchall()

    max_id = -1

    for idx, name in rows:
        mapping[name] = idx
        max_id = max(max_id, idx)

    return mapping, max_id + 1


def get_or_create_id(mapping, value, next_id_ref):
    """
    next_id_ref = [current_next_id]
    (mutable wrapper to emulate pass-by-reference)
    """

    if value not in mapping:
        mapping[value] = next_id_ref[0]
        next_id_ref[0] += 1

    return mapping[value]


# ============================================================
# DB Setup
# ============================================================

def create_tables(cur):
    cur.executescript("""
    CREATE TABLE IF NOT EXISTS samples (
        sample_id TEXT PRIMARY KEY,
        label INTEGER,
        split TEXT
    );

    CREATE TABLE IF NOT EXISTS caps_vocab (
        id INTEGER PRIMARY KEY,
        name TEXT UNIQUE
    );

    CREATE TABLE IF NOT EXISTS behaviors_vocab (
        id INTEGER PRIMARY KEY,
        name TEXT UNIQUE
    );

    CREATE TABLE IF NOT EXISTS tactics_vocab (
        id INTEGER PRIMARY KEY,
        name TEXT UNIQUE
    );

    CREATE TABLE IF NOT EXISTS sample_caps (
        sample_id TEXT,
        cap_id INTEGER
    );

    CREATE TABLE IF NOT EXISTS sample_behaviors (
        sample_id TEXT,
        behavior_id INTEGER
    );

    CREATE TABLE IF NOT EXISTS sample_tactics (
        sample_id TEXT,
        tactic_id INTEGER
    );
    """)


def create_indexes(cur):
    cur.executescript("""
    CREATE INDEX IF NOT EXISTS idx_samples_label
    ON samples(label);

    CREATE INDEX IF NOT EXISTS idx_samples_split
    ON samples(split);

    CREATE INDEX IF NOT EXISTS idx_sample_caps_sid
    ON sample_caps(sample_id);

    CREATE INDEX IF NOT EXISTS idx_sample_behaviors_sid
    ON sample_behaviors(sample_id);

    CREATE INDEX IF NOT EXISTS idx_sample_tactics_sid
    ON sample_tactics(sample_id);
    """)


# ============================================================
# Ingestion
# ============================================================

def process_file(
    path,
    split,
    cur,
    conn,
    batch_size,

    cap_to_id,
    behavior_to_id,
    tactic_to_id,

    next_cap_id,
    next_behavior_id,
    next_tactic_id,
):
    total = 0
    skipped = 0

    print(f"\nProcessing [{split}]: {path}")

    with open(path, "r") as f:
        for line in tqdm(f):

            # ----------------------------
            # Parse JSON
            # ----------------------------
            try:
                obj = json.loads(line)
            except Exception:
                skipped += 1
                continue

            # ----------------------------
            # Required fields
            # ----------------------------
            sid = obj.get("sha256")
            label = obj.get("label")

            if sid is None or label is None:
                skipped += 1
                continue

            # ----------------------------
            # Optional fields
            # ----------------------------
            caps = obj.get("caps", [])
            behaviors = obj.get("mbc", [])
            tactics = obj.get("ttps", [])

            # ----------------------------
            # Insert sample
            # ----------------------------
            cur.execute("""
                INSERT OR REPLACE INTO samples
                VALUES (?, ?, ?)
            """, (sid, label, split))

            # ----------------------------
            # CAPS
            # ----------------------------
            for c in caps:

                name = normalize(
                    c.get("Capability", "")
                )

                if not name:
                    continue

                cid = get_or_create_id(
                    cap_to_id,
                    name,
                    next_cap_id
                )

                cur.execute("""
                    INSERT INTO sample_caps
                    VALUES (?, ?)
                """, (sid, cid))

            # ----------------------------
            # BEHAVIORS
            # ----------------------------
            for b in behaviors:

                name = normalize(
                    b.get("Behavior", "")
                )

                if not name:
                    continue

                bid = get_or_create_id(
                    behavior_to_id,
                    name,
                    next_behavior_id
                )

                cur.execute("""
                    INSERT INTO sample_behaviors
                    VALUES (?, ?)
                """, (sid, bid))

            # ----------------------------
            # TACTICS
            # ----------------------------
            for t in tactics:

                name = normalize(
                    t.get("Tactic", "")
                )

                if not name:
                    continue

                tid = get_or_create_id(
                    tactic_to_id,
                    name,
                    next_tactic_id
                )

                cur.execute("""
                    INSERT INTO sample_tactics
                    VALUES (?, ?)
                """, (sid, tid))

            total += 1

            # ----------------------------
            # Batch commit
            # ----------------------------
            if total % batch_size == 0:
                conn.commit()

    conn.commit()

    return total, skipped


# ============================================================
# Save vocabularies
# ============================================================

def save_vocab(cur, table_name, mapping):
    rows = [
        (idx, name)
        for name, idx in mapping.items()
    ]

    cur.executemany(f"""
        INSERT OR IGNORE INTO {table_name}
        VALUES (?, ?)
    """, rows)


# ============================================================
# Main
# ============================================================

def main():

    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--inputs-train",
        nargs="*",
        default=[]
    )

    parser.add_argument(
        "--inputs-test",
        nargs="*",
        default=[]
    )

    parser.add_argument(
        "--output",
        required=True
    )

    parser.add_argument(
        "--batch-size",
        type=int,
        default=1000
    )

    args = parser.parse_args()

    # ========================================================
    # DB init
    # ========================================================

    output_path = Path(args.output)

    conn = sqlite3.connect(output_path)
    cur = conn.cursor()

    # Faster SQLite settings
    cur.execute("PRAGMA journal_mode=WAL;")
    cur.execute("PRAGMA synchronous=OFF;")

    create_tables(cur)
    conn.commit()

    # ========================================================
    # Load existing vocab
    # ========================================================

    print("Loading vocabularies from DB...")

    cap_to_id, next_cap = load_vocab(
        cur,
        "caps_vocab"
    )

    behavior_to_id, next_behavior = load_vocab(
        cur,
        "behaviors_vocab"
    )

    tactic_to_id, next_tactic = load_vocab(
        cur,
        "tactics_vocab"
    )

    next_cap_id = [next_cap]
    next_behavior_id = [next_behavior]
    next_tactic_id = [next_tactic]

    print(f"Loaded caps: {len(cap_to_id)}")
    print(f"Loaded behaviors: {len(behavior_to_id)}")
    print(f"Loaded tactics: {len(tactic_to_id)}")

    # ========================================================
    # Process train files
    # ========================================================

    total_samples = 0
    total_skipped = 0

    for path in args.inputs_train:

        processed, skipped = process_file(
            path=path,
            split="train",

            cur=cur,
            conn=conn,
            batch_size=args.batch_size,

            cap_to_id=cap_to_id,
            behavior_to_id=behavior_to_id,
            tactic_to_id=tactic_to_id,

            next_cap_id=next_cap_id,
            next_behavior_id=next_behavior_id,
            next_tactic_id=next_tactic_id,
        )

        total_samples += processed
        total_skipped += skipped

    # ========================================================
    # Process test files
    # ========================================================

    for path in args.inputs_test:

        processed, skipped = process_file(
            path=path,
            split="test",

            cur=cur,
            conn=conn,
            batch_size=args.batch_size,

            cap_to_id=cap_to_id,
            behavior_to_id=behavior_to_id,
            tactic_to_id=tactic_to_id,

            next_cap_id=next_cap_id,
            next_behavior_id=next_behavior_id,
            next_tactic_id=next_tactic_id,
        )

        total_samples += processed
        total_skipped += skipped

    # ========================================================
    # Save vocabularies
    # ========================================================

    print("\nSaving vocabularies...")

    save_vocab(cur, "caps_vocab", cap_to_id)
    save_vocab(cur, "behaviors_vocab", behavior_to_id)
    save_vocab(cur, "tactics_vocab", tactic_to_id)

    conn.commit()

    # ========================================================
    # Create indexes
    # ========================================================

    print("Creating indexes...")

    create_indexes(cur)

    conn.commit()

    # ========================================================
    # Summary
    # ========================================================

    print("\nDone.")
    print(f"Total samples processed: {total_samples}")
    print(f"Skipped lines: {total_skipped}")

    print(f"Unique caps: {len(cap_to_id)}")
    print(f"Unique behaviors: {len(behavior_to_id)}")
    print(f"Unique tactics: {len(tactic_to_id)}")

    conn.close()


if __name__ == "__main__":
    main()
