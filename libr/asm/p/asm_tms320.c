/*
 * TMS320 disassembly engine
 *
 * Written by Ilya V. Matveychikov <i.matveychikov@milabs.ru>
 *
 * Distributed under LGPLv3
 */

#include <stdio.h>
#include <r_types.h>
#include <r_lib.h>
#include <r_asm.h>
#include <capstone/capstone.h>

static csh cd = 0;

#ifdef CAPSTONE_TMS320C64X_H
#define CAPSTONE_HAS_TMS320C64X 1
//#include "cs_mnemonics.c"
#else
#define CAPSTONE_HAS_TMS320C64X 0
#warning Cannot find capstone-tms320c64x support
#endif

#if CAPSTONE_HAS_TMS320C64X

static int tms320c64x_disassemble(RAsm *a, RAsmOp *op, const ut8 *buf, int len) {
	cs_insn* insn;
	int n = -1, ret = -1;
	int mode = 0;
	if (op) {
		memset (op, 0, sizeof (RAsmOp));
		op->size = 4;
	}
	if (cd != 0) {
		cs_close (&cd);
	}
	ret = cs_open (CS_ARCH_TMS320C64X, mode, &cd);
	if (ret) {
		goto fin;
	}
	cs_option (cd, CS_OPT_DETAIL, CS_OPT_OFF);
	if (!op) {
		return 0;
	}
	n = cs_disasm (cd, buf, len, a->pc, 1, &insn);
	if (n < 1) {
		strcpy (op->buf_asm, "invalid");
		op->size = 4;
		ret = -1;
		goto beach;
	} else {
		ret = 4;
	}
	if (insn->size < 1) {
		goto beach;
	}
	op->size = insn->size;
	snprintf (op->buf_asm, R_ASM_BUFSIZE, "%s%s%s",
		insn->mnemonic, insn->op_str[0]? " ": "",
		insn->op_str);
	r_str_replace_char (op->buf_asm, '%', 0);
	r_str_case (op->buf_asm, false);
	cs_free (insn, n);
	beach:
	// cs_close (&cd);
	fin:
	return ret;
}
#endif

#include "../arch/tms320/tms320_dasm.h"

static tms320_dasm_t engine = { 0 };

static int tms320_disassemble(RAsm *a, RAsmOp *op, const ut8 *buf, int len) {
	if (a->cpu && r_str_casecmp (a->cpu, "c54x") == 0) {
		tms320_f_set_cpu (&engine, TMS320_F_CPU_C54X);
	} else if (a->cpu && r_str_casecmp(a->cpu, "c55x+") == 0) {
		tms320_f_set_cpu (&engine, TMS320_F_CPU_C55X_PLUS);
	} else if (a->cpu && r_str_casecmp(a->cpu, "c55x") == 0) {
		tms320_f_set_cpu (&engine, TMS320_F_CPU_C55X);
	} else {
#if CAPSTONE_HAS_TMS320C64X
		if (a->cpu && !r_str_casecmp (a->cpu, "c64x")) {
			return tms320c64x_disassemble (a, op, buf, len);
		}
#endif
		snprintf (op->buf_asm, R_ASM_BUFSIZE - 1, "Unknown asm.cpu");
		return op->size = -1;
	}
	op->size = tms320_dasm (&engine, buf, len);
	strncpy (op->buf_asm, engine.syntax, R_ASM_BUFSIZE - 1);
	op->buf_asm[R_ASM_BUFSIZE - 1] = 0;
	return op->size;
}

static bool tms320_init(void * user) {
	return tms320_dasm_init (&engine);
}

static bool tms320_fini(void * user) {
	return tms320_dasm_fini (&engine);
}

RAsmPlugin r_asm_plugin_tms320 = {
	.name = "tms320",
	.arch = "tms320",
#if CAPSTONE_HAS_TMS320C64X
	.cpus = "c54x,c55x,c55x+,c64x",
	.desc = "TMS320 DSP family (c54x,c55x,c55x+,c64x)",
#else
	.cpus = "c54x,c55x,c55x+",
	.desc = "TMS320 DSP family (c54x,c55x,c55x+)",
#endif
	.license = "LGPLv3",
	.bits = 32,
	.endian = R_SYS_ENDIAN_LITTLE | R_SYS_ENDIAN_BIG,
	.init = tms320_init,
	.fini = tms320_fini,
	.disassemble = &tms320_disassemble,
};

#ifndef CORELIB
RLibStruct radare_plugin = {
	.type = R_LIB_TYPE_ASM,
	.data = &r_asm_plugin_tms320,
	.version = R2_VERSION
};
#endif
