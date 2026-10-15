/* Capcom's yz2 decompressor (game/yz2asm): the LZ-style unpacker behind the yz2 archives. Assembly in
 * the original (the debug build has the unit as Capcom asm, the byte patterns show no compiler idiom);
 * the functions are whole-function asm() bodies, reproduced from the split object. */
#include "asm_regs.h"

#if defined(__powerpc__) || defined(__ppc__)

asm("	.text\n"
    "	.balign 4\n");

// yz2Decode_Decode(ctx, dst, size, in_ev): the decode loop. Low symbols of the main model are literal
// bytes, higher ones copy a run from the 256-entry dictionary, and each run is added to it.
// This is a register-level reading of the original asm, and the exact symbol split is not verified.
asm("	.globl yz2Decode_Decode\n"
    "	.type yz2Decode_Decode,@function\n"
    "yz2Decode_Decode:\n"
    "	stwu r1, -0x80(r1)\n"
    "	mflr r0\n"
    "	stmw r2, 0x8(r1)\n"
    "	stw r0, 0x84(r1)\n"
    "	lwz r30, 0x0(r3)\n"
    "	mr r20, r4\n"
    "	add r22, r4, r5\n"
    "	lwz r23, 0x0(r6)\n"
    "	mr r21, r20\n"
    "	lis r31, 0x80\n"
    "	addi r29, r3, 0x30\n"
    "	addi r27, r3, 0xc\n"
    "	lis r4, .float_one_data@ha\n"
    "	lfs f5, .float_one_data@l(r4)\n"
    "	lis r4, .float_zero_data@ha\n"
    "	lfs f4, .float_zero_data@l(r4)\n"
    "	fmr f2, f4\n"
    "	fmr f3, f4\n"
    "	lwz r17, 0x0(r29)\n"
    "	lwz r18, 0x14(r29)\n"
    "	lwz r19, 0x4(r29)\n"
    "	lwz r16, 0x8(r29)\n"
    "	lwz r15, 0xc(r29)\n"
    "	lwz r6, 0x0(r27)\n"
    "	lwz r12, 0x14(r27)\n"
    "	lwz r14, 0x4(r27)\n"
    "	lwz r5, 0x8(r27)\n"
    "	lwz r3, 0xc(r27)\n"
    "	.size yz2Decode_Decode,.-yz2Decode_Decode\n");

// Loop head: next symbol.
asm("	.globl yz2Decode_loop\n"
    "	.type yz2Decode_loop,@function\n"
    "yz2Decode_loop:\n"
    "	cmplw r20, r22\n"
    "	bge yz2Decode_end\n"
    "	bl FrequencyDecode_Decode768\n"
    "	cmplwi r24, 0x3ff\n"
    "	ble yz2Decode_L01\n"
    "	stb r24, 0x0(r20)\n"
    "	li r24, 0x1\n"
    "	addi r20, r20, 0x1\n"
    "	b yz2Decode_dic_set\n"
    "	.size yz2Decode_loop,.-yz2Decode_loop\n");

