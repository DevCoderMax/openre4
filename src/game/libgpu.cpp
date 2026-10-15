// game/libgpu: PlayStation ordering-table primitives drawn through GX (D:/Bio4/Prog/libgpu.cpp).
#include "types.h"
#include "vec.h"
#include "gx.h"
#include "libgpu.h"

#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
#define OT_TABLE_MAX 4096
static void* s_ot_table[OT_TABLE_MAX];
static u32 s_ot_count = 1; // 0 unused

void* OtLookup(u32 tag)
{
    if (tag == 0xFFFFFFFF || tag == 0) {
        return nullptr;
    }
    u32 idx = tag & 0x7FFFFFFF;
    if (idx < s_ot_count) {
        return s_ot_table[idx];
    }
    return nullptr;
}

u32 OtRegister(void* ptr, bool is_prim)
{
    if (!ptr) {
        return 0xFFFFFFFF;
    }
    for (u32 i = 1; i < s_ot_count; i++) {
        if (s_ot_table[i] == ptr) {
            return is_prim ? (i | 0x80000000) : (i & 0x7FFFFFFF);
        }
    }
    if (s_ot_count < OT_TABLE_MAX) {
        u32 idx = s_ot_count++;
        s_ot_table[idx] = ptr;
        return is_prim ? (idx | 0x80000000) : (idx & 0x7FFFFFFF);
    }
    return 0xFFFFFFFF;
}

void make_g3(uintptr_t tag);
void make_g4(uintptr_t tag);
void make_f3(uintptr_t tag);
void make_f4(uintptr_t tag);
void make_tile(uintptr_t tag);
void make_lg2(uintptr_t tag);
void make_lg3(uintptr_t tag);
void make_lg4(uintptr_t tag);
void make_lf2(uintptr_t tag);
void make_lf3(uintptr_t tag);
void make_lf4(uintptr_t tag);
#else
// The primitive is handed over as the OT tag word and cast at every access: a pointer kept in an
// integer variable has no REG_POINTER flag, so its loads may alias the stack stores of the make_f*
// conversions and stay in source order (a POLY_F3* parameter hoists all loads above the stores).
void make_g3(u32 tag);
void make_g4(u32 tag);
void make_f3(u32 tag);
void make_f4(u32 tag);
void make_tile(u32 tag);
void make_lg2(u32 tag);
void make_lg3(u32 tag);
void make_lg4(u32 tag);
void make_lf2(u32 tag);
void make_lf3(u32 tag);
void make_lf4(u32 tag);
#endif

// Links a primitive at the head of an ordering-table entry (PS1 libgpu AddPrim).
void AddPrim(u32* pOt, u32* pWk)
{
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
    *pWk = *pOt;
    *pOt = OtRegister(pWk, true);
#else
    *pWk = *pOt;
    *pOt = (u32) pWk;
#endif
}

// Unlinks a primitive from an ordering table chain.
void DelPrim(u32* pOt, u32* pWk)
{
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
    if (!pOt || *pOt == 0xFFFFFFFF) {
        return;
    }
    do {
        u32 tag = *pOt;
        u32* p = (u32*) OtLookup(tag);
        if ((s32) tag < 0 && p == pWk) {
            *pOt = *p;
            return;
        }
        pOt = (u32*) OtLookup(tag);
    } while (pOt && *pOt != 0xFFFFFFFF);
#else
    if (*pOt == 0xFFFFFFFF) {
        return;
    }
    do {
        u32* p = (u32*) *pOt;
        if ((s32) *pOt < 0 && p == pWk) {
            *pOt = *p;
            return;
        }
        pOt = (u32*) (*pOt | 0x80000000);
    } while (*pOt != 0xFFFFFFFF);
#endif
}

// Initialises a reverse ordering table of n entries (each pointing to the previous, the first terminated).
void ClearOTagR(u32* pOt, int n)
{
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
    int i;
    *pOt = 0xFFFFFFFF;
    for (i = 0; i < n - 1; i++) {
        pOt[1] = OtRegister(pOt, false);
        pOt++;
    }
#else
    int i;

    *pOt = 0xFFFFFFFF;
    for (i = 0; i < n - 1; i++) {
        pOt[1] = (u32) pOt & 0x7FFFFFFF;
        pOt++;
    }
#endif
}

