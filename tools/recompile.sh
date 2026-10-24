#!/usr/bin/env bash
# Regenerates the symbol file and recompiled C from the user's ROM.
# Usage: tools/recompile.sh   (expects baserom.us.z64 in the repo root)
set -euo pipefail
cd "$(dirname "$0")/.."

ROM=baserom.us.z64
SHA1=d2c258ee3844be77049e4af5208f8f2bb073a86e
N64RECOMP_REV=ffb39cd

[ -f "$ROM" ] || { echo "Put your ISS64 (USA) ROM at ./$ROM"; exit 1; }
echo "$SHA1  $ROM" | sha1sum -c --quiet || { echo "ROM SHA1 mismatch: need the USA [!] dump"; exit 1; }

if [ ! -x tools/N64Recomp/build/N64Recomp ]; then
    git clone --recursive https://github.com/N64Recomp/N64Recomp.git tools/N64Recomp
    git -C tools/N64Recomp checkout "$N64RECOMP_REV"
    git -C tools/N64Recomp submodule update --init --recursive
    cmake -S tools/N64Recomp -B tools/N64Recomp/build -DCMAKE_BUILD_TYPE=Release
    cmake --build tools/N64Recomp/build -j --target N64RecompCLI
fi

[ -d venv ] || { python3 -m venv venv && venv/bin/pip install -q -r tools/requirements.txt; }

mkdir -p build/RecompiledFuncs
venv/bin/python tools/gen_syms.py "$ROM" config/overlays.md build/iss64.syms.toml
tools/N64Recomp/build/N64Recomp iss64.recomp.toml