// Probably the literal / short-reference path (yz2Decode_L01).
asm("	.globl yz2Decode_L01\n"
    "	.type yz2Decode_L01,@function\n"
    "yz2Decode_L01:\n"
    "	lbz r9, 0x0(r21)\n"
    "	cmplwi r24, 0x1ff\n"
    "	slwi r0, r9, 10\n"
    "	add r0, r0, r9\n"
    "	slwi r0, r0, 2\n"
    "	add r26, r30, r0\n"
    "	lhz r9, 0x2(r26)\n"
    "	add r9, r24, r9\n"
    "	clrlwi r25, r9, 23\n"
    "	bgt yz2Decode_L02\n"
    "	bl FrequencyDecode_Decode\n"
    "	cmplwi r24, 0x2\n"
    "	bgt .L_801D3F28\n"
    "	cmpwi r24, 0x2\n"
    "	bne .L_801D3EE0\n"
    "	bl FrequencyDecode_Decode\n"
    "	slwi r28, r24, 8\n"
    "	b .L_801D3F20\n"
    ".L_801D3EE0:\n"
    "	cmpwi r24, 0x1\n"
    "	bne .L_801D3F00\n"
    "	bl FrequencyDecode_Decode\n"
    "	slwi r28, r24, 16\n"
    "	bl FrequencyDecode_Decode\n"
    "	slwi r0, r24, 8\n"
    "	or r28, r28, r0\n"
    "	b .L_801D3F20\n"
    ".L_801D3F00:\n"
    "	bl FrequencyDecode_Decode\n"
    "	slwi r28, r24, 24\n"
    "	bl FrequencyDecode_Decode\n"
    "	slwi r0, r24, 16\n"
    "	or r28, r28, r0\n"
    "	bl FrequencyDecode_Decode\n"
    "	slwi r0, r24, 8\n"
    "	or r28, r28, r0\n"
    ".L_801D3F20:\n"
    "	bl FrequencyDecode_Decode\n"
    "	or r24, r28, r24\n"
    ".L_801D3F28:\n"
    "	slwi r0, r25, 2\n"
    "	addi r9, r26, 0x4\n"
    "	lwzx r9, r9, r0\n"
    "	subi r24, r24, 0x1\n"
    "	b yz2Decode_L03\n"
    "	.size yz2Decode_L01,.-yz2Decode_L01\n");

// Probably the long-reference path: more fields read from the 0x100-symbol model.
asm("	.globl yz2Decode_L02\n"
    "	.type yz2Decode_L02,@function\n"
    "yz2Decode_L02:\n"
    "	slwi r10, r25, 2\n"
    "	addi r9, r26, 0x804\n"
    "	addi r11, r26, 0x4\n"
    "	lwzx r24, r9, r10\n"
    "	lwzx r9, r11, r10\n"
    "	.size yz2Decode_L02,.-yz2Decode_L02\n");

asm("	.type yz2Decode_L03,@function\n"
    "yz2Decode_L03:\n"
    "	li r8, 0x0\n"
    "	mtctr r24\n"
    "	.size yz2Decode_L03,.-yz2Decode_L03\n");

// Copies a referenced run into the output.
asm("	.globl yz2Decode_str_trans_loop\n"
    "	.type yz2Decode_str_trans_loop,@function\n"
    "yz2Decode_str_trans_loop:\n"
    "	lbzx r0, r9, r8\n"
    "	stbx r0, r20, r8\n"
    "	addi r8, r8, 0x1\n"
    "	bdnz yz2Decode_str_trans_loop\n"
    "	add r20, r20, r24\n"
    "	.size yz2Decode_str_trans_loop,.-yz2Decode_str_trans_loop\n");

// Enters the run just written into the dictionary (256 entries of two 0x800-byte halves).
asm("	.globl yz2Decode_dic_set\n"
    "	.type yz2Decode_dic_set,@function\n"
    "yz2Decode_dic_set:\n"
    "	subi r9, r20, 0x1\n"
    "	cmplw r21, r9\n"
    "	bge yz2Decode_loop\n"
    "	lbz r0, 0x0(r21)\n"
    "	addi r4, r21, 0x1\n"
    "	mr r21, r9\n"
    "	slwi r9, r0, 10\n"
    "	add r9, r9, r0\n"
    "	slwi r9, r9, 2\n"
    "	lwzx r10, r30, r9\n"
    "	add r11, r30, r9\n"
    "	addi r8, r11, 0x4\n"
    "	slwi r0, r10, 2\n"
    "	addi r11, r11, 0x804\n"
    "	stwx r4, r8, r0\n"
    "	addi r10, r10, 0x1\n"
    "	stwx r24, r11, r0\n"
    "	clrlwi r10, r10, 23\n"
    "	stwx r10, r30, r9\n"
    "	b yz2Decode_loop\n"
    "	.size yz2Decode_dic_set,.-yz2Decode_dic_set\n");

// Epilogue.
asm("	.globl yz2Decode_end\n"
    "	.type yz2Decode_end,@function\n"
    "yz2Decode_end:\n"
    "	lwz r0, 0x84(r1)\n"
    "	mtlr r0\n"
    "	lmw r2, 0x8(r1)\n"
    "	addi r1, r1, 0x80\n"
    "	blr\n"
    "	.size yz2Decode_end,.-yz2Decode_end\n");

