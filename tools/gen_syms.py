"""Find functions in each code segment with spimdisasm and write an N64Recomp symbol file.

Usage: gen_syms.py ROM OVERLAY_TABLE NAMES OUT
"""
import re, struct, subprocess, sys, os, tempfile
rom_path, ov_table, names_path, out = sys.argv[1:5]
names = {int(a, 16): n for n, a in re.findall(r"^(\w+) = 0x([0-9A-Fa-f]+);", open(names_path).read(), re.M)}
rom = open(rom_path, 'rb').read()
# Main text ends where rspboot begins (0x80090430); the rest of main is data and microcode.
segs = [("entry_main", 0x1000, 0x91030, 0x80000400)]
for line in open(ov_table):
    m = re.match(r"\| 0x([0-9A-F]+) \| 0x([0-9A-F]+) \| 0x([0-9A-F]+) \| 0x[0-9A-F]+ \| (\d+)", line)
    if m:
        r0, r1, v, jr = int(m[1],16), int(m[2],16), int(m[3],16), int(m[4])
        if jr >= 10:
            segs.append((f"ovl_{r0:06X}", r0, r1, v))

def text_end(r0, r1):
    last = r0
    for i in range(r0, r1, 4):
        if rom[i:i+4] == b'\x03\xe0\x00\x08':
            last = i + 8
    return (last + 15) & ~15

lines = []
for name, r0, r1, v in segs:
    te = r1 if name == "entry_main" else text_end(r0, r1)
    with tempfile.TemporaryDirectory() as d:
        subprocess.run([sys.executable, "-m", "spimdisasm", "singleFileDisasm", rom_path, d,
                        "--start", hex(r0), "--end", hex(te), "--vram", hex(v)],
                       check=True, capture_output=True)
        text = "".join(open(os.path.join(dp, f)).read() for dp, _, fs in os.walk(d) for f in fs)
    starts = sorted({int(x, 16) for x in re.findall(r"glabel func_([0-9A-F]{8})", text)} |
                    {int(x, 16) for x in re.findall(r"glabel \w+\s*\n.*?/\* [0-9A-F]+ ([0-9A-F]{8})", text)})
    # Named symbols are known function starts even when spimdisasm merges them (e.g. tiny leaf functions like sqrtf).
    starts = sorted(set(starts) | set(names))
    starts = [s for s in starts if v <= s < v + (te - r0)]
    lines.append(f'[[section]]\nname = "{name}"\nrom = 0x{r0:08X}\nvram = 0x{v:08X}\nsize = 0x{te-r0:X}\n\nfunctions = [')
    for a, b in zip(starts, starts[1:] + [v + te - r0]):
        lines.append(f'    {{ name = "{names.get(a, f'func_{a:08X}')}", vram = 0x{a:08X}, size = 0x{b-a:X} }},')
    lines.append(']\n')
    print(name, hex(r0), hex(te), hex(v), len(starts), "funcs", file=sys.stderr)
open(out, 'w').write("\n".join(lines))