// Draws the chain: screen-space ortho projection (512x448), then each primitive by its code
// (low 5 bits of word 1) through the make_* table. Used by the debug line/polygon drawing.
void DrawOTag(u32* pOt)
{
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
    static void (*tbl[])(uintptr_t) = {
        make_g3,
        make_g4,
        make_f3,
        make_f4,
        make_tile,
        make_lg2,
        make_lg3,
        make_lg4,
        make_lf2,
        make_lf3,
        make_lf4,
    };
#else
    static void (*tbl[])(u32) = {
        make_g3,
        make_g4,
        make_f3,
        make_f4,
        make_tile,
        make_lg2,
        make_lg3,
        make_lg4,
        make_lf2,
        make_lf3,
        make_lf4,
    };
#endif
    Mtx44 proj;
    Mtx mv;

    C_MTXOrtho(proj, 0.0f, 448.0f, 0.0f, 512.0f, 0.0f, -100.0f);
    GXSetProjection(proj, 1);
    PSMTXIdentity(mv);
    GXLoadPosMtxImm(mv, 0);
    GXSetCurrentMtx(0);
    GXSetBlendMode(0, 1, 0, 0);
    GXSetCullMode(0);
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
    if (!pOt || *pOt == 0xFFFFFFFF) {
        return;
    }
    do {
        u32 tag = *pOt;
        void* p = OtLookup(tag);
        if ((s32) tag < 0 && p) {
            u32* pu = (u32*) p;
            tbl[pu[1] & 0x1F]((uintptr_t) pu);
        }
        pOt = (u32*) OtLookup(tag);
    } while (pOt && *pOt != 0xFFFFFFFF);
#else
    if (*pOt == 0xFFFFFFFF) {
        return;
    }
    do {
        u32* p = (u32*) *pOt;
        if ((s32) p < 0) {
            tbl[p[1] & 0x1F]((u32) p);
        }
        pOt = (u32*) (*pOt | 0x80000000);
    } while (*pOt != 0xFFFFFFFF);
#endif
}

// GX state for one primitive: colour-only vertices, position + colour descriptors, GXBegin.
static inline void gpuSetup(int prim, int nverts)
{
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
    GXSetBlendMode(1, 1, 0, 0);
    GXSetCullMode(0);
    GXSetZMode(0, 3, 0);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(0, 0xFF, 0xFF, 4);
    GXSetTevOp(0, 4);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(11, 1);
    GXSetVtxAttrFmt(0, 9, 1, 3, 0);
    GXSetVtxAttrFmt(0, 11, 1, 5, 0);
    GXBegin(prim, 0, nverts);
}

// Gouraud triangle (POLY_G3) as a GX triangle with per-vertex colour.
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
void make_g3(uintptr_t tag)
#else
void make_g3(u32 tag)
#endif
{
#define p ((POLY_G3*) tag)
    gpuSetup(0x90, 3);
    GXPosition3s16(p->x0, p->y0, p->z0);
    GXColor4u8(p->c0.r, p->c0.g, p->c0.b, 0xFF);
    GXPosition3s16(p->x1, p->y1, p->z1);
    GXColor4u8(p->c1.r, p->c1.g, p->c1.b, 0xFF);
    GXPosition3s16(p->x2, p->y2, p->z2);
    GXColor4u8(p->c2.r, p->c2.g, p->c2.b, 0xFF);
}
#undef p

// Gouraud quad (POLY_G4).
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
void make_g4(uintptr_t tag)
#else
void make_g4(u32 tag)
#endif
{
#define p ((POLY_G4*) tag)
    gpuSetup(0x98, 4);
    GXPosition3s16(p->x0, p->y0, p->z0);
    GXColor4u8(p->c0.r, p->c0.g, p->c0.b, 0xFF);
    GXPosition3s16(p->x1, p->y1, p->z1);
    GXColor4u8(p->c1.r, p->c1.g, p->c1.b, 0xFF);
    GXPosition3s16(p->x2, p->y2, p->z2);
    GXColor4u8(p->c2.r, p->c2.g, p->c2.b, 0xFF);
    GXPosition3s16(p->x3, p->y3, p->z3);
    GXColor4u8(p->c3.r, p->c3.g, p->c3.b, 0xFF);
}
#undef p