// FrequencyDecode_Decode: one symbol of the 0x100-symbol model with the adaptive arithmetic coder
// (bit reader state in Yz2Dec, frequency / cumulative tables in Yz2Freq); the symbol's frequency
// is bumped and the table rescaled when it fills.
asm("	.globl FrequencyDecode_Decode\n"
    "	.type FrequencyDecode_Decode,@function\n"
    "FrequencyDecode_Decode:\n"
    "	lwz r4, 0x0(r17)\n"
    "	lwz r8, 0x4(r17)\n"
    "	cmplw r4, r31\n"
    "	bgt .L_801D4058\n"
    "	cmplwi r4, 0x8000\n"
    "	ble .L_801D4000\n"
    "	slwi r10, r8, 8\n"
    "	slwi r4, r4, 8\n"
    "	lbz r0, 0x0(r23)\n"
    "	addi r23, r23, 0x1\n"
    "	or r8, r10, r0\n"
    "	b .L_801D4058\n"
    ".L_801D4000:\n"
    "	cmplwi r4, 0x80\n"
    "	ble .L_801D402C\n"
    "	slwi r8, r8, 16\n"
    "	slwi r4, r4, 16\n"
    "	lbz r0, 0x0(r23)\n"
    "	slwi r0, r0, 8\n"
    "	lbz r7, 0x1(r23)\n"
    "	addi r23, r23, 0x2\n"
    "	or r8, r8, r0\n"
    "	or r8, r8, r7\n"
    "	b .L_801D4058\n"
    ".L_801D402C:\n"
    "	slwi r8, r8, 24\n"
    "	slwi r4, r4, 24\n"
    "	lbz r9, 0x0(r23)\n"
    "	slwi r9, r9, 16\n"
    "	lbz r0, 0x1(r23)\n"
    "	or r8, r8, r9\n"
    "	slwi r0, r0, 8\n"
    "	lbz r9, 0x2(r23)\n"
    "	addi r23, r23, 0x3\n"
    "	or r8, r8, r0\n"
    "	or r8, r8, r9\n"
    ".L_801D4058:\n"
    "	srwi r4, r4, 14\n"
    "	stw r8, 0x4(r17)\n"
    "	stw r4, 0x0(r17)\n"
    "	divwu r24, r8, r4\n"
    "	lwz r4, 0x20(r29)\n"
    "	fcmpu cr0, f2, f4\n"
    "	bne .L_801D40D0\n"
    "	lwz r8, 0x18(r29)\n"
    "	li r7, 0x0\n"
    "	cmpw r7, r8\n"
    "	bge .L_801D40CC\n"
    "	li r0, 0x0\n"
    ".L_801D4088:\n"
    "	slwi r9, r7, 2\n"
    "	add r11, r18, r9\n"
    "	lhzx r10, r18, r9\n"
    "	mr r9, r0\n"
    "	lhz r0, 0x2(r11)\n"
    "	add r0, r0, r10\n"
    "	cmpw r9, r0\n"
    "	bge .L_801D40C0\n"
    "	slwi r11, r9, 2\n"
    "	subf r9, r9, r0\n"
    "	mtctr r9\n"
    ".L_801D40B4:\n"
    "	stwx r7, r11, r4\n"
    "	addi r11, r11, 0x4\n"
    "	bdnz .L_801D40B4\n"
    ".L_801D40C0:\n"
    "	addi r7, r7, 0x1\n"
    "	cmpw r7, r8\n"
    "	blt .L_801D4088\n"
    ".L_801D40CC:\n"
    "	fmr f2, f5\n"
    ".L_801D40D0:\n"
    "	slwi r24, r24, 2\n"
    "	lwzx r24, r24, r4\n"
    "	slwi r7, r24, 2\n"
    "	add r8, r24, r24\n"
    "	add r10, r7, r18\n"
    "	lwz r11, 0x0(r17)\n"
    "	lhz r0, 0x2(r10)\n"
    "	lwz r9, 0x4(r17)\n"
    "	mullw r0, r11, r0\n"
    "	subf r9, r0, r9\n"
    "	stw r9, 0x4(r17)\n"
    "	lhzx r0, r7, r18\n"
    "	mullw r11, r11, r0\n"
    "	srwi r11, r11, 1\n"
    "	stw r11, 0x0(r17)\n"
    "	lhzx r9, r8, r19\n"
    "	addi r9, r9, 0x1\n"
    "	sthx r9, r8, r19\n"
    "	addi r16, r16, 0x1\n"
    "	cmpwi r15, 0xe\n"
    "	bgt .L_801D4190\n"
    "	lwz r0, 0x10(r29)\n"
    "	cmpw r16, r0\n"
    "	bne .L_801D41EC\n"
    "	subfic r0, r15, 0xf\n"
    "	lwz r4, 0x18(r29)\n"
    "	li r10, 0x0\n"
    "	fmr f2, f4\n"
    "	li r9, 0x1\n"
    "	slw r11, r9, r0\n"
    "	cmpwi r4, 0x0\n"
    "	ble .L_801D417C\n"
    "	mr r8, r18\n"
    "	mr r7, r19\n"
    "	mtctr r4\n"
    ".L_801D415C:\n"
    "	lhz r0, 0x0(r7)\n"
    "	addi r7, r7, 0x2\n"
    "	mullw r0, r11, r0\n"
    "	sth r10, 0x2(r8)\n"
    "	add r10, r10, r0\n"
    "	sth r0, 0x0(r8)\n"
    "	addi r8, r8, 0x4\n"
    "	bdnz .L_801D415C\n"
    ".L_801D417C:\n"
    "	li r0, 0x1\n"
    "	addi r15, r15, 0x1\n"
    "	slw r0, r0, r15\n"
    "	stw r0, 0x10(r29)\n"
    "	b .L_801D41EC\n"
    ".L_801D4190:\n"
    "	cmplwi r16, 0x7fff\n"
    "	ble .L_801D41EC\n"
    "	lwz r4, 0x18(r29)\n"
    "	fmr f2, f4\n"
    "	li r16, 0x0\n"
    "	li r8, 0x0\n"
    "	cmpw r16, r4\n"
    "	bge .L_801D41EC\n"
    "	mr r7, r18\n"
    "	mr r10, r19\n"
    "	mtctr r4\n"
    ".L_801D41BC:\n"
    "	lhz r0, 0x0(r10)\n"
    "	sth r0, 0x0(r7)\n"
    "	cmplwi r0, 0x1\n"
    "	sth r8, 0x2(r7)\n"
    "	add r8, r8, r0\n"
    "	ble .L_801D41DC\n"
    "	srwi r0, r0, 1\n"
    "	sth r0, 0x0(r10)\n"
    ".L_801D41DC:\n"
    "	addi r10, r10, 0x2\n"
    "	add r16, r16, r0\n"
    "	addi r7, r7, 0x4\n"
    "	bdnz .L_801D41BC\n"
    ".L_801D41EC:\n"
    "	blr\n"
    "	.size FrequencyDecode_Decode,.-FrequencyDecode_Decode\n");

