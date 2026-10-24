#!/usr/bin/env bash
# Clones the runtime, renderer and frontend into lib/ at pinned commits.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p lib

fetch() { # name url commit
    if [ ! -d "lib/$1/.git" ]; then
        git clone "$2" "lib/$1"
    fi
    git -C "lib/$1" fetch -q origin "$3" 2>/dev/null || true
    git -C "lib/$1" checkout -q "$3"
    git -C "lib/$1" submodule update -q --init --recursive
    # Apply our fixes to this dependency (patches/deps/<name>-*.patch), once.
    for p in patches/deps/"$1"-*.patch; do
        [ -f "$p" ] || continue
        if git -C "lib/$1" apply --check "$PWD/$p" 2>/dev/null; then
            git -C "lib/$1" apply "$PWD/$p"
        fi
    done
}

fetch N64ModernRuntime https://github.com/N64Recomp/N64ModernRuntime ca568b6
fetch rt64             https://github.com/rt64/rt64                  f0d8c9f
fetch RecompFrontend   https://github.com/N64Recomp/RecompFrontend   b3b7ebb

# UI stylesheet, fonts and icons for RecompFrontend's menus, taken from BanjoRecomp.
if [ ! -f lib/assets/recomp.rcss ]; then
    rm -rf lib/BanjoRecomp-assets
    git clone -q --filter=blob:none --sparse https://github.com/BanjoRecomp/BanjoRecomp.git lib/BanjoRecomp-assets
    git -C lib/BanjoRecomp-assets checkout -q dbd5215
    git -C lib/BanjoRecomp-assets sparse-checkout set assets
    mkdir -p lib/assets/fonts
    cp -r lib/BanjoRecomp-assets/assets/{recomp.rcss,NotoEmoji-Regular.ttf,promptfont,icons} lib/assets/
    cp lib/BanjoRecomp-assets/assets/InterVariable.ttf lib/assets/fonts/primary.ttf
fi
