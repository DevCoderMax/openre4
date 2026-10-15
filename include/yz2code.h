#ifndef YZ2CODE_H
#define YZ2CODE_H

#include "types.h"

// Input state shared with the assembly decoder (yz2asm).
struct Yz2InEv {
    u8* src;      // 0x00  compressed stream read pointer
    u8* heap;     // 0x04 / 0x08  work buffer start
    u8* free;     // 0x08 / 0x10  work buffer allocation pointer
    u32 size0;    // 0x0C / 0x18  first hex field of the header string
    u32 size1;    // 0x10 / 0x1C  second hex field of the header string
};

// Adaptive frequency table.
struct Yz2Freq {
    u16* freq;    // 0x00
    u32 max;      // 0x04
    int bits;     // 0x08
    u32 range;    // 0x0C
    u16* cum;     // 0x10
    int n;        // 0x14
    u32 acc;      // 0x18
};

struct Yz2Dec;

struct Yz2Model {
    Yz2Dec* dec;    // 0x00
    Yz2Freq fd;     // 0x04
    u8* table;      // 0x20
};

// Bit reader and the two models.
struct Yz2Dec {
    u32 mask;       // 0x00
    u32 byte;       // 0x04
    Yz2Model m1;    // 0x08
    Yz2Model m2;    // 0x2C
};

// Dictionary entry (0x1004 bytes, 0x100 of them).
struct Yz2DicEnt {
    u32 cnt;        // 0x00
    u8 a[0x800];    // 0x04
    u8 b[0x800];    // 0x804
};

struct Yz2Ctx {
    Yz2DicEnt* dic; // 0x00
    Yz2Dec d;       // 0x04 / 0x08
    u32 pad_54;     // 0x54
    int n;          // 0x58
};

extern "C" void yz2Decode_Decode(void* ctx, void* dst, u32 size, void* ev);
u32 Yz2DecodeSet(char* str, void* buf);
void Yz2DecodeExec(void* dst);

#endif
