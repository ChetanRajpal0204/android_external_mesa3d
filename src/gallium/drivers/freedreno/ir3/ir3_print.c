/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#include <stdarg.h>
#include <stdio.h>

#include "ir3.h"

#define PTRID(x) ((unsigned long)(x))

static void print_instr_name(struct ir3_instruction *instr)
{
#ifdef DEBUG
	printf("%04u:", instr->serialno);
#endif
	printf("%03u: ", instr->depth);

	if (instr->flags & IR3_INSTR_SY)
		printf("(sy)");
	if (instr->flags & IR3_INSTR_SS)
		printf("(ss)");

	if (is_meta(instr)) {
		switch(instr->opc) {
		case OPC_META_PHI:
			printf("&#934;");
			break;
		default:
			/* shouldn't hit here.. just for debugging: */
			switch (instr->opc) {
			case OPC_META_INPUT:  printf("_meta:in");   break;
			case OPC_META_OUTPUT: printf("_meta:out");  break;
			case OPC_META_FO:     printf("_meta:fo");   break;
			case OPC_META_FI:     printf("_meta:fi");   break;
			case OPC_META_FLOW:   printf("_meta:flow"); break;

			default: printf("_meta:%d", instr->opc); break;
			}
			break;
		}
	} else if (instr->category == 1) {
		static const char *type[] = {
				[TYPE_F16] = "f16",
				[TYPE_F32] = "f32",
				[TYPE_U16] = "u16",
				[TYPE_U32] = "u32",
				[TYPE_S16] = "s16",
				[TYPE_S32] = "s32",
				[TYPE_U8]  = "u8",
				[TYPE_S8]  = "s8",
		};
		if (instr->cat1.src_type == instr->cat1.dst_type)
			printf("mov");
		else
			printf("cov");
		printf(".%s%s", type[instr->cat1.src_type], type[instr->cat1.dst_type]);
	} else {
		printf("%s", ir3_instr_name(instr));
		if (instr->flags & IR3_INSTR_3D)
			printf(".3d");
		if (instr->flags & IR3_INSTR_A)
			printf(".a");
		if (instr->flags & IR3_INSTR_O)
			printf(".o");
		if (instr->flags & IR3_INSTR_P)
			printf(".p");
		if (instr->flags & IR3_INSTR_S)
			printf(".s");
		if (instr->flags & IR3_INSTR_S2EN)
			printf(".s2en");
	}
}

static void print_reg_name(struct ir3_register *reg, bool followssa)
{
	if ((reg->flags & (IR3_REG_FABS | IR3_REG_SABS)) &&
			(reg->flags & (IR3_REG_FNEG | IR3_REG_SNEG | IR3_REG_BNOT)))
		printf("(absneg)");
	else if (reg->flags & (IR3_REG_FNEG | IR3_REG_SNEG | IR3_REG_BNOT))
		printf("(neg)");
	else if (reg->flags & (IR3_REG_FABS | IR3_REG_SABS))
		printf("(abs)");

	if (reg->flags & IR3_REG_IMMED) {
		printf("imm[%f,%d,0x%x]", reg->fim_val, reg->iim_val, reg->iim_val);
	} else if (reg->flags & IR3_REG_SSA) {
		printf("_");
		if (followssa) {
			printf("[");
			print_instr_name(reg->instr);
			printf("]");
		}
	} else if (reg->flags & IR3_REG_RELATIV) {
		if (reg->flags & IR3_REG_HALF)
			printf("h");
		if (reg->flags & IR3_REG_CONST)
			printf("c<a0.x + %u>", reg->num);
		else
			printf("\x1b[0;31mr<a0.x + %u>\x1b[0m (%u)", reg->num, reg->size);
	} else {
		if (reg->flags & IR3_REG_HALF)
			printf("h");
		if (reg->flags & IR3_REG_CONST)
			printf("c%u.%c", reg_num(reg), "xyzw"[reg_comp(reg)]);
		else
			printf("\x1b[0;31mr%u.%c\x1b[0m", reg_num(reg), "xyzw"[reg_comp(reg)]);
	}
}

static void
tab(int lvl)
{
	for (int i = 0; i < lvl; i++)
		printf("\t");
}

static void
print_instr(struct ir3_instruction *instr, int lvl)
{
	unsigned i;

	tab(lvl);

	print_instr_name(instr);
	for (i = 0; i < instr->regs_count; i++) {
		struct ir3_register *reg = instr->regs[i];
		printf(i ? ", " : " ");
		print_reg_name(reg, !!i);
	}

	if (instr->address) {
		printf(", address=_");
		printf("[");
		print_instr_name(instr->address);
		printf("]");
	}

	if (instr->fanin) {
		printf(", fanin=_");
		printf("[");
		print_instr_name(instr->fanin);
		printf("]");
	}

	if (is_meta(instr)) {
		if (instr->opc == OPC_META_FO) {
			printf(", off=%d", instr->fo.off);
		} else if ((instr->opc == OPC_META_FI) && instr->fi.aid) {
			printf(", aid=%d", instr->fi.aid);
		}
	}

	printf("\n");
}

void ir3_print_instr(struct ir3_instruction *instr)
{
	print_instr(instr, 0);
}

static void
print_block(struct ir3_block *block, int lvl)
{
	struct ir3_instruction *instr;
	tab(lvl); printf("block {\n");
	for (instr = block->head; instr; instr = instr->next) {
		print_instr(instr, lvl+1);
	}
	tab(lvl); printf("}\n");
}

void
ir3_print(struct ir3 *ir)
{
	struct ir3_block *block = ir->block;

	print_block(block, 0);

	for (unsigned i = 0; i < block->noutputs; i++) {
		if (!block->outputs[i])
			continue;
		printf("out%d: ", i);
		print_instr(block->outputs[i], 0);
	}
}