# ISS64 (USA) ROM notes

- File: International Superstar Soccer 64 (U) [!].z64, 8 MiB, big-endian .z64
- SHA1 d2c258ee3844be77049e4af5208f8f2bb073a86e, MD5 6a345402ae1db5ce1041365e36126bce
- Game code NJPE, internal name "I S S 64", CIC-6102 boot code
- Entrypoint 0x80000400 clears BSS 0x800DC890 (size 0xDD4E0) then jumps to main 0x8001A070, sp 0x801984D8
- Main segment uncompressed: rom 0x1050..0xDD490 -> vram 0x80000450 (text to ~rom 0x91260, then data)
- Microcode strings: F3DEX 1.21 and F3DLX.Rej 1.21 (both supported by RT64)
- Compiler assumed IDO (splat default; to be confirmed)

## Overlay table (exact)
All overlays are loaded by `func_8001B1B4(rom_start, vram, size)`, a DMA wrapper in main. Together they cover ROM 0x0DD490..0x203E90 contiguously.
A high `jr ra` count means the overlay holds code; 0 means it's data only.
Overlays that share a VRAM (for example the two at 0x803F0000) are swapped in and out, so N64Recomp has to treat them as relocatable sections.

| ROM start | ROM end | VRAM | Size | jr ra count | Loaded by |
|---|---|---|---|---|---|
| 0x0DD490 | 0x110130 | 0x8024B800 | 0x32CA0 | 635 | 0x80027414 |
| 0x110130 | 0x11BA10 | 0x8033EE00 | 0xB8E0 | 12 | 0x8001b7d4 |
| 0x11BA10 | 0x126F20 | 0x803BB200 | 0xB510 | 43 | 0x8003a29c |
| 0x126F20 | 0x132960 | 0x803C6710 | 0xBA40 | 0 | 0x8003a1c4 |
| 0x132960 | 0x13D260 | 0x803D2150 | 0xA900 | 0 | 0x8003a20c |
| 0x13D260 | 0x146C90 | 0x803DCA50 | 0x9A30 | 0 | 0x8003a254 |
| 0x146C90 | 0x14D690 | 0x803E6480 | 0x6A00 | 0 | 0x8003a2e4 |
| 0x14D690 | 0x151880 | 0x80396A00 | 0x41F0 | 115 | 0x8003a32c |
| 0x151880 | 0x158B30 | 0x8039ABF0 | 0x72B0 | 167 | 0x8003a374 |
| 0x158B30 | 0x15EF00 | 0x803A1EF0 | 0x63D0 | 167 | 0x8003a3bc |
| 0x15EF00 | 0x164450 | 0x803A8320 | 0x5550 | 147 | 0x8003a404 |
| 0x164450 | 0x167810 | 0x803AD8C0 | 0x33C0 | 82 | 0x8003a44c |
| 0x167810 | 0x16CFC0 | 0x803B0CA0 | 0x57B0 | 110 | 0x8003a494 |
| 0x16CFC0 | 0x1728B0 | 0x80264800 | 0x58F0 | 134 | 0x8003a4dc |
| 0x1728B0 | 0x177C00 | 0x8026A0F0 | 0x5350 | 160 | 0x8003a524 |
| 0x177C00 | 0x17D9B0 | 0x8026F480 | 0x5DB0 | 114 | 0x8003a0ec |
| 0x17D9B0 | 0x190D00 | 0x80275230 | 0x13350 | 377 | 0x8003a134 |
| 0x190D00 | 0x198B00 | 0x803F0000 | 0x7E00 | 70 | 0x80027478 |
| 0x198B00 | 0x1A4EE0 | 0x803F0000 | 0xC3E0 | 285 | 0x8003a17c |
| 0x1A4EE0 | 0x1C3400 | 0x801E1300 | 0x1E520 | 23 | 0x8001b308 |
| 0x1C3400 | 0x1F9700 | 0x8028B800 | 0x36300 | 628 | 0x8001b328 |
| 0x1F9700 | 0x203E90 | 0x802D0000 | 0xA790 | 0 | 0x8001b2e8 |

- 0x203E90+ is high-entropy (compressed assets), format unknown yet.

## Prior art
- No public ISS64 decomp/recomp found (Oct 2026).
- djultra64/ISS2000 on GitHub: N64Recomp + N64ModernRuntime + RT64 port of ISS 2000 (sibling Konami title), GPL-3.0, user-supplied ROM. Best template.

## Recompilation status (2026-10-24)
- `research/recomp/gen_syms.py` uses spimdisasm to find functions in main plus 15 code overlays and writes an N64Recomp symbol file. It finds 5,655 functions.
- N64Recomp (commit ffb39cd) recompiles all of them into about 30 MB of C, except 14 low-level libultra OS functions (cache ops, cop0, the exception handler, the ll<->float helpers). Those are in the `ignored` list in `iss64.recomp.toml`, and N64ModernRuntime will provide replacements.
- Every generated C file passes a clang syntax check. The only errors are calls to those 14 ignored functions.
- Next: identify libultra functions (osCreateThread, osPiStartDma, osContStartReadData, the audio library, etc.) and map them to N64ModernRuntime. Then write the runtime glue for the overlay DMA loader `func_8001B1B4`.
