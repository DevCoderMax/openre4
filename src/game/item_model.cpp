// game/item_model: item pick-up models (D:/Bio4/Prog/item_model.cpp). The room's "ITM" block packs
// bin/tpl model pairs per item id; cItmSys (g_pItemModelSys, 0x100 entries) registers them and the
// item objects (obj16 etc.) fetch them with ItemGetBinTplAddr.
#include "item_model.h"
#include "main_mem.h"
#include <cstdio>
#include <cstdlib>

cItmSys* g_pItemModelSys;

// Item model pack: offsets from the pack start to the id table, the model table and the
// texture table.
struct ItemModelPack {
    u32 x0;      // 0x00
    u32 ofsId;   // 0x04
    u32 ofsBin;  // 0x08
    u32 ofsTpl;  // 0x0C
};

struct ItemModelId {
    u16 id;      // 0x00
    u16 pad_2;
    u32 pad_4;
};

struct ItemModelIdTbl {
    u32 num;            // 0x00
    ItemModelId id[1];  // 0x04
};

// model / texture tables: offsets from the table start
struct ItemModelOfsTbl {
    u32 num;     // 0x00
    u32 ofs[1];  // 0x04
};

// Boot: same as the room init.
void ItemModelInit()
{
    ItemModelRoomInit();
}

// Room init: allocates a fresh (empty) item model table.
void ItemModelRoomInit()
{
    cItmSys* sys;

#line 94 "D:/Bio4/Prog/item_model.cpp"
    sys = (cItmSys*) MEM_ALLOC(sizeof(cItmSys), 1, 13);
#if defined(OPENRE4_NATIVE)
    if (!sys) {
        printf("[OpenRE4 Debug] ItemModelRoomInit: MEM_ALLOC failed, using malloc fallback\n");
        fflush(stdout);
        sys = (cItmSys*) malloc(sizeof(cItmSys));
    }
#endif
    g_pItemModelSys = sys;
    if (sys) {
        sys->Init();
    }
}

// Registers every model of the room's ITM pack.
int ItemModelDataLoad(void* data)
{
    if (!g_pItemModelSys || !data) {
        return 0;
    }
    return g_pItemModelSys->DataLoad((uintptr_t) data);
}

// Model binary of item `no`; 0 when the room has none.
int ItemGetBinAddr(u8 id, void** pBin_addr)
{
    return g_pItemModelSys->GetBinAddr(id, pBin_addr);
}

// Texture palette of item `no`; 0 when the room has none.
int ItemGetTplAddr(u8 id, void** pTpl_addr)
{
    return g_pItemModelSys->GetTplAddr(id, pTpl_addr);
}

// Both model files of item `no`; 0 when either is missing.
int ItemGetBinTplAddr(u8 id, void** pBin_addr, void** pTpl_addr)
{
    if (ItemGetBinAddr(id, pBin_addr) == 0) {
        return 0;
    }
    if (ItemGetTplAddr(id, pTpl_addr) == 0) {
        return 0;
    }
    return 1;
}

// Clears the 0x100 bin/tpl entries.
void cItmSys::WorkClear()
{
    int i;

    for (i = 0; i < 0x100; i++) {
        work[i].bin = 0;
        work[i].tpl = 0;
    }
}

// Clears the table.
void cItmSys::Init()
{
    WorkClear();
}

// Walks the pack's id / bin offset / tpl offset tables and registers each item id.
int cItmSys::DataLoad(uintptr_t data_addr)
{
    if (!data_addr) {
        return 0;
    }
    ItemModelPack* pack = (ItemModelPack*) data_addr;
#if defined(OPENRE4_NATIVE)
    u32 ofsId = pack->ofsId;
    u32 ofsBin = pack->ofsBin;
    u32 ofsTpl = pack->ofsTpl;
    if (ofsId > 0x1000000 || ofsBin > 0x1000000 || ofsTpl > 0x1000000) {
        ofsId = __builtin_bswap32(ofsId);
        ofsBin = __builtin_bswap32(ofsBin);
        ofsTpl = __builtin_bswap32(ofsTpl);
    }
#else
    u32 ofsId = pack->ofsId;
    u32 ofsBin = pack->ofsBin;
    u32 ofsTpl = pack->ofsTpl;
#endif

    ItemModelIdTbl* idTbl = (ItemModelIdTbl*) (data_addr + ofsId);
    ItemModelOfsTbl* binTbl = (ItemModelOfsTbl*) (data_addr + ofsBin);
    ItemModelOfsTbl* tplTbl = (ItemModelOfsTbl*) (data_addr + ofsTpl);
    u32 i;

#if defined(OPENRE4_NATIVE)
    u32 num = idTbl->num;
    if (num > 0x1000) {
        num = __builtin_bswap32(num);
    }
#else
    u32 num = idTbl->num;
#endif

    for (i = 0; i < num; i++) {
#if defined(OPENRE4_NATIVE)
        u16 id = idTbl->id[i].id;
        if (id > 0x100) {
            id = __builtin_bswap16(id);
        }
        u32 bOfs = binTbl->ofs[i];
        if (bOfs > 0x1000000) {
            bOfs = __builtin_bswap32(bOfs);
        }
        u32 tOfs = tplTbl->ofs[i];
        if (tOfs > 0x1000000) {
            tOfs = __builtin_bswap32(tOfs);
        }
        g_pItemModelSys->ItmRegist((u8*) binTbl + bOfs, (u8*) tplTbl + tOfs, (u8) id);
#else
        u16 id = idTbl->id[i].id;
        g_pItemModelSys->ItmRegist((u8*) binTbl + binTbl->ofs[i], (u8*) tplTbl + tplTbl->ofs[i], id);
#endif
    }
    return 1;
}

// Stores the model pair for item `no`.
int cItmSys::ItmRegist(void* bin, void* tpl, u8 no)
{
    work[no].bin = bin;
    work[no].tpl = tpl;
    return 1;
}

// Registered binary of item `no` (1 when present).
int cItmSys::GetBinAddr(u8 id, void** pBin_addr)
{
    *pBin_addr = work[id].bin;
    if (*pBin_addr != 0) {
        return 1;
    }
    return 0;
}

// Registered tpl of item `no` (1 when present).
int cItmSys::GetTplAddr(u8 id, void** pTpl_addr)
{
    *pTpl_addr = work[id].tpl;
    if (*pTpl_addr != 0) {
        return 1;
    }
    return 0;
}
