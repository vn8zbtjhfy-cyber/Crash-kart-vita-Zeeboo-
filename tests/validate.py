#!/usr/bin/env python3
"""Validate C decoder content against independent Python extraction manifest.
Usage: python3 tests/validate.py data.vfs tests/manifest_recursos.csv
"""
import csv, hashlib, subprocess, sys, tempfile
from pathlib import Path
root=Path(__file__).resolve().parent.parent
if len(sys.argv)!=3:
 sys.exit('Uso: python3 tests/validate.py data.vfs tests/manifest_recursos.csv')
vfs=Path(sys.argv[1]); manifest=Path(sys.argv[2]); bin_path=root/'tests/fufs_test'
subprocess.run(['gcc','-std=c99','-O2','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(root/'src/fufs.c'),str(root/'src/test_vfs.c'),'-lz','-o',str(bin_path)],check=True)
with tempfile.TemporaryDirectory() as folder:
 lines=subprocess.check_output([str(bin_path),str(vfs),folder],text=True).splitlines()
 by_i={}
 for line in lines:
  if len(line.split())==5:
   i,ident,ext,n,crc=line.split();by_i[int(i)]=(ident,ext,int(n),int(crc,16))
 rows=list(csv.DictReader(manifest.open()))
 assert len(rows)==len(by_i)==412,(len(rows),len(by_i))
 for r in rows:
  i=int(r['indice']); ident,ext,n,crc=by_i[i]
  assert ident==r['crc'] and ext==r['tipo'] and n==int(r['tamano_extraido']),(i,ident,ext,n)
  path=Path(folder)/f'{i:03d}_{ident}.{ext}'
  data=path.read_bytes()
  assert hashlib.sha256(data).hexdigest()==r['sha256'],(i,'SHA256 mismatch')
 print('ÉXITO: lector C verificado contra los 412 archivos de la ROM y SHA256 de cada uno.')
