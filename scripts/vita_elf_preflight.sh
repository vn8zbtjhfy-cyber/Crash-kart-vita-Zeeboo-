#!/usr/bin/env bash
# Cross-compile only the diagnostic PS Vita ELF. NO SELF and NO VPK.
set -euo pipefail

if [[ -z "${VITASDK:-}" || ! -f "$VITASDK/share/vita.toolchain.cmake" ]]; then
    echo "ERROR: VITASDK and its real vita.toolchain.cmake are required." >&2
    echo "Host Linux tests are NOT equivalent to ARM/VitaSDK compilation." >&2
    exit 2
fi
if [[ ! -x "$VITASDK/bin/arm-vita-eabi-gcc" ]]; then
    echo "ERROR: missing executable $VITASDK/bin/arm-vita-eabi-gcc" >&2
    exit 2
fi
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${1:-$root/build-vita-elf}"
"$VITASDK/bin/arm-vita-eabi-gcc" --version | head -n 1
cmake -S "$root" -B "$build" \
    -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCNK_ENABLE_VITA_ELF_LINK_CHECK=ON
cmake --build "$build" --target cnk_vita_elf_link_check --parallel "${CNK_BUILD_JOBS:-4}"
echo "OK: ARM ELF link completed; this is NOT an installable/console-tested game."