// Same decoder for the 0x500-symbol main model (called first in every loop step).
asm("	.globl FrequencyDecode_Decode768\n"
    "	.type FrequencyDecode_Decode768,@function\n"
    "FrequencyDecode_Decode768:\n"
    "	lwz r4, 0x0(r6)\n"
    "	lwz r8, 0x4(r6)\n"
    "	cmplw r4, r31\n"
    "	bgt .L_801D4278\n"
    "	cmplwi r4, 0x8000\n"
    "	ble .L_801D4220\n"
    "	slwi r10, r8, 8\n"
    "	slwi r4, r4, 8\n"
    "	lbz r0, 0x0(r23)\n"
    "	addi r23, r23, 0x1\n"
    "	or r8, r10, r0\n"
    "	b .L_801D4278\n"
    ".L_801D4220:\n"
    "	cmplwi r4, 0x80\n"
    "	ble .L_801D424C\n"
    "	slwi r8, r8, 16\n"
    "	slwi r4, r4, 16\n"
    "	lbz r0, 0x0(r23)\n"
    "	slwi r0, r0, 8\n"
    "	lbz r7, 0x1(r23)\n"
    "	addi r23, r23, 0x2\n"
    "	or r8, r8, r0\n"
    "	or r8, r8, r7\n"
    "	b .L_801D4278\n"
    ".L_801D424C:\n"
    "	slwi r8, r8, 24\n"
    "	slwi r4, r4, 24\n"
    "	lbz r9, 0x0(r23)\n"
    "	slwi r9, r9, 16\n"
    "	lbz r0, 0x1(r23)\n"
    "	or r8, r8, r9\n"
    "	slwi r0, r0, 8\n"
    "	lbz r9, 0x2(r23)\n"
    "	addi r23, r23, 0x3\n"
    "	or r8, r8, r0\n"
    "	or r8, r8, r9\n"
    ".L_801D4278:\n"
    "	srwi r4, r4, 14\n"
    "	stw r8, 0x4(r6)\n"
    "	stw r4, 0x0(r6)\n"
    "	divwu r24, r8, r4\n"
    "	lwz r4, 0x20(r27)\n"
    "	fcmpu cr0, f3, f4\n"
    "	bne .L_801D42F0\n"
    "	lwz r8, 0x18(r27)\n"
    "	li r7, 0x0\n"
    "	cmpw r7, r8\n"
    "	bge .L_801D42EC\n"
    "	li r0, 0x0\n"
    ".L_801D42A8:\n"
    "	slwi r9, r7, 2\n"
    "	add r11, r12, r9\n"
    "	lhzx r10, r12, r9\n"
    "	mr r9, r0\n"
    "	lhz r0, 0x2(r11)\n"
    "	add r0, r0, r10\n"
    "	cmpw r9, r0\n"
    "	bge .L_801D42E0\n"
    "	slwi r11, r9, 2\n"
    "	subf r9, r9, r0\n"
    "	mtctr r9\n"
    ".L_801D42D4:\n"
    "	stwx r7, r11, r4\n"
    "	addi r11, r11, 0x4\n"
    "	bdnz .L_801D42D4\n"
    ".L_801D42E0:\n"
    "	addi r7, r7, 0x1\n"
    "	cmpw r7, r8\n"
    "	blt .L_801D42A8\n"
    ".L_801D42EC:\n"
    "	fmr f3, f5\n"
    ".L_801D42F0:\n"
    "	slwi r24, r24, 2\n"
    "	lwzx r24, r24, r4\n"
    "	slwi r7, r24, 2\n"
    "	add r8, r24, r24\n"
    "	add r10, r7, r12\n"
    "	lwz r11, 0x0(r6)\n"
    "	lhz r0, 0x2(r10)\n"
    "	lwz r9, 0x4(r6)\n"
    "	mullw r0, r11, r0\n"
    "	subf r9, r0, r9\n"
    "	stw r9, 0x4(r6)\n"
    "	lhzx r0, r7, r12\n"
    "	mullw r11, r11, r0\n"
    "	srwi r11, r11, 1\n"
    "	stw r11, 0x0(r6)\n"
    "	lhzx r9, r8, r14\n"
    "	addi r9, r9, 0x1\n"
    "	sthx r9, r8, r14\n"
    "	addi r5, r5, 0x1\n"
    "	cmpwi r3, 0xe\n"
    "	bgt .L_801D43B0\n"
    "	lwz r0, 0x10(r27)\n"
    "	cmpw r5, r0\n"
    "	bne .L_801D440C\n"
    "	subfic r0, r3, 0xf\n"
    "	lwz r4, 0x18(r27)\n"
    "	li r10, 0x0\n"
    "	fmr f3, f4\n"
    "	li r9, 0x1\n"
    "	slw r11, r9, r0\n"
    "	cmpwi r4, 0x0\n"
    "	ble .L_801D439C\n"
    "	mr r8, r12\n"
    "	mr r7, r14\n"
    "	mtctr r4\n"
    ".L_801D437C:\n"
    "	lhz r0, 0x0(r7)\n"
    "	addi r7, r7, 0x2\n"
    "	mullw r0, r11, r0\n"
    "	sth r10, 0x2(r8)\n"
    "	add r10, r10, r0\n"
    "	sth r0, 0x0(r8)\n"
    "	addi r8, r8, 0x4\n"
    "	bdnz .L_801D437C\n"
    ".L_801D439C:\n"
    "	li r0, 0x1\n"
    "	addi r3, r3, 0x1\n"
    "	slw r0, r0, r3\n"
    "	stw r0, 0x10(r27)\n"
    "	b .L_801D440C\n"
    ".L_801D43B0:\n"
    "	cmplwi r5, 0x7fff\n"
    "	ble .L_801D440C\n"
    "	lwz r4, 0x18(r27)\n"
    "	fmr f3, f4\n"
    "	li r5, 0x0\n"
    "	li r8, 0x0\n"
    "	cmpw r5, r4\n"
    "	bge .L_801D440C\n"
    "	mr r7, r12\n"
    "	mr r10, r14\n"
    "	mtctr r4\n"
    ".L_801D43DC:\n"
    "	lhz r0, 0x0(r10)\n"
    "	sth r0, 0x0(r7)\n"
    "	cmplwi r0, 0x1\n"
    "	sth r8, 0x2(r7)\n"
    "	add r8, r8, r0\n"
    "	ble .L_801D43FC\n"
    "	srwi r0, r0, 1\n"
    "	sth r0, 0x0(r10)\n"
    ".L_801D43FC:\n"
    "	addi r10, r10, 0x2\n"
    "	add r5, r5, r0\n"
    "	addi r7, r7, 0x4\n"
    "	bdnz .L_801D43DC\n"
    ".L_801D440C:\n"
    "	blr\n"
    "	.size FrequencyDecode_Decode768,.-FrequencyDecode_Decode768\n");

