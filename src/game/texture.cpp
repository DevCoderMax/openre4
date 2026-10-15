// game/texture: cTexSys, a registry of texture slots indexed by texture id, loaded and released per
// owner. Used for the room textures (room_tex.cpp) and the id / cockpit textures (id_sys).
#include "types.h"
#include "vec.h"
#include "gx.h"
#include "tpl.h"
#include "db_log.h"
#include "main_mem.h"
#include "texture.h"
#include <stdlib.h>

#line 20 "D:/Bio4/Prog/texture.cpp"

int lod_enable = 0;
int tex_dummy = 0;


// Creates a registry `name` with a pool of `num` GX texture objects and their in-use bitmap.
void cTexSys::Init(const char* name, u32 max)
{
    u32 i;

    this->m_name = name;
    nTexObj = max;
#line 53
    if ((pTexObj = (GXTexObj*) MEM_ALLOC(max * sizeof(GXTexObj), 1, 0xD)) == NULL) {
        nTexObj = 0;
        pLog->err(0, 0, "%s::Init(): Memory Allocation Failed.", this->m_name);
        return;
    }
#line 60
    if ((pFlag = (u8*) MEM_ALLOC(nTexObj / 8 + 1, 1, 0xD)) == NULL) {
        nTexObj = 0;
        pLog->err(0, 0, "%s::Init(): Memory Allocation Failed.", this->m_name);
        return;
    }
    for (i = 0; i < nTexObj; i++) {
        SetTexObjFlag(i, 0);
    }
    Clear();
}

// Drops every registered texture and frees the whole object pool.
void cTexSys::Clear()
{
    u32 i;
    TEX_WK* w = m_texw_array;

    for (i = 0; i < 256; i++, w++) {
        w->Owner = TEX_OWNER_NONE;
    }
    x5408 = 0;
    for (i = 0; i < nTexObj; i++) {
        SetTexObjFlag(i, 0);
    }
}

// In-use bit of pool object `no`.
int cTexSys::GetTexObjFlag(u32 no)
{
    if (no >= nTexObj) {
        pLog->err(0, 0, "%s::GetTexObjFlag : TexNo over [%d/%d]", m_name, no, nTexObj);
        return 0;
    }
    if ((pFlag[no >> 3] >> (no & 7)) & 1) {
        return 1;
    }
    return 0;
}

// Sets / clears the in-use bit of pool object `no`.
void cTexSys::SetTexObjFlag(u32 no, int flag)
{
    u8 bit;

    if (no >= nTexObj) {
        pLog->err(0, 0, "%s::GetTexObjFlag : TexNo over [%d/%d]", m_name, no, nTexObj);
    }
    bit = 1 << (no & 7);
    if (flag == 1) {
        pFlag[no >> 3] |= bit;
    } else {
        pFlag[no >> 3] &= ~bit;
    }
}

// Registers every texture of a TexData file (version 3) under `owner`; `clamp` forces clamped
// (non-repeating) textures. Returns 0 on a bad file.
int cTexSys::DataLoad(TexData* data, u32 owner, int clamp)
{
    if (!data) {
        return 0;
    }
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
    u8* base = (u8*) data;
    u32 ver = data->version;
    bool is_be = (ver == 0x03000000);
    if (is_be) {
        ver = __builtin_bswap32(ver);
    }
    if (ver != 3) {
        pLog->err(0, 0, "%s::DataLoad() : Data Invalid. [0x%p]", m_name, data);
        return 0;
    }
    u32 oI = is_be ? __builtin_bswap32(data->ofsId) : data->ofsId;
    u32 oT = is_be ? __builtin_bswap32(data->ofsTpl) : data->ofsTpl;
    u32 oA = is_be ? __builtin_bswap32(data->ofsAnm) : data->ofsAnm;

    TexIdTbl* ids = (TexIdTbl*) (base + oI);
    TexOfsTbl* tpls = (TexOfsTbl*) (base + oT);
    TexOfsTbl* anms = (TexOfsTbl*) (base + oA);

    u32 num = is_be ? __builtin_bswap32(ids->num) : ids->num;
    for (u32 i = 0; i < num; i++) {
        u32 t_ofs = is_be ? __builtin_bswap32(tpls->ofs[i]) : tpls->ofs[i];
        u32 a_ofs = is_be ? __builtin_bswap32(anms->ofs[i]) : anms->ofs[i];
        TEXPalette* tpl = (TEXPalette*) ((u8*) tpls + t_ofs);
        TexAnm* anm = (TexAnm*) ((u8*) anms + a_ofs);
        u16 id = is_be ? __builtin_bswap16(ids->ent[i].id) : ids->ent[i].id;
        TexRegist(tpl, anm, id, owner, clamp, 1);
    }
    return 1;
#else
    TexIdTbl* ids;
    TexOfsTbl* tpls;
    TexOfsTbl* anms;
    u32 i;
    // COMPILER-DIFF: candidate (local-alloc qty order). The target names the three offset
    // temporaries r9/r11/r0 (ofsId/ofsTpl/ofsAnm), ours r0/r9/r0: a fake-lifetime tie decided by
    // the sched1 position of the third `lwz` (30 statement/base/order forms tried); the two pins
    // give the target's names with the same schedule.
    register u32 oI;
    register u32 oT;

    if (data->version != 3) {
        pLog->err(0, 0, "%s::DataLoad() : Data Invalid. [0x%x]", m_name, data);
        return 0;
    }
    oI = data->ofsId;
    ids = (TexIdTbl*) ((u8*) data + oI);
    oT = data->ofsTpl;
    tpls = (TexOfsTbl*) ((u8*) data + oT);
    anms = (TexOfsTbl*) ((u8*) data + data->ofsAnm);
    for (i = 0; i < ids->num; i++) {
        TEXPalette* tpl = (TEXPalette*) ((u8*) tpls + tpls->ofs[i]);
        TexAnm* anm = (TexAnm*) ((u8*) anms + anms->ofs[i]);
        u16 id = ids->ent[i].id;
        TexRegist(tpl, anm, id, owner, clamp, 1);
    }
    return 1;
#endif
}

