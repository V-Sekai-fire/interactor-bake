#!/bin/sh
# Build bake.elf and run its gates through the sandbox host's probe: the alpha
# cull (exactly half a split plane kept; a planted opaque texture fails) and
# the atlas (rects apart, each colour at its centre; a planted overlap fails).
#
#   SANDBOX_API=<sandbox-api> RV64_TOOLCHAIN=<riscv64-sysroot>/toolchain.cmake \
#   PROBE=<sbhost_probe> HOST_LIB=<sandbox_host library> tests/bake/build.sh
#
# clang and lld come from pixi: pixi exec -s clangxx -s lld -s cmake -s ninja -- tests/bake/build.sh
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
OUT="$ROOT/build/bake"
cmake -S "$ROOT/guest/bake" -B "$OUT" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$RV64_TOOLCHAIN" -DSANDBOX_API="$SANDBOX_API"
cmake --build "$OUT"
echo "built $OUT/bake"
[ -n "$PROBE" ] || exit 0
check() { # name clean-arg planted-arg
  clean=$("$PROBE" "$HOST_LIB" call "$OUT/bake" "$1" "$2" | grep '^rc')
  planted=$("$PROBE" "$HOST_LIB" call "$OUT/bake" "$1" "$3" | grep '^rc')
  echo "$clean"
  echo "$planted"
  case "$clean" in *"result PASS"*) ;; *) echo "$1: clean run did not pass"; exit 1;; esac
  case "$planted" in *"result FAIL"*) ;; *) echo "$1: planted control did not fail"; exit 1;; esac
}
check gate_alpha i:0 i:1
check gate_atlas i:0 i:1
echo "gates: PASS, controls: FAIL as planted"