asm("	.rodata\n"
    "	.balign 8\n");

asm("	.type .float_one_data,@object\n"
    ".float_one_data:\n"
    "	.float 1\n"
    "	.size .float_one_data,.-.float_one_data\n");

asm("	.globl .float_zero_data\n"
    "	.type .float_zero_data,@object\n"
    ".float_zero_data:\n"
    "	.float 0\n"
    "	.size .float_zero_data,.-.float_zero_data\n");

#else
#include "types.h"
#include "yz2code.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct Yz2ModelNative {
    int n;
    uint16_t freq[1280];
    uint16_t cum_f[1280];
    uint16_t cum_s[1280];
    int total;
    int bits;
    int range;

    void init(int count) {
        n = count;
        int base = 0x8000 / n;
        int rem = 0x8000 % n;
        uint16_t s = 0;
        for (int i = 0; i < n; i++) {
            uint16_t f = base + (i < rem ? 1 : 0);
            cum_f[i] = f;
            cum_s[i] = s;
            s += f;
        }
        for (int i = 0; i < n; i++) {
            freq[i] = 1;
        }
        total = n;
        bits = 0;
        if (n >= 1) {
            while (true) {
                bits++;
                if (bits > 14) break;
                if (n < (1 << bits)) break;
            }
        }
        range = 1 << bits;
    }
};

