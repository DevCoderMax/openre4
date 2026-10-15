#ifndef FLAG_H
#define FLAG_H

#include "types.h"

// Bit flags packed into u32 words, numbered from the top bit: flag `no` is bit 31 - (no & 31) of word no >> 5.

// The per-field tests go through a real inline rather than the FlagChk macro: the flag number
// arrives as a parameter, which changes how sched1 orders the constants that follow the test
// (objWep drawPoint and esp_app EspDrawLaserLine only match this way).
static inline u32 FlagBitChk(u32* flg, u32 no) { return flg[no >> 5] & (0x80000000 >> (no & 31)); }

// Set, clear and toggle, against the same base and index as FlagBitChk.
// TODO: PS2's flag.h had these as inline functions taking the word's address, like FlagBitChk.
#if defined(TARGET_PC) || defined(__LP64__) || defined(_WIN64)
#define FlagOn(base, no) (((u32*)(base))[((u32)(no)) >> 5] |= (0x80000000U >> (((u32)(no)) & 31)))
#define FlagOff(base, no) (((u32*)(base))[((u32)(no)) >> 5] &= ~(0x80000000U >> (((u32)(no)) & 31)))
#define FlagXor(base, no) (((u32*)(base))[((u32)(no)) >> 5] ^= (0x80000000U >> (((u32)(no)) & 31)))

#define FLAG_WORD_VAR(base, no, op) ({ u32* flagBase_ = (u32*) (base); __typeof__(no) flagNo_ = (no); \
                                       flagBase_[flagNo_ >> 5] op; })
#else
#define FlagOn(base, no) (*(u32*) ((((no) >> 5) << 2) + (u32) (base)) |= (0x80000000 >> ((no) & 31)))
#define FlagOff(base, no) (*(u32*) ((((no) >> 5) << 2) + (u32) (base)) &= ~(0x80000000 >> ((no) & 31)))
#define FlagXor(base, no) (*(u32*) ((((no) >> 5) << 2) + (u32) (base)) ^= (0x80000000 >> ((no) & 31)))

#define FLAG_WORD_VAR(base, no, op) ({ u32 flagBase_ = (u32) (base); __typeof__(no) flagNo_ = (no); \
                                       *(u32*) (((flagNo_ >> 5) << 2) + flagBase_) op; })
#endif
#define FlagChkVar(base, no) FLAG_WORD_VAR(base, no, & (0x80000000 >> (flagNo_ & 31)))
#define FlagOnVar(base, no) FLAG_WORD_VAR(base, no, |= (0x80000000 >> (flagNo_ & 31)))
#define FlagOffVar(base, no) FLAG_WORD_VAR(base, no, &= ~(0x80000000 >> (flagNo_ & 31)))
#define FlagXorVar(base, no) FLAG_WORD_VAR(base, no, ^= (0x80000000 >> (flagNo_ & 31)))

#endif
