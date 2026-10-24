// libultra functions the game calls that N64ModernRuntime doesn't provide.
#include <cstdint>

#include "recomp.h"

namespace {
    uint64_t get_u64_arg(gpr hi, gpr lo) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(hi)) << 32) | static_cast<uint32_t>(lo);
    }

    void set_u64_return(recomp_context* ctx, uint64_t value) {
        ctx->r2 = static_cast<int32_t>(value >> 32);
        ctx->r3 = static_cast<int32_t>(value);
    }
}

extern "C" {

// long long __ll_rshift(long long a, long long b): a in a0/a1, b in a2/a3.
void __ll_rshift_recomp(uint8_t* rdram, recomp_context* ctx) {
    int64_t a = static_cast<int64_t>(get_u64_arg(ctx->r4, ctx->r5));
    uint32_t shift = static_cast<uint32_t>(ctx->r7) & 63;
    set_u64_return(ctx, static_cast<uint64_t>(a >> shift));
}

// unsigned long long __f_to_ull(float f): f in f12.
void __f_to_ull_recomp(uint8_t* rdram, recomp_context* ctx) {
    float f = ctx->f12.fl;
    set_u64_return(ctx, f <= 0.0f ? 0 : static_cast<uint64_t>(f));
}

// The runtime emulates the Controller Pak inside osPfsInitPak and friends, so
// these lower-level calls only have to report success. Pak saving is not
// supported yet (see docs/rom-notes.md).

// s32 osPfsIsPlug(OSMesgQueue* queue, u8* pattern): no paks plugged in.
void osPfsIsPlug_recomp(uint8_t* rdram, recomp_context* ctx) {
    MEM_B(0, ctx->r5) = 0;
    ctx->r2 = 0;
}

// s32 __osGetId(OSPfs* pfs)
void __osGetId_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 0;
}

// s32 osPfsReSizeFile(OSPfs*, u16 company, u32 game, u8* name, u8* ext, int length)
void osPfsReSizeFile(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 0;
}

}
