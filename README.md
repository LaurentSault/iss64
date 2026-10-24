# ISS64 PC port

A native PC port of **International Superstar Soccer 64** (N64, USA), built by statically recompiling the game's code with [N64Recomp](https://github.com/N64Recomp/N64Recomp). It will run on [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime) and render with [RT64](https://github.com/rt64/rt64).

**This repository contains no game code or data.** You supply your own ROM, and everything is generated from it locally.

## Status
Early. The whole game's MIPS code recompiles to C: 5,655 functions across main and 15 code overlays. The runtime glue and renderer are not hooked up yet, so nothing runs yet.

## Requirements
- `International Superstar Soccer 64 (U) [!].z64`, SHA1 `d2c258ee3844be77049e4af5208f8f2bb073a86e`
- Python 3.10+, CMake, a C/C++ compiler, git

## Recompile
```sh
cp "/path/to/International Superstar Soccer 64 (U) [!].z64" baserom.us.z64
tools/recompile.sh
```
This writes the generated C to `build/RecompiledFuncs/`.

## Layout
- `config/overlays.md`: the game's overlay table, recovered from its DMA loader `func_8001B1B4`
- `tools/gen_syms.py`: finds functions with spimdisasm and writes the N64Recomp symbol file
- `iss64.recomp.toml`: N64Recomp config, including the libultra OS functions the runtime has to replace
- `docs/rom-notes.md`: reverse-engineering notes

## Roadmap
1. Identify libultra functions and map them to N64ModernRuntime
2. Runtime glue: overlay loading, RSP audio microcode, input
3. RT64 rendering, then boot to the title screen
4. Full match playable, then PC extras (widescreen, 60fps, remapping)

Prior art: [djultra64/ISS2000](https://github.com/djultra64/ISS2000), the same approach for ISS 2000.
