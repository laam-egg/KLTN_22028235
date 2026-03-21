def main():
    import argparse
    parser = argparse.ArgumentParser(description="Main program")
    parser.add_argument("db_path", help="Path to SQLite3 dataset")
    args = parser.parse_args()

    from .all import all_in_one

    all_in_one(False, args.db_path)
