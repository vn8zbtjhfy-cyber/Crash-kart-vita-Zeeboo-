#!/usr/bin/env python3
"""Generate verified SCEN->PNG bindings from *user-owned* Zeebx cache bytes.

No BREW emulation, no game data bundled. Compare exact original PNG bytes
against extracted FUFS resources and output the existing native Vita texture
manifest format. Ambiguous names, duplicate resource bytes and malformed
assets are skipped instead of invented.
"""
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import struct
import sys
import tempfile
import zlib

MAX_VFS = 64 * 1024 * 1024
MAX_DECODED = 64 * 1024 * 1024
MAX_PNGS = 8192
PNG_SIG = b"\x89PNG\r\n\x1a\n"


def u32(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 4 > len(data):
        raise ValueError("truncated little-endian integer")
    return struct.unpack_from("<I", data, offset)[0]


def inflate_bounded(data: bytes) -> bytes:
    stream = zlib.decompressobj()
    decoded = stream.decompress(data, MAX_DECODED + 1)
    if len(decoded) > MAX_DECODED:
        raise ValueError("decompressed resource too large")
    decoded += stream.flush(MAX_DECODED + 1 - len(decoded))
    if len(decoded) > MAX_DECODED or not stream.eof or stream.unused_data:
        raise ValueError("truncated, oversized or concatenated zlib resource")
    return decoded


class Fufs:
    def __init__(self, path: Path):
        self.data = path.read_bytes()
        if not 12 <= len(self.data) <= MAX_VFS or self.data[:4] != b"FUFS":
            raise ValueError("not a valid FUFS header/size")
        count = u32(self.data, 8)
        if count > 50000 or 12 + 12 * count > len(self.data):
            raise ValueError("invalid FUFS table")
        self.entries: list[tuple[int, int, int]] = []
        for i in range(count):
            off, ident, length = struct.unpack_from("<III", self.data, 12 + 12 * i)
            if off < 12 + 12 * count or off > len(self.data) or length > len(self.data) - off:
                raise ValueError(f"invalid FUFS entry {i}")
            self.entries.append((off, ident, length))

    def extract(self, index: int) -> bytes:
        off, _, n = self.entries[index]
        payload = self.data[off:off+n]
        if payload.startswith(b"PLZP"):
            if len(payload) < 12:
                raise ValueError("short PLZP resource")
            payload = inflate_bounded(payload[12:])
        elif len(payload) > 1 and payload[:2] in (b"\x78\x01", b"\x78\x9c", b"\x78\xda"):
            payload = inflate_bounded(payload)
        for _ in range(3):
            if len(payload) <= 1 or payload[:2] not in (
                b"\x78\x01", b"\x78\x9c", b"\x78\xda"
            ):
                break
            payload = inflate_bounded(payload)
        return payload


def inspect_png(data: bytes) -> bool:
    if not data.startswith(PNG_SIG):
        return False
    at, ihdr, idat, iend = 8, False, False, False
    while at < len(data):
        if len(data) - at < 12:
            return False
        n = int.from_bytes(data[at:at+4], "big")
        if n > len(data) - at - 12:
            return False
        chunk = data[at+4:at+8]
        raw = data[at+8:at+8+n]
        crc = int.from_bytes(data[at+8+n:at+12+n], "big")
        if zlib.crc32(chunk + raw) & 0xffffffff != crc:
            return False
        if not ihdr:
            if chunk != b"IHDR" or n != 13:
                return False
            width = int.from_bytes(raw[0:4], "big")
            height = int.from_bytes(raw[4:8], "big")
            if not (0 < width <= 8192 and 0 < height <= 8192):
                return False
            ihdr = True
        elif chunk == b"IHDR":
            return False
        if chunk == b"IDAT":
            idat = True
        if chunk == b"IEND":
            if n != 0 or not idat or at + 12 != len(data):
                return False
            iend = True
            break
        at += n + 12
    return ihdr and idat and iend


def vita_uploadable_png(data: bytes) -> bool:
    """Match the native vitaGL uploader's dimensions and RGBA memory cap.

    This is a preflight only: libpng must still decode the full image on Vita.
    """
    if not inspect_png(data):
        return False
    width = int.from_bytes(data[16:20], "big")
    height = int.from_bytes(data[20:24], "big")
    return (width <= 4096 and height <= 4096
            and width * height * 4 <= 32 * 1024 * 1024)


def scene_textures(data: bytes) -> list[str] | None:
    """Read original PIFF SCEN TXTR string table, preserving exact slot order."""
    if (len(data) < 12 or data[:4] != b"PIFF" or data[8:12] != b"SCEN"
            or u32(data, 4) != len(data) - 8):
        return None
    at, txtr = 12, None
    while at < len(data):
        if len(data) - at < 8:
            raise ValueError("malformed SCEN chunk")
        n = u32(data, at+4)
        if n > len(data) - at - 8:
            raise ValueError("truncated SCEN chunk")
        if data[at:at+4] == b"TXTR":
            if txtr is not None:
                raise ValueError("duplicate SCEN TXTR chunk")
            txtr = data[at+8:at+8+n]
        at += 8 + n
    if txtr is None or len(txtr) < 4:
        raise ValueError("missing scene texture strings")
    count = u32(txtr, 0)
    if count > 1024:
        raise ValueError("oversized SCEN TXTR count")
    names, pos = [], 4
    for _ in range(count):
        if pos == len(txtr):
            raise ValueError("truncated scene name")
        n = txtr[pos]
        pos += 1
        if n > 250 or n > len(txtr) - pos:
            raise ValueError("oversized scene name")
        names.append(txtr[pos:pos+n].decode("ascii", errors="strict"))
        pos += n
    if pos != len(txtr):
        raise ValueError("trailing TXTR bytes")
    return names


def cache_pngs(root: Path) -> dict[str, list[Path]]:
    """Read-only walk; don't follow links out of the user's extracted cache."""
    items: dict[str, list[Path]] = {}
    count = 0
    for folder, dirs, files in os.walk(root, followlinks=False):
        dirs[:] = [d for d in dirs if not Path(folder, d).is_symlink()]
        for filename in files:
            file = Path(folder, filename)
            if file.is_symlink() or file.suffix.lower() != ".png":
                continue
            count += 1
            if count > MAX_PNGS:
                raise ValueError("cache contains more than 8192 PNG files")
            items.setdefault(filename, []).append(file)
    return items


def build_verified(vfs: Fufs, cache: Path):
    """Return (manifest rows, audit counters); refuses uncertain bindings."""
    original: dict[bytes, list[tuple[int, int, bytes]]] = {}
    scenes: list[tuple[int, int, list[str]]] = []
    for i, (_, ident, _) in enumerate(vfs.entries):
        raw = vfs.extract(i)
        if raw.startswith(PNG_SIG):
            if not inspect_png(raw):
                raise ValueError(f"corrupt source PNG index {i}")
            # The native uploader refuses textures above 4096px or 32 MiB RGBA.
            # Do not create bindings which are guaranteed to fail at runtime.
            if not vita_uploadable_png(raw):
                continue
            digest = hashlib.sha256(raw).digest()
            original.setdefault(digest, []).append((i, ident, raw))
        elif raw.startswith(b"PIFF") and raw[8:12] == b"SCEN":
            textures = scene_textures(raw)
            if textures is not None:
                scenes.append((i, ident, textures))
    if len(scenes) > 64:
        raise ValueError("too many scenes")
    disk_names = cache_pngs(cache)
    candidate_names = {name.rsplit("/", 1)[-1] for _, _, names in scenes
                       for name in names if name and not ("\\" in name)}
    found: dict[str, tuple[int, int]] = {}
    ambiguous: set[str] = set()
    for basename in sorted(candidate_names):
        if basename in (".", "..") or "/" in basename:
            continue
        files = disk_names.get(basename, [])
        if not files:
            continue
        all_matches: set[tuple[int, int]] = set()
        asset_digests: set[bytes] = set()
        for file in files:
            if file.stat().st_size > 32 * 1024 * 1024:
                ambiguous.add(basename)
                continue
            raw = file.read_bytes()
            if not vita_uploadable_png(raw):
                ambiguous.add(basename)
                continue
            h = hashlib.sha256(raw).digest()
            asset_digests.add(h)
            for idx, ident, original_bytes in original.get(h, []):
                if raw == original_bytes:
                    all_matches.add((idx, ident))
        # An identical filename in different directories with different bytes
        # cannot safely become one source-index mapping, even if one matched.
        if basename in ambiguous or len(asset_digests) != 1 or len(all_matches) != 1:
            ambiguous.add(basename)
        else:
            found[basename] = next(iter(all_matches))
    rows = []
    for scene_index, scene_id, names in scenes:
        for slot, name in enumerate(names):
            if (not name or len(name) >= 96 or any(ord(ch) < 32 or ord(ch) > 126 for ch in name)
                    or "\\" in name or "\t" in name
                    or "\r" in name or "\n" in name or name.startswith("/")
                    or ".." in name.split("/")):
                continue
            basename = name.rsplit("/", 1)[-1]
            target = found.get(basename)
            if target is None:
                continue
            png_index, png_id = target
            rows.append((scene_index, scene_id, slot, name, png_index, png_id))
    if len(rows) > 4096:
        raise ValueError("too many validated texture references")
    return rows, len(scenes), len(original), len(ambiguous)


def write_manifest(output: Path, rows: list[tuple]) -> None:
    if output.exists():
        raise FileExistsError(f"refusing to overwrite existing manifest: {output}")
    output.parent.mkdir(parents=True, exist_ok=True)
    fd, tempname = tempfile.mkstemp(prefix=".cnk_verified_", dir=str(output.parent))
    try:
        with os.fdopen(fd, "w", encoding="ascii", newline="\n") as f:
            f.write("# Byte-exact matches from user-owned Zeebx extracted files + FUFS\n")
            f.write("# Do not ship the user-owned source files or PNGs.\n")
            for scene_i, scene_id, slot, name, png_i, png_id in rows:
                f.write(f"{scene_i}\t{scene_id:08x}\t{slot}\t{name}\t"
                        f"{png_i}\t{png_id:08x}\n")
        # Atomic create-if-absent: os.replace() could silently overwrite a
        # reviewed manifest created by another process after the exists() check.
        # The temporary file and destination share a directory/filesystem.
        os.link(tempname, output)
    finally:
        if os.path.exists(tempname):
            os.unlink(tempname)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vfs", type=Path, required=True,
                        help="user-owned original data.vfs (never uploaded)")
    parser.add_argument("--zeebx-cache", type=Path, required=True,
                        help="user-owned extracted Zeebx game cache directory")
    parser.add_argument("--out", type=Path, required=True,
                        help="new verified_textures.tsv, never overwritten")
    args = parser.parse_args(argv)
    try:
        if not args.zeebx_cache.is_dir():
            raise ValueError("Zeebx extracted cache directory not found")
        if args.out.exists():
            raise FileExistsError("output manifest already exists")
        rows, scenes, png, ambiguous = build_verified(
            Fufs(args.vfs), args.zeebx_cache
        )
        if not rows:
            print(f"NO MATCH: {scenes} SCEN, {png} source PNG; "
                  f"{ambiguous} ambiguous basenames; no file written.",
                  file=sys.stderr)
            return 2
        write_manifest(args.out, rows)
        print(f"VERIFIED {len(rows)} scene texture rows; {scenes} SCEN, "
              f"{png} unique source PNG hashes; "
              f"{ambiguous} ambiguous basenames skipped.")
        print("The native Vita manifest verifier must also pass when loaded.")
        return 0
    except (ValueError, OSError, zlib.error, UnicodeError) as error:
        print(f"FAILED: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
