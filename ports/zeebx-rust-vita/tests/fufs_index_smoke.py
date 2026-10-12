#!/usr/bin/env python3
import struct
from pathlib import Path
data=(Path(__file__).resolve().parents[1]/"assets/brew/mod/274214/data.vfs").read_bytes()
assert data.startswith(b"FUFS")
count=struct.unpack_from("<I",data,8)[0]
assert count==412
entries=[struct.unpack_from("<III",data,12+i*12) for i in range(count)]
assert all(o+s<=len(data) for o,ident,s in entries)
assert all(entries[i][1]<entries[i+1][1] for i in range(count-1))
print("FUFS index",count,"entries")
