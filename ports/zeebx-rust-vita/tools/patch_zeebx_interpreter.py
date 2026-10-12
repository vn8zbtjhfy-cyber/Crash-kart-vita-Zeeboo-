#!/usr/bin/env python3
"""Select the Zeebx ARM interpreter, not Dynarmic JIT, for Vita."""
import argparse
from pathlib import Path

def patch(root: Path):
    for name in ["src/cpu/mod.rs", "src/session.rs", "Cargo.toml"]:
        path = root / name
        if not path.is_file():
            raise SystemExit(f"Missing Zeebx source: {path}")
        before = path.read_text()
        needle = 'target_os = "ios"'
        add = 'target_os = "ios", target_os = "vita"'
        if needle not in before:
            raise SystemExit(f"Missing cfg anchor: {path}")
        after = before.replace(add,needle).replace(needle,add)
        if after != before:
            path.write_text(after)
            print("Patched",name)
        else:
            print("Already patched",name)
if __name__ == "__main__":
    p=argparse.ArgumentParser();p.add_argument("zeebx_checkout",type=Path)
    patch(p.parse_args().zeebx_checkout)
