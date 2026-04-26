from pathlib import Path
import sys
DIR = Path(__file__).resolve().parent
sys.path.append(DIR)
from common_utils import is_JSONL_PE
import json
from thrember.model import gather_feature_paths
from tqdm import tqdm

def enumerate_capa_features(
    data_dir: Path | str,
    capa_features_path: Path | str,
):
    data_path = Path(data_dir)
    jsonl_paths = list(filter(
        is_JSONL_PE,
        gather_feature_paths(data_path, subset=""), # take all subsets: train, test, challenge etc.
    ))

    ALL_CAPS: set[str] = set()
    ALL_TTPS: set[str] = set()
    ALL_MBCS: set[str] = set()

    for p in tqdm(jsonl_paths):
        with open(p, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                raw_obj = json.loads(line)

                caps = [(x['Namespace'] + ':' + x['Capability']).lower() for x in raw_obj['caps']]
                ttps = [(x['Tactic'] + ':' + x['Technique']).lower() for x in raw_obj['ttps']]
                mbcs = [(x['Objective'] + ':' + x['Behavior']).lower() for x in raw_obj['mbc']]

                ALL_CAPS.update(caps)
                ALL_TTPS.update(ttps)
                ALL_MBCS.update(mbcs)
    
    capa_features: dict[str, int] = {}
    i = 0
    for feature in (*ALL_CAPS, *ALL_TTPS, *ALL_MBCS):
        capa_features[feature] = i
        i += 1
    
    with open(capa_features_path, 'w', encoding='utf-8') as f:
        json.dump(capa_features, f)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description="Enumerate all Capa features")
    parser.add_argument("data_dir", help="Path to EMBER2024 JSONL dataset")
    parser.add_argument("capa_features_path", help="Path to enumerated Capa features file (JSON)")
    args = parser.parse_args()

    enumerate_capa_features(
        data_dir=args.data_dir,
        capa_features_path=args.capa_features_path,
    )