// `num` consecutive free pool objects, marked in use; NULL when the pool is full.
GXTexObj* cTexSys::PullTexObj(u32 num)
{
    u32 start = 0;
    u32 cnt = 0;
    u32 i;
    GXTexObj* obj;

    while (cnt != num) {
        if (GetTexObjFlag(start + cnt) == 0) {
            cnt++;
        } else {
            start++;
            cnt = 0;
        }
        if (start + cnt >= nTexObj) {
            goto full;
        }
    }
    obj = &pTexObj[start];
    for (i = 0; i < num; i++) {
        SetTexObjFlag(start + i, 1);
    }
    goto done;

full:
    pLog->err(0, 0, "%s::PullTexObj(): TEXOBJ MAX!!", m_name);
    return NULL;

done:
    return obj;
}

#pragma pack(push, 1)
struct TPL_Disc_Desc {
    u32 texHeaderOfs;
    u32 clutHeaderOfs;
};

struct TPL_Disc_TexHeader {
    u16 height;
    u16 width;
    u32 format;
    u32 dataOfs;
    u32 wrapS;
    u32 wrapT;
    u32 minFilter;
    u32 magFilter;
    f32 LODBias;
    u8 edgeLODEnable;
    u8 minLOD;
    u8 maxLOD;
    u8 unpacked;
};

struct TPL_Disc_ClutHeader {
    u16 numEntries;
    u8 unpacked;
    u8 pad8;
    u32 format;
    u32 dataOfs;
};
#pragma pack(pop)

