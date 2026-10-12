#!/usr/bin/env python3
import argparse,hashlib,json
from pathlib import Path
def generate(root,out):
 m=json.loads((root/"manifest.json").read_text())
 out.mkdir(parents=True,exist_ok=True)
 asm=[".section .rodata",".balign 16"]
 h=["#pragma once","#include <stdint.h>","#include <stddef.h>","typedef struct EmbeddedAsset {const uint8_t *bytes;size_t size;} EmbeddedAsset;","EmbeddedAsset embedded_asset_find(const char *);"]
 c=['#include "embedded.h"','#include <string.h>']
 records=[]
 for i,x in enumerate(m["files"]):
  path=(root/x["path"]).resolve()
  if not path.is_file():raise ValueError(f"Missing embedded asset: {x['path']}")
  data=path.read_bytes()
  if len(data)!=x["size"] or hashlib.sha256(data).hexdigest()!=x["sha256"]:
   raise ValueError(f"Invalid asset: {x['path']}")
  a=f"asset_{i}_begin";b=f"asset_{i}_end"
  asm+=[".global "+a,a+':',f'.incbin "{path}"',".global "+b,b+':',".balign 16"]
  c += [f"extern const uint8_t {a}[];",f"extern const uint8_t {b}[];"]
  records.append((x["path"],a,b))
 c+=["typedef struct R{const char*n;const uint8_t*a,*b;}R;","static const R items[]={"]
 c += [f'{{"{name}",{a},{b}}},' for name,a,b in records]
 c += ["};","EmbeddedAsset embedded_asset_find(const char*name){EmbeddedAsset nil={0,0};if(!name)return nil;",
       "for(size_t i=0;i<sizeof(items)/sizeof(items[0]);++i){",
       "if(!strcmp(name,items[i].n)){EmbeddedAsset item={items[i].a,(size_t)(items[i].b-items[i].a)};return item;}}return nil;}"]
 (out/"embedded.S").write_text("\n".join(asm)+"\n")
 (out/"embedded.c").write_text("\n".join(c)+"\n")
 (out/"embedded.h").write_text("\n".join(h)+"\n")
if __name__=="__main__":
 ap=argparse.ArgumentParser();ap.add_argument("--root",type=Path,required=True);ap.add_argument("--out",type=Path,required=True)
 a=ap.parse_args();generate(a.root,a.out)
