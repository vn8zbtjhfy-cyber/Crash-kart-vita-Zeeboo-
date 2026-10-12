#!/usr/bin/env python3
"""ROM-free regression for exact Zeebx-cache -> FUFS PNG mappings.

All synthetic files are generated in a temporary directory and removed.
No commercial ROM, game source image or Zeebx cache is included here.
"""
from __future__ import annotations

import importlib.util
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "generar_texturas_desde_cache_zeebx.py"
spec = importlib.util.spec_from_file_location("cnk_zeebx_cache_match", SCRIPT)
assert spec is not None and spec.loader is not None
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)


def chunk(tag: bytes, payload: bytes) -> bytes:
    return (struct.pack(">I", len(payload)) + tag + payload
            + struct.pack(">I", zlib.crc32(tag + payload) & 0xffffffff))


def png(color: int) -> bytes:
    hdr = struct.pack(">IIBBBBB", 1, 1, 8, 6, 0, 0, 0)
    pixel = zlib.compress(bytes([0, color, 0, 0, 255]))
    return matcher.PNG_SIG + chunk(b"IHDR", hdr) + chunk(b"IDAT", pixel) + chunk(b"IEND", b"")


def scene(names: list[str]) -> bytes:
    body = struct.pack("<I", len(names))
    for name in names:
        encoded = name.encode("ascii")
        body += bytes([len(encoded)]) + encoded
    payload = b"SCEN" + b"TXTR" + struct.pack("<I", len(body)) + body
    return b"PIFF" + struct.pack("<I", len(payload)) + payload


def fufs(items: list[tuple[int, bytes]]) -> bytes:
    start = 12 + 12 * len(items)
    table = b""
    payloads = b""
    for ident, data in items:
        table += struct.pack("<III", start + len(payloads), ident, len(data))
        payloads += data
    return b"FUFS" + struct.pack("<II", 1, len(items)) + table + payloads


class TestZeebxTextureMatching(unittest.TestCase):
    def test_crc_and_truncation(self):
        a = png(70)
        self.assertTrue(matcher.inspect_png(a))
        self.assertFalse(matcher.inspect_png(a[:-1]))
        altered = bytearray(a)
        altered[16] ^= 1
        self.assertFalse(matcher.inspect_png(bytes(altered)))
        self.assertIsNone(matcher.scene_textures(b"foo"))
        self.assertEqual(matcher.scene_textures(scene(["grass.png"])), ["grass.png"])

    def test_source_bytes_drive_manifest_without_guesses(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            cache = root / "cache" / "mod" / "274214" / "data" / "textures" / "world"
            cache.mkdir(parents=True)
            original = png(20)
            other = png(45)
            # The source carries compressed PNG bytes as PLZP, as the
            # native game's fufs_extract() already supports.
            compressed = b"PLZP" + b"\0" * 8 + zlib.compress(original)
            archive = root / "data.vfs"
            archive.write_bytes(fufs([
                (0x13572468, scene(["grass.png", "missing.png"])),
                (0xaabbccdd, compressed),
                (0x12344321, other),
            ]))
            (cache / "grass.png").write_bytes(original)
            rows, scenes, count, ambiguous = matcher.build_verified(
                matcher.Fufs(archive), root / "cache"
            )
            self.assertEqual((scenes, count, ambiguous), (1, 2, 0))
            self.assertEqual(rows, [(0, 0x13572468, 0, "grass.png", 1, 0xaabbccdd)])
            out = root / "verified_textures.tsv"
            cli = subprocess.run([sys.executable, str(SCRIPT),
                "--vfs", str(archive), "--zeebx-cache", str(root / "cache"),
                "--out", str(out)], capture_output=True, text=True, check=False)
            self.assertEqual(cli.returncode, 0, cli.stderr)
            self.assertTrue(out.exists())
            self.assertIn("0\t13572468\t0\tgrass.png\t1\taabbccdd", out.read_text())
            # A second run refuses to overwrite previously reviewed bindings.
            again = subprocess.run([sys.executable, str(SCRIPT),
                "--vfs", str(archive), "--zeebx-cache", str(root / "cache"),
                "--out", str(out)], capture_output=True, text=True, check=False)
            self.assertNotEqual(again.returncode, 0)
            # A conflicting same-named file makes the identity ambiguous.
            second = root / "cache" / "elsewhere"
            second.mkdir()
            (second / "grass.png").write_bytes(other)
            rows, _, _, ambiguous = matcher.build_verified(
                matcher.Fufs(archive), root / "cache"
            )
            self.assertEqual(rows, [])
            self.assertEqual(ambiguous, 1)

    def test_duplicate_fuFS_resource_not_assigned_to_one_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            cache = root / "cache"
            cache.mkdir()
            original = png(33)
            (cache / "grass.png").write_bytes(original)
            archive = root / "data.vfs"
            archive.write_bytes(fufs([
                (10, scene(["grass.png"])), (20, original), (30, original)
            ]))
            rows, _, _, ambiguous = matcher.build_verified(matcher.Fufs(archive), cache)
            self.assertEqual(rows, [])
            self.assertEqual(ambiguous, 1)
            out = root / "new.tsv"
            result = matcher.main(["--vfs", str(archive),
                                  "--zeebx-cache", str(cache),
                                  "--out", str(out)])
            self.assertEqual(result, 2)
            self.assertFalse(out.exists())

    def test_bad_fufs_table_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            archive = Path(tmp) / "fake.vfs"
            archive.write_bytes(b"FUFS" + struct.pack("<II", 1, 50001))
            with self.assertRaises(ValueError):
                matcher.Fufs(archive)


if __name__ == "__main__":
    unittest.main()