// Flat triangle (POLY_F3): expanded to a gouraud triangle with one colour.
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
void make_f3(uintptr_t tag)
#else
void make_f3(u32 tag)
#endif
{
#define p ((POLY_F3*) tag)
    POLY_G3 g;

    g.x0 = p->x0;
    g.y0 = p->y0;
    g.z0 = p->z0;
    g.x1 = p->x1;
    g.y1 = p->y1;
    g.z1 = p->z1;
    g.x2 = p->x2;
    g.y2 = p->y2;
    g.z2 = p->z2;
    g.c0.r = g.c1.r = g.c2.r = p->c0.r;
    g.c0.g = g.c1.g = g.c2.g = p->c0.g;
    g.c0.b = g.c1.b = g.c2.b = p->c0.b;
    g.c0.cd = g.c1.cd = g.c2.cd = p->c0.cd;
    make_g3((uintptr_t) &g);
}
#undef p

// Flat quad (POLY_F4): expanded to a gouraud quad with one colour.
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
void make_f4(uintptr_t tag)
#else
void make_f4(u32 tag)
#endif
{
#define p ((POLY_F4*) tag)
    POLY_G4 g;

    g.x0 = p->x0;
    g.y0 = p->y0;
    g.z0 = p->z0;
    g.x1 = p->x1;
    g.y1 = p->y1;
    g.z1 = p->z1;
    g.x2 = p->x2;
    g.y2 = p->y2;
    g.z2 = p->z2;
    g.x3 = p->x3;
    g.y3 = p->y3;
    g.z3 = p->z3;
    g.c0.r = g.c1.r = g.c2.r = g.c3.r = p->c0.r;
    g.c0.g = g.c1.g = g.c2.g = g.c3.g = p->c0.g;
    g.c0.b = g.c1.b = g.c2.b = g.c3.b = p->c0.b;
    g.c0.cd = g.c1.cd = g.c2.cd = g.c3.cd = p->c0.cd;
    make_g4((uintptr_t) &g);
}
#undef p

// Axis-aligned rectangle (TILE) at x0,y0 with w x h as a flat quad.
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
void make_tile(uintptr_t tag)
#else
void make_tile(u32 tag)
#endif
{
#define p ((TILE*) tag)
    POLY_G4 g;

    g.x0 = g.x2 = p->x0;
    g.x1 = g.x3 = p->x0 + p->w;
    g.y0 = g.y1 = p->y0;
    g.y2 = g.y3 = p->y0 + p->h;
    g.z0 = g.z1 = g.z2 = g.z3 = p->z0;
    g.c0.r = g.c1.r = g.c2.r = g.c3.r = p->c0.r;
    g.c0.g = g.c1.g = g.c2.g = g.c3.g = p->c0.g;
    g.c0.b = g.c1.b = g.c2.b = g.c3.b = p->c0.b;
    g.c0.cd = g.c1.cd = g.c2.cd = g.c3.cd = p->c0.cd;
    make_g4((uintptr_t) &g);
}
#undef p

// Gouraud line strip of 2 points (LINE_G2).
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
void make_lg2(uintptr_t tag)
#else
void make_lg2(u32 tag)
#endif
{
#define p ((LINE_G2*) tag)
    gpuSetup(0xB0, 2);
    GXPosition3s16(p->x0, p->y0, p->z0);
    GXColor4u8(p->c0.r, p->c0.g, p->c0.b, 0xFF);
    GXPosition3s16(p->x1, p->y1, p->z1);
    GXColor4u8(p->c1.r, p->c1.g, p->c1.b, 0xFF);
}
#undef p

