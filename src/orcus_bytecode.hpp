#pragma once

#include "orcus.hpp"
#include <stdint.h>


typedef void (*orcus_instruction) (orcus_state* const ORCUS_CPP_restrict M); //takes a state


void ORCUS_BYTECODE_NOP(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_ADD(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_SUB(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_MUL(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_DIV(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_POW(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_IDIV(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_MOD(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_BAND(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_BOR(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_BXOR(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_BNOT(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_BSHL(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_BSHR(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_LAND(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_LOR(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_LXOR(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_LNOT(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_EQL(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_NEQ(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_GRT(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_LET(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_GTE(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_LTE(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_SENV(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_GENV(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_SET(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_GET(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_SREG(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_GREG(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_NINT(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_NFLO(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_NTRU(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_NFAL(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_NNIL(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_NSTR(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_NFUN(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_NTAB(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_NARR(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_JMP(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_JMPR(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_JIF(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_JNIF(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_JRIF(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_JRNI(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_CALL(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_END(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_LEN(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_CNCT(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_CLS(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_GETL(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_SETL(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_GETG(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_SETG(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_CPYT(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_SWPT(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_CPYX(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_UNM(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_INC(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_DEC(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_SCON(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_GCON(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_SWXY(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_NNSE(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_SETL_KCS(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_GETL_KCS(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_SETG_KCS(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_GETG_KCS(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_SENV_KCS(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_GENV_KCS(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_SET_KCS(orcus_state* const ORCUS_CPP_restrict M);
void ORCUS_BYTECODE_GET_KCS(orcus_state* const ORCUS_CPP_restrict M);

void ORCUS_BYTECODE_MSET(orcus_state* const ORCUS_CPP_restrict M);

extern orcus_instruction orcus_bytecode_list[];


enum orcus_opcodes:orcus_opcode {
	ORCUS_OPCODE_NOP = 0,

	ORCUS_OPCODE_ADD = 1,
	ORCUS_OPCODE_SUB = 2,
	ORCUS_OPCODE_MUL = 3,
	ORCUS_OPCODE_DIV = 4,
	ORCUS_OPCODE_POW = 5,
	ORCUS_OPCODE_IDIV = 6,
	ORCUS_OPCODE_MOD = 7,

	ORCUS_OPCODE_BAND = 8,
	ORCUS_OPCODE_BOR = 9,
	ORCUS_OPCODE_BXOR = 10,
	ORCUS_OPCODE_BNOT = 11,
	ORCUS_OPCODE_BSHL = 12,
	ORCUS_OPCODE_BSHR = 13,

	ORCUS_OPCODE_LAND = 14,
	ORCUS_OPCODE_LOR = 15,
	ORCUS_OPCODE_LXOR = 16,
	ORCUS_OPCODE_LNOT = 17,

	ORCUS_OPCODE_EQL = 18,
	ORCUS_OPCODE_NEQ = 19,
	ORCUS_OPCODE_GRT = 20,
	ORCUS_OPCODE_LET = 21,
	ORCUS_OPCODE_GTE = 22,
	ORCUS_OPCODE_LTE = 23,

	ORCUS_OPCODE_SENV = 24,
	ORCUS_OPCODE_GENV = 25,
	ORCUS_OPCODE_SET = 26,
	ORCUS_OPCODE_GET = 27,
	ORCUS_OPCODE_SREG = 28,
	ORCUS_OPCODE_GREG = 29,

	ORCUS_OPCODE_NINT = 30,
	ORCUS_OPCODE_NFLO = 31,
	ORCUS_OPCODE_NTRU = 32,
	ORCUS_OPCODE_NFAL = 33,
	ORCUS_OPCODE_NNIL = 34,
	ORCUS_OPCODE_NSTR = 35,
	ORCUS_OPCODE_NFUN = 36,
	ORCUS_OPCODE_NTAB = 37,
	ORCUS_OPCODE_NARR = 38,

	ORCUS_OPCODE_JMP = 39,
	ORCUS_OPCODE_JMPR = 40,
	ORCUS_OPCODE_JIF = 41,
	ORCUS_OPCODE_JNIF = 42,
	ORCUS_OPCODE_JRIF = 43,
	ORCUS_OPCODE_JRNI = 44,

	ORCUS_OPCODE_CALL = 45,
	ORCUS_OPCODE_EXIT = 46,
	ORCUS_OPCODE_LEN = 47,
	ORCUS_OPCODE_CNCT = 48,
	ORCUS_OPCODE_CLS = 49,
	ORCUS_OPCODE_GETL = 50,
	ORCUS_OPCODE_SETL = 51,
	ORCUS_OPCODE_GETG = 52,
	ORCUS_OPCODE_SETG = 53,

	ORCUS_OPCODE_CPYT = 54,
	ORCUS_OPCODE_SWPT = 55,
	ORCUS_OPCODE_CPYX = 56,

	ORCUS_OPCODE_UNM = 57,
	ORCUS_OPCODE_INC = 58,
	ORCUS_OPCODE_DEC = 59,

	ORCUS_OPCODE_SCON = 60,
	ORCUS_OPCODE_GCON = 61,

	ORCUS_OPCODE_SWXY = 62,

	ORCUS_OPCODE_NNSE = 63,

	ORCUS_OPCODE_SETL_KCS=64,
	ORCUS_OPCODE_GETL_KCS=65,
	ORCUS_OPCODE_SETG_KCS=66,
	ORCUS_OPCODE_GETG_KCS=67,
	ORCUS_OPCODE_SENV_KCS = 68,
	ORCUS_OPCODE_GENV_KCS = 69,
	ORCUS_OPCODE_SET_KCS = 70,
	ORCUS_OPCODE_GET_KCS = 71,

	ORCUS_OPCODE_MSET = 72,
};
