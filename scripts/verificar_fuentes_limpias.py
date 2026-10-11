#!/usr/bin/env python3
"""Check source-only GitHub ZIP for required Vita files and forbidden assets.
This does NOT claim that the VitaSDK build succeeds.
"""
import pathlib
import sys
import zipfile

REQUIRED = {
    'CMakeLists.txt', 'src/vita_main.c', 'src/cnk_vita_native_app.c',
    'src/cnk_vita_diagnostics.c', 'src/cnk_vita_diagnostics.h',
    'scripts/vita_elf_preflight.sh', '.github/workflows/vita-elf-link.yml',
}
FORBIDDEN = {'.vfs', '.mod', '.sig', '.vpk', '.self', '.velf', '.elf',
             '.mif', '.wav', '.ogg', '.mp3', '.png', '.jpg', '.jpeg', '.zip'}

def audit(path):
    with zipfile.ZipFile(path) as arc:
        names = arc.namelist()
        normalized = set(names)
        if len(normalized) != len(names):
            raise ValueError('duplicate archive entry')
        missing = sorted(REQUIRED - normalized)
        if missing:
            raise ValueError('missing source files: ' + ', '.join(missing))
        for name in names:
            posix = pathlib.PurePosixPath(name)
            if name.startswith('/') or '\\' in name or '..' in posix.parts:
                raise ValueError('unsafe path: ' + name)
            if posix.suffix.lower() in FORBIDDEN:
                raise ValueError('binary/original asset in source ZIP: ' + name)
            # The source package is intentionally limited to source/build files.
            if not (name in {'README.md', '.gitignore', 'CMakeLists.txt'}
                    or name.startswith(('src/', 'scripts/', 'tests/', 'tools/', '.github/'))):
                raise ValueError('unexpected distribution entry: ' + name)
    return len(names)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('usage: verificar_fuentes_limpias.py <source-only.zip>')
    print(f'OK: {audit(sys.argv[1])} clean source entries; no original assets')
