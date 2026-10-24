# ISS64 PC port

A native PC port of **International Superstar Soccer 64** (N64, USA), built by statically recompiling the game's code with [N64Recomp](https://github.com/N64Recomp/N64Recomp). It will run on [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime) and render with [RT64](https://github.com/rt64/rt64).

**This repository contains no game code or data.** You supply your own ROM, and everything is generated from it locally.

## Status
Early. On Linux it boots through the Konami logo and the intro to the title screen, and keyboard input reaches the main menu. Known issue: the main menu buttons render without their labels. Nothing past the main menu has been tested yet.
- The whole game's MIPS code recompiles to C: 5,654 functions across main and 15 code overlays.
- 234 libultra functions are identified, so the runtime can swap in its own versions.
- The audio microcode (aspMain) is recompiled too.
- The harness in `src/` wires everything to N64ModernRuntime, RT64 and RecompFrontend.

## Requirements
- `International Superstar Soccer 64 (U) [!].z64`, SHA1 `d2c258ee3844be77049e4af5208f8f2bb073a86e`
- CMake 3.20+, Ninja, Clang (on Windows, clang-cl from Visual Studio), Python 3.10+, git
- Linux: SDL2, X11, Freetype and Vulkan development packages
- A GPU with Vulkan (Linux) or Direct3D 12 (Windows) to run it

## Build
```sh
cp "/path/to/International Superstar Soccer 64 (U) [!].z64" baserom.us.z64
tools/fetch_deps.sh          # clones N64ModernRuntime, RT64, RecompFrontend into lib/
tools/recompile.sh           # generates build/RecompiledFuncs and build/rsp from your ROM
cmake -S . -B build/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build/cmake
```
Run `build/cmake/ISS64Recompiled` and choose your ROM in the launcher. Or set `ISS64_ROM=/path/to/rom` to skip the launcher.

On Windows, run the same steps from Git Bash inside a "x64 Native Tools" prompt, and use `-DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl`.

## Layout
- `config/overlays.md`: the game's overlay table, recovered from its DMA loader `func_8001B1B4`
- `tools/gen_syms.py`: finds functions with spimdisasm and writes the N64Recomp symbol file
- `config/libultra_syms.txt`: libultra function names, found with n64sym and checked by hand
- `iss64.recomp.toml`, `aspMain.us.toml`: N64Recomp and RSPRecomp configs
- `src/`: the PC harness (window, renderer, audio, input, overlay registration, a few libultra stubs)
- `docs/rom-notes.md`: reverse-engineering notes

## Roadmap
1. Fix the missing main menu labels, then get through the menus into a match
2. Controller Pak saves (osPfsReSizeFile is stubbed for now)
3. Full match playable
4. PC extras (widescreen, 60fps, remapping)

Prior art: [djultra64/ISS2000](https://github.com/djultra64/ISS2000), the same approach for ISS 2000.
