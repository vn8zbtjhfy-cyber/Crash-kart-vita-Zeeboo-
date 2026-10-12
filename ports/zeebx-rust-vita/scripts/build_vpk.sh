#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${VITASDK:?VITASDK required}"
export PATH="$VITASDK/bin:$PATH"
for cmd in cargo rustup cmake git python3 arm-vita-eabi-gcc; do
 command -v "$cmd" >/dev/null || { echo "Missing $cmd" >&2; exit 1; }
done
if [[ ! -f zeebx-emu/Cargo.toml ]]; then
 git clone --depth 1 --branch development https://github.com/ZeebxTeam/zeebx-emu.git zeebx-emu
fi
python3 tools/patch_zeebx_interpreter.py zeebx-emu
python3 tools/patch_zeebx_embedded_fs.py zeebx-emu
python3 tools/patch_zeebx_sqlite_vita.py zeebx-emu
python3 tools/patch_zeebx_memory_vita.py zeebx-emu
python3 tests/validate_assets.py
python3 tests/fufs_index_smoke.py
cargo +nightly build -Z build-std=std,panic_abort --release --target armv7-sony-vita-newlibeabihf --manifest-path rust/zeebx_vita/Cargo.toml
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" -DCMAKE_BUILD_TYPE=Release -DNITRO_EXPERIMENTAL_VPK=ON
cmake --build build --parallel 2
test -s build/Crash_Nitro_Kart_3D.vpk
