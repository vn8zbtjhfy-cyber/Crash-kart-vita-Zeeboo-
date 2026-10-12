#!/usr/bin/env python3
import hashlib,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
m=json.loads((root/"assets/manifest.json").read_text())
assert len(m["files"])==12
for x in m["files"]:
 b=(root/"assets"/x["path"]).read_bytes()
 assert len(b)==x["size"] and hashlib.sha256(b).hexdigest()==x["sha256"],x["path"]
assert (root/"assets/brew/mod/274214/data.vfs").read_bytes().startswith(b"FUFS")
print("12 ROM files SHA256 checked")