void UnpackTplNative(TEXPalette* tpl)
{
    if (tpl == NULL) {
        return;
    }
    if (tpl->version == 0x12345678) {
        return;
    }

    bool is_big_endian = (tpl->version == 0x30AF2000 || tpl->version == 0x0020AF30);
    u32 numDescriptors = tpl->numDescriptors;
    u32 desc_ofs = (u32)(uintptr_t)tpl->descriptorArray;
    if (tpl->version == 0x30AF2000) {
        numDescriptors = __builtin_bswap32(numDescriptors);
        desc_ofs = __builtin_bswap32(desc_ofs);
    }

    TEXDescriptor* native_descs = (TEXDescriptor*) calloc(numDescriptors, sizeof(TEXDescriptor));
    u8* base = (u8*) tpl;
    TPL_Disc_Desc* disc_descs = (TPL_Disc_Desc*) (base + desc_ofs);

    for (u32 i = 0; i < numDescriptors; i++) {
        u32 th_ofs = disc_descs[i].texHeaderOfs;
        u32 clut_ofs = disc_descs[i].clutHeaderOfs;
        if (tpl->version == 0x30AF2000) {
            th_ofs = __builtin_bswap32(th_ofs);
            clut_ofs = __builtin_bswap32(clut_ofs);
        }

        if (th_ofs != 0) {
            TPL_Disc_TexHeader* dth = (TPL_Disc_TexHeader*) (base + th_ofs);
            TEXHeader* th = (TEXHeader*) calloc(1, sizeof(TEXHeader));
            if (tpl->version == 0x30AF2000) {
                th->height = __builtin_bswap16(dth->height);
                th->width = __builtin_bswap16(dth->width);
                th->format = __builtin_bswap32(dth->format);
                u32 dataOfs = __builtin_bswap32(dth->dataOfs);
                th->data = base + dataOfs;
                th->wrapS = __builtin_bswap32(dth->wrapS);
                th->wrapT = __builtin_bswap32(dth->wrapT);
                th->minFilter = __builtin_bswap32(dth->minFilter);
                th->magFilter = __builtin_bswap32(dth->magFilter);
                th->LODBias = dth->LODBias;
            } else {
                th->height = dth->height;
                th->width = dth->width;
                th->format = dth->format;
                th->data = base + dth->dataOfs;
                th->wrapS = dth->wrapS;
                th->wrapT = dth->wrapT;
                th->minFilter = dth->minFilter;
                th->magFilter = dth->magFilter;
                th->LODBias = dth->LODBias;
            }
            th->edgeLODEnable = dth->edgeLODEnable;
            th->minLOD = dth->minLOD;
            th->maxLOD = dth->maxLOD;
            th->unpacked = 1;
            native_descs[i].textureHeader = th;
        }

        if (clut_ofs != 0) {
            TPL_Disc_ClutHeader* dclut = (TPL_Disc_ClutHeader*) (base + clut_ofs);
            CLUTHeader* clut = (CLUTHeader*) calloc(1, sizeof(CLUTHeader));
            if (tpl->version == 0x30AF2000) {
                clut->numEntries = __builtin_bswap16(dclut->numEntries);
                clut->format = (GXTlutFmt) __builtin_bswap32(dclut->format);
                u32 cdataOfs = __builtin_bswap32(dclut->dataOfs);
                clut->data = base + cdataOfs;
            } else {
                clut->numEntries = dclut->numEntries;
                clut->format = (GXTlutFmt) dclut->format;
                clut->data = base + dclut->dataOfs;
            }
            clut->unpacked = 1;
            native_descs[i].CLUTHeader = clut;
        }
    }

    tpl->numDescriptors = numDescriptors;
    tpl->descriptorArray = native_descs;
    tpl->version = 0x12345678;
}

// Relocates a TPL's offsets into pointers (once: skipped when the descriptor pointer is already a
// real address).
void cTexSys::CalcTplAddr(TEXPalette* tpl)
{
#if defined(OPENRE4_NATIVE)
    UnpackTplNative(tpl);
#else
    u32 i;
    TEXDescriptor* desc;

    if (tpl == NULL) {
        return;
    }
    if ((s32) tpl->descriptorArray < 0) {
        return;
    }
    tpl->descriptorArray = (TEXDescriptor*) ((u32) tpl->descriptorArray + (u32) tpl);
    for (i = 0; i < tpl->numDescriptors; i++) {
        desc = &tpl->descriptorArray[i];
        desc->textureHeader = (TEXHeader*) ((u8*) tpl + (u32) desc->textureHeader);
        desc->textureHeader->data = (u8*) tpl + (u32) desc->textureHeader->data;
        if (desc->CLUTHeader != NULL) {
            desc->CLUTHeader = (CLUTHeader*) ((u8*) tpl + (u32) desc->CLUTHeader);
            desc->CLUTHeader->data = (u8*) tpl + (u32) desc->CLUTHeader->data;
        }
    }
#endif
}

