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
# Windows (Git Bash) uses .exe suffixes, `python` and venv/Scripts.
EXE=""; PYTHON=python3; VENV_BIN=venv/bin
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) EXE=".exe"; PYTHON=python; VENV_BIN=venv/Scripts ;;
esac

if [ ! -x "$RECOMP/build/N64Recomp$EXE" ]; then
    cmake -S "$RECOMP" -B "$RECOMP/build" -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build "$RECOMP/build" --target N64RecompCLI RSPRecomp
fi

[ -d venv ] || { "$PYTHON" -m venv venv && "$VENV_BIN/python" -m pip install -q -r tools/requirements.txt; }

rm -rf build/RecompiledFuncs && mkdir -p build/RecompiledFuncs build/rsp
"$VENV_BIN/python" tools/gen_syms.py "$ROM" config/overlays.md config/libultra_syms.txt build/iss64.syms.toml
"$RECOMP/build/N64Recomp$EXE" iss64.recomp.toml
"$RECOMP/build/RSPRecomp$EXE" aspMain.us.toml