static inline int decode_sym(Yz2ModelNative& m, uint32_t& R, uint32_t& C, const uint8_t*& src) {
    if (R <= 0x800000) {
        if (R > 0x8000) {
            C = ((C << 8) | *src++);
            R <<= 8;
        } else if (R > 0x80) {
            C = ((C << 16) | (src[0] << 8) | src[1]);
            R <<= 16;
            src += 2;
        } else {
            C = ((C << 24) | (src[0] << 16) | (src[1] << 8) | src[2]);
            R <<= 24;
            src += 3;
        }
    }
    uint32_t step = R >> 14;
    uint32_t value = C / step;
    if (value >= 0x8000) value = 0x7FFF;

    int low = 0, high = m.n - 1, s = 0;
    while (low <= high) {
        int mid = (low + high) / 2;
        if (m.cum_s[mid] <= value) {
            s = mid;
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    C -= step * m.cum_s[s];
    R = (step * m.cum_f[s]) >> 1;

    m.freq[s]++;
    m.total++;
    if (m.bits <= 14) {
        if (m.total == m.range) {
            int sh = 15 - m.bits;
            uint16_t run = 0;
            for (int i = 0; i < m.n; i++) {
                uint16_t v = m.freq[i] << sh;
                m.cum_s[i] = run;
                run += v;
                m.cum_f[i] = v;
            }
            m.bits++;
            m.range = 1 << m.bits;
        }
    } else if (m.total > 0x7FFF) {
        uint16_t run = 0;
        int total = 0;
        for (int i = 0; i < m.n; i++) {
            uint16_t v = m.freq[i];
            m.cum_f[i] = v;
            m.cum_s[i] = run;
            run += v;
            if (v > 1) v >>= 1;
            m.freq[i] = v;
            total += v;
        }
        m.total = total;
    }
    return s;
}

extern "C" void yz2Decode_Decode(void* ctx, void* dst, u32 size, void* ev) {
    Yz2InEv* event = (Yz2InEv*) ev;
    Yz2Ctx* c = (Yz2Ctx*) ctx;
    const uint8_t* src = event->src;
    uint8_t* out = (uint8_t*) dst;
    uint32_t unpacked = size;

    Yz2ModelNative main_model;
    main_model.init(0x500);
    Yz2ModelNative side_model;
    side_model.init(0x100);

    uint32_t R = 0x80;
    uint32_t C = c ? c->d.byte : *src++;

    static int dic_ptr[256 * 512];
    static int dic_len[256 * 512];
    static int dic_cnt[256];
    memset(dic_ptr, -1, sizeof(dic_ptr));
    memset(dic_len, 0, sizeof(dic_len));
    memset(dic_cnt, 0, sizeof(dic_cnt));

    uint32_t o = 0;
    uint32_t scan = 0;
    while (o < unpacked) {
        int s = decode_sym(main_model, R, C, src);
        int length = 0;
        if (s > 0x3FF) {
            out[o++] = (uint8_t)(s & 0xFF);
            length = 1;
        } else {
            uint8_t ctx_b = out[scan];
            int ebase = ctx_b * 512;
            int slot = (s + dic_cnt[ctx_b]) & 0x1FF;
            int src_off = dic_ptr[ebase + slot];
            if (s > 0x1FF) {
                length = dic_len[ebase + slot];
            } else {
                int t = decode_sym(side_model, R, C, src);
                if (t > 2) {
                    length = t;
                } else if (t == 2) {
                    int b0 = decode_sym(side_model, R, C, src);
                    int b1 = decode_sym(side_model, R, C, src);
                    length = (b0 << 8) | b1;
                } else if (t == 1) {
                    int b0 = decode_sym(side_model, R, C, src);
                    int b1 = decode_sym(side_model, R, C, src);
                    int b2 = decode_sym(side_model, R, C, src);
                    length = (b0 << 16) | (b1 << 8) | b2;
                } else {
                    int b0 = decode_sym(side_model, R, C, src);
                    int b1 = decode_sym(side_model, R, C, src);
                    int b2 = decode_sym(side_model, R, C, src);
                    int b3 = decode_sym(side_model, R, C, src);
                    length = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
                }
                length -= 1;
            }
            if (length <= 0 || src_off < 0 || o + length > unpacked) {
                break;
            }
            if (src_off + length <= (int)o) {
                memcpy(&out[o], &out[src_off], length);
            } else {
                for (int k = 0; k < length; k++) {
                    out[o + k] = out[src_off + k];
                }
            }
            o += length;
        }

        int last = (int)o - 1;
        if ((int)scan < last) {
            uint8_t ctx_b = out[scan];
            int cnt = dic_cnt[ctx_b];
            dic_ptr[ctx_b * 512 + cnt] = scan + 1;
            dic_len[ctx_b * 512 + cnt] = length;
            dic_cnt[ctx_b] = (cnt + 1) & 0x1FF;
            scan = last;
        }
    }
}
#endif
