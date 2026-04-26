import json
import sys

def transform_capa_full(data: dict) -> dict:
    caps = []
    ttps = []
    mbc_out = []

    seen_ttps = set()
    seen_mbc = set()

    for rule in data.get("rules", {}).values():
        meta = rule.get("meta", {})

        caps.append({
            "Capability": meta.get("name", ""),
            "Namespace": meta.get("namespace", ""),
            "Addrs": []
        })

        for entry in meta.get("attack", []):
            parts = entry.get("parts", [])
            if len(parts) >= 2:
                tactic = parts[0].upper()
                # Rebuild full technique string: "Technique::Subtechnique [TID]"
                technique = " :: ".join(parts[1:]) if len(parts) > 2 else parts[1]
                key = (tactic, technique)
                if key not in seen_ttps:
                    seen_ttps.add(key)
                    ttps.append({"Tactic": tactic, "Technique": technique})

        for entry in meta.get("mbc", []):
            parts = entry.get("parts", [])
            if len(parts) >= 2:
                objective = parts[0].upper()
                behavior = " :: ".join(parts[1:]) if len(parts) > 2 else parts[1]
                key = (objective, behavior)
                if key not in seen_mbc:
                    seen_mbc.add(key)
                    mbc_out.append({"Objective": objective, "Behavior": behavior})

    return {"caps": caps, "ttps": ttps, "mbc": mbc_out}


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else None
    if path:
        with open(path) as f:
            data = json.load(f)
    else:
        data = json.load(sys.stdin)

    print(json.dumps(transform_capa_full(data), indent=4))
