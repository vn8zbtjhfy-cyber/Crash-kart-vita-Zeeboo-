#!/usr/bin/env bash
# Build vitaGL with the animated splash DISABLED in the library itself.
# Linking against the VitaSDK's prebuilt vitaGL would retain that splash.
set -euo pipefail
: "${VITASDK:?VITASDK environment variable required}"
command -v arm-vita-eabi-gcc >/dev/null
command -v arm-vita-eabi-nm >/dev/null
command -v make >/dev/null
if [[ ! -f vitagl-source/Makefile ]]; then
  git clone --depth 1 https://github.com/Rinnegatamante/vitaGL.git vitagl-source
fi
cd vitagl-source
echo "Rebuilding vitaGL without splash at commit $(git rev-parse --short HEAD)"
make -j2 NO_SPLASHSCREEN=1
test -s libvitaGL.a
test -s source/vgl.o
# Check that the compiled init function has no unresolved splash invocation.
if arm-vita-eabi-nm -u source/vgl.o | grep -q 'invoke_splashscreen'; then
  echo "ERROR: vitaGL object still references splash" >&2
  exit 1
fi
make NO_SPLASHSCREEN=1 install
test -s "$VITASDK/arm-vita-eabi/lib/libvitaGL.a"
echo "OK: NO_SPLASHSCREEN vitaGL compiled and installed"