// Gouraud line strip of 3 points.
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
void make_lg3(uintptr_t tag)
#else
void make_lg3(u32 tag)
#endif
{
#define p ((LINE_G3*) tag)
    gpuSetup(0xB0, 3);
    GXPosition3s16(p->x0, p->y0, p->z0);
    GXColor4u8(p->c0.r, p->c0.g, p->c0.b, 0xFF);
    GXPosition3s16(p->x1, p->y1, p->z1);
    GXColor4u8(p->c1.r, p->c1.g, p->c1.b, 0xFF);
    GXPosition3s16(p->x2, p->y2, p->z2);
    GXColor4u8(p->c2.r, p->c2.g, p->c2.b, 0xFF);
}
#undef p

// Gouraud line strip of 4 points.
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
void make_lg4(uintptr_t tag)
#else
void make_lg4(u32 tag)
#endif
{
#define p ((LINE_G4*) tag)
    gpuSetup(0xB0, 4);
    GXPosition3s16(p->x0, p->y0, p->z0);
    GXColor4u8(p->c0.r, p->c0.g, p->c0.b, 0xFF);
    GXPosition3s16(p->x1, p->y1, p->z1);
    GXColor4u8(p->c1.r, p->c1.g, p->c1.b, 0xFF);
    GXPosition3s16(p->x2, p->y2, p->z2);
    GXColor4u8(p->c2.r, p->c2.g, p->c2.b, 0xFF);
    GXPosition3s16(p->x3, p->y3, p->z3);
    GXColor4u8(p->c3.r, p->c3.g, p->c3.b, 0xFF);
}
#undef p

// Flat 2-point line, expanded to LINE_G2.
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
void make_lf2(uintptr_t tag)
#else
void make_lf2(u32 tag)
#endif
{
#define p ((LINE_F2*) tag)
    LINE_G2 g;

    g.x0 = p->x0;
    g.y0 = p->y0;
    g.z0 = p->z0;
    g.x1 = p->x1;
    g.y1 = p->y1;
    g.z1 = p->z1;
    g.c0.r = g.c1.r = p->c0.r;
    g.c0.g = g.c1.g = p->c0.g;
    g.c0.b = g.c1.b = p->c0.b;
    g.c0.cd = g.c1.cd = p->c0.cd;
    make_lg2((uintptr_t) &g);
}
#undef p

// Flat 3-point line strip, expanded to LINE_G3.
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
void make_lf3(uintptr_t tag)
#else
void make_lf3(u32 tag)
#endif
{
#define p ((LINE_F3*) tag)
    LINE_G3 g;

    g.x0 = p->x0;
    g.y0 = p->y0;
    g.z0 = p->z0;
    g.x1 = p->x1;
    g.y1 = p->y1;
    g.z1 = p->z1;
    g.x2 = p->x2;
    g.y2 = p->y2;
    g.z2 = p->z2;
    g.c0.r = g.c1.r = g.c2.r = p->c0.r;
    g.c0.g = g.c1.g = g.c2.g = p->c0.g;
    g.c0.b = g.c1.b = g.c2.b = p->c0.b;
    g.c0.cd = g.c1.cd = g.c2.cd = p->c0.cd;
    make_lg3((uintptr_t) &g);
}
#undef p

// Flat 4-point line strip, expanded to LINE_G4.
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
void make_lf4(uintptr_t tag)
#else
void make_lf4(u32 tag)
#endif
{
#define p ((LINE_F4*) tag)
    LINE_G4 g;

    g.x0 = p->x0;
    g.y0 = p->y0;
    g.z0 = p->z0;
    g.x1 = p->x1;
    g.y1 = p->y1;
    g.z1 = p->z1;
    g.x2 = p->x2;
    g.y2 = p->y2;
    g.z2 = p->z2;
    g.x3 = p->x3;
    g.y3 = p->y3;
    g.z3 = p->z3;
    g.c0.r = g.c1.r = g.c2.r = g.c3.r = p->c0.r;
    g.c0.g = g.c1.g = g.c2.g = g.c3.g = p->c0.g;
    g.c0.b = g.c1.b = g.c2.b = g.c3.b = p->c0.b;
    g.c0.cd = g.c1.cd = g.c2.cd = g.c3.cd = p->c0.cd;
    make_lg4((uintptr_t) &g);
}
#undef p