// Registers texture `id`: pool objects for the animation's frame count, one GX texture object per
// TPL image (repeat wrap when power-of-two and not clamped; CI formats also load the palette),
// optional LOD setup. 0 when the id is taken (error when `check`) or the pool is full.
int cTexSys::TexRegist(TEXPalette* tpl, TexAnm* anm, u8 id, u32 owner, int clamp, int check)
{
    TEX_WK* w = &m_texw_array[id];
    TEXDescriptor* desc;
    TEXHeader* hdr;
    GXTexObj* obj;
    int i;

    if (w->Owner != 0) {
        if (check != 0) {
            pLog->err(0, 0, "%s::TexRegist():TexId[%x] id already used.", m_name, id);
        }
        return 0;
    }
    CalcTplAddr(tpl);
    desc = TEXGet(tpl, 0);
    u16 nTex = anm->numTex;
#if defined(OPENRE4_NATIVE) || defined(TARGET_PC)
    if (nTex > 255) {
        nTex = __builtin_bswap16(nTex);
        anm->numTex = nTex;
    }
#endif
    w->nTexObj = nTex;
    w->pTex_obj_start = PullTexObj(w->nTexObj);
    if (w->pTex_obj_start == NULL) {
        pLog->err(0, 0, "%s : ID[%02x] PullTexObj() work full!!", m_name, id);
        return 0;
    }
    w->texHdr = desc->textureHeader;
    w->Anm_addr = anm;
    w->Owner = (TEX_OWNER) owner;
    w->Tpl_addr = tpl;
    for (i = 0; i < w->nTexObj; i++) {
        obj = &w->pTex_obj_start[i];
        desc = TEXGet(tpl, i);
        hdr = desc->textureHeader;
        if (hdr->format - 8 <= 1) {
            if (clamp == 0 && ((hdr->width & (hdr->width - 1)) == 0) && ((hdr->height & (hdr->height - 1)) == 0)) {
                GXInitTexObjCI(obj, hdr->data, hdr->width, hdr->height, hdr->format, 1, 1, 0, 0);
            } else {
                GXInitTexObjCI(obj, desc->textureHeader->data, desc->textureHeader->width, desc->textureHeader->height,
                               desc->textureHeader->format, 0, 0, 0, 0);
            }
            GXInitTlutObj(&w->tlut, desc->CLUTHeader->data, desc->CLUTHeader->format, desc->CLUTHeader->numEntries);
            GXLoadTlut(&w->tlut, 0);
        } else {
            if (clamp == 0 && ((hdr->width & (hdr->width - 1)) == 0) && ((hdr->height & (hdr->height - 1)) == 0)) {
                GXInitTexObj(obj, hdr->data, hdr->width, hdr->height, hdr->format, 1, 1, 0);
            } else {
                GXInitTexObj(obj, desc->textureHeader->data, desc->textureHeader->width, desc->textureHeader->height,
                             desc->textureHeader->format, 0, 0, 0);
            }
        }
        if (lod_enable) {
            TEXHeader* h = desc->textureHeader;
            GXInitTexObjLOD(obj, 0, 0, (f32) h->minLOD, (f32) h->maxLOD, h->LODBias, 0, h->edgeLODEnable, 0);
        }
    }
    PSMTXIdentity(w->_Mtx);
    return 1;
}

// TPL of texture `id`; 0 when unregistered.
int cTexSys::GetTplAddr(u32 id, TEXPalette** tpl_addr)
{
    TEX_WK* w = &m_texw_array[id];

    if (w->Owner == TEX_OWNER_NONE) {
        return 0;
    }
    *tpl_addr = w->Tpl_addr;
    return 1;
}

// GX texture object `no` (animation frame) of texture `id`; 0 when unregistered.
int cTexSys::GetTexObj(u32 id, u32 no, GXTexObj** texobj)
{
    TEX_WK* w = &m_texw_array[id];

    if (w->Owner == TEX_OWNER_NONE) {
        return 0;
    }
    *texobj = &w->pTex_obj_start[no];
    return 1;
}

// Animation table of texture `id`; 0 when unregistered.
int cTexSys::GetAnmAddr(u32 id, TexAnm** anm)
{
    TEX_WK* w = &m_texw_array[id];

    if (w->Owner == TEX_OWNER_NONE) {
        return 0;
    }
    *anm = w->Anm_addr;
    return 1;
}

// Palette object of a CI texture `id`; 0 when unregistered or not paletted.
int cTexSys::GetTlutObj(u32 id, GXTlutObj** out)
{
    TEX_WK* w = &m_texw_array[id];

    if (w->Owner == TEX_OWNER_NONE) {
        return 0;
    }
    if (w->texHdr->format - 8 <= 1) {
        *out = &w->tlut;
        return 1;
    }
    *out = NULL;
    return 0;
}

// The registry slot of texture `id`, NULL (error unless `quiet`) when unregistered.
TEX_WK* cTexSys::GetTexWk(u32 id, int bNoDispErrMsg)
{
    TEX_WK* w = &m_texw_array[id];

    if (w->Owner != 0) {
        return w;
    }
    if (bNoDispErrMsg == 0) {
        pLog->err(0, 0, "GetTexWk(): TexId[%x] No such texture", id);
    }
    return NULL;
}

// Unregisters every texture of `owner` and frees its pool objects. Returns the count released.
int cTexSys::TexRelease(u32 owner)
{
    TEX_WK* w;
    u32 i;
    u32 j;
    u32 base;

    for (w = m_texw_array, i = 0; i < 256; w++, i++) {
        if (w->Owner == owner) {
            w->Owner = TEX_OWNER_NONE;
            base = w->pTex_obj_start - pTexObj;
            for (j = base; j < base + w->nTexObj; j++) {
                SetTexObjFlag(j, 0);
            }
        }
    }
    return 1;
}
