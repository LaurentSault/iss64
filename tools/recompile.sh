#!/usr/bin/env bash
# Regenerates the symbol file and recompiled C from the user's ROM.
# Usage: tools/recompile.sh   (expects baserom.us.z64 in the repo root)
set -euo pipefail
cd "$(dirname "$0")/.."

ROM=baserom.us.z64
SHA1=d2c258ee3844be77049e4af5208f8f2bb073a86e
RECOMP=lib/N64ModernRuntime/N64Recomp

[ -f "$ROM" ] || { echo "Put your ISS64 (USA) ROM at ./$ROM"; exit 1; }
echo "$SHA1  $ROM" | sha1sum -c --quiet || { echo "ROM SHA1 mismatch: need the USA [!] dump"; exit 1; }

# Use the N64Recomp pinned by N64ModernRuntime so the generated C matches the runtime's recomp.h.
[ -d "$RECOMP" ] || tools/fetch_deps.sh
if [ ! -x "$RECOMP/build/N64Recomp" ]; then
    cmake -S "$RECOMP" -B "$RECOMP/build" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$RECOMP/build" -j --target N64RecompCLI RSPRecomp
fi

[ -d venv ] || { python3 -m venv venv && venv/bin/pip install -q -r tools/requirements.txt; }

rm -rf build/RecompiledFuncs && mkdir -p build/RecompiledFuncs build/rsp
venv/bin/python tools/gen_syms.py "$ROM" config/overlays.md config/libultra_syms.txt build/iss64.syms.toml
"$RECOMP/build/N64Recomp" iss64.recomp.toml
"$RECOMP/build/RSPRecomp" aspMain.us.toml
