// Hands N64Recomp's section and overlay tables to librecomp, and tells it when
// the game's overlay loader (func_8001B1B4) swaps code in and out of RAM.
#include <algorithm>

#include "recomp_overlays.inl"

#include "librecomp/overlays.hpp"

extern "C" void load_overlays(uint32_t rom, int32_t ram_addr, uint32_t size);
extern "C" void unload_overlays(int32_t ram_addr, uint32_t size);

namespace iss64 {
    void register_overlays() {
        recomp::overlays::overlay_section_table_data_t sections {
            .code_sections = section_table,
            .num_code_sections = ARRLEN(section_table),
            .total_num_sections = num_sections,
        };

        recomp::overlays::overlays_by_index_t overlays {
            .table = overlay_sections_by_index,
            .len = ARRLEN(overlay_sections_by_index),
        };

        recomp::overlays::register_overlays(sections, overlays);
    }
}

// Called from a hook at the end of func_8001B1B4(rom, vram, size), after the DMA.
extern "C" void iss64_overlay_loaded(uint32_t rom, uint32_t vram, uint32_t size) {
    // The DMA overwrote any code that was in [vram, vram + size). librecomp
    // refuses to unload part of a section, and overlays that share RAM differ in
    // size, so widen the range to cover every section the DMA touched.
    uint32_t start = vram;
    uint32_t end = vram + size;
    for (const SectionTableEntry& section : section_table) {
        if (section.ram_addr < vram + size && section.ram_addr + section.size > vram) {
            start = std::min(start, section.ram_addr);
            end = std::max(end, section.ram_addr + section.size);
        }
    }
    unload_overlays(static_cast<int32_t>(start), end - start);
    load_overlays(rom, static_cast<int32_t>(vram), size);
}
