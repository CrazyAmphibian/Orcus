#include "orcus_bytecode.hpp"
#include "orcus.hpp"
#include "orcus_error.hpp"
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>


void ORCUS_BYTECODE_NOP(orcus_state* const ORCUS_CPP_restrict M) {
	return;
}


void ORCUS_BYTECODE_ADD(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack -1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type == ORCUS_TYPE_FLOAT) {
		if (st->type == ORCUS_TYPE_INT) {
			st->type = ORCUS_TYPE_FLOAT;
			st->data.f = (orcus_float)st->data.i;
		}
	}else if (ss->type == ORCUS_TYPE_INT) {
		if (st->type == ORCUS_TYPE_FLOAT) {
			ss->type = ORCUS_TYPE_FLOAT;
			ss->data.f = (orcus_float)ss->data.i;
		}
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 1);
		return;
	}

	if (st->type == ORCUS_TYPE_FLOAT) {
		ss->data.f += st->data.f;
	}
	else if (st->type == ORCUS_TYPE_INT) {
		ss->data.i += st->data.i;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 2);
		return;
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_SUB(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type == ORCUS_TYPE_FLOAT) {
		if (st->type == ORCUS_TYPE_INT) {
			st->type = ORCUS_TYPE_FLOAT;
			st->data.f = (orcus_float)st->data.i;
		}
	}
	else if (ss->type == ORCUS_TYPE_INT) {
		if (st->type == ORCUS_TYPE_FLOAT) {
			ss->type = ORCUS_TYPE_FLOAT;
			ss->data.f = (orcus_float)ss->data.i;
		}
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 1);
		return;
	}

	if (st->type == ORCUS_TYPE_FLOAT) {
		ss->data.f -= st->data.f;
	}
	else if (st->type == ORCUS_TYPE_INT) {
		ss->data.i -= st->data.i;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 2);
		return;
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_MUL(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type == ORCUS_TYPE_FLOAT) {
		if (st->type == ORCUS_TYPE_INT) {
			st->type = ORCUS_TYPE_FLOAT;
			st->data.f = (orcus_float)st->data.i;
		}
	}
	else if (ss->type == ORCUS_TYPE_INT) {
		if (st->type == ORCUS_TYPE_FLOAT) {
			ss->type = ORCUS_TYPE_FLOAT;
			ss->data.f = (orcus_float)ss->data.i;
		}
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 1);
		return;
	}

	if (st->type == ORCUS_TYPE_FLOAT) {
		ss->data.f *= st->data.f;
	}
	else if (st->type == ORCUS_TYPE_INT) {
		ss->data.i *= st->data.i;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 2);
		return;
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_DIV(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type == ORCUS_TYPE_FLOAT) {
	}
	else if (ss->type == ORCUS_TYPE_INT) {
		ss->data.f = ss->data.i;
		ss->type = ORCUS_TYPE_FLOAT;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 1);
		return;
	}

	if (st->type == ORCUS_TYPE_FLOAT) {
		ss->data.f /= st->data.f;
	}
	else if (st->type == ORCUS_TYPE_INT) {
		ss->data.f /= (orcus_float)st->data.i;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 2);
		return;
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_POW(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type == ORCUS_TYPE_FLOAT) {
	}
	else if (ss->type == ORCUS_TYPE_INT) {
		ss->data.f = ss->data.i;
		ss->type = ORCUS_TYPE_FLOAT;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 1);
		return;
	}

	if (st->type == ORCUS_TYPE_FLOAT) {
		ss->data.f = pow(ss->data.f,st->data.f);
	}
	else if (st->type == ORCUS_TYPE_INT) {
		ss->data.f = pow(ss->data.f, (orcus_float)st->data.i);
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 2);
		return;
	}

	M->sizeofstack--;
	return;
}


void ORCUS_BYTECODE_IDIV(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type == ORCUS_TYPE_FLOAT) {
		ss->type = ORCUS_TYPE_INT;
		ss->data.i = (orcus_int)ss->data.f;
	}
	else if (ss->type == ORCUS_TYPE_INT) {
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 1);
		return;
	}

	if (st->type == ORCUS_TYPE_FLOAT) {
		st->data.i = (orcus_int)st->data.f;
		if (!st->data.i) {
			orcus_raise_error(M, ORCUS_ERROR_DIV_ZERO);
			return;
		}
		ss->data.i /= st->data.i;
	}
	else if (st->type == ORCUS_TYPE_INT) {
		if (!st->data.i) {
			orcus_raise_error(M, ORCUS_ERROR_DIV_ZERO);
			return;
		}
		ss->data.i /= st->data.i;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 2);
		return;
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_MOD(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type == ORCUS_TYPE_FLOAT) {
		if (st->type == ORCUS_TYPE_INT) {
			st->type = ORCUS_TYPE_FLOAT;
			st->data.f = (orcus_float)st->data.i;
		}
	}
	else if (ss->type == ORCUS_TYPE_INT) {
		if (st->type == ORCUS_TYPE_FLOAT) {
			ss->type = ORCUS_TYPE_FLOAT;
			ss->data.f = (orcus_float)ss->data.i;
		}
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 1);
		return;
	}

	if (st->type == ORCUS_TYPE_FLOAT) {
		ss->data.f = fmod(ss->data.f,st->data.f);
	}
	else if (st->type == ORCUS_TYPE_INT) {
		if (!st->data.i) {
			orcus_raise_error(M, ORCUS_ERROR_DIV_ZERO);
			return;
		}
		ss->data.i %= st->data.i;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 2);
		return;
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_BAND(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if(ss->type != ORCUS_TYPE_INT && ss->type != ORCUS_TYPE_FLOAT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 2);
		return;
	}

	if (st->type != ORCUS_TYPE_INT && st->type != ORCUS_TYPE_FLOAT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 1);
		return;
	}
	ss->data.i &= st->data.i;

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_BOR(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type != ORCUS_TYPE_INT && ss->type != ORCUS_TYPE_FLOAT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 2);
		return;
	}

	if (st->type != ORCUS_TYPE_INT && st->type != ORCUS_TYPE_FLOAT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 1);
		return;
	}
	ss->data.i |= st->data.i;

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_BXOR(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type != ORCUS_TYPE_INT && ss->type != ORCUS_TYPE_FLOAT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 2);
		return;
	}

	if (st->type != ORCUS_TYPE_INT && st->type != ORCUS_TYPE_FLOAT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 1);
		return;
	}
	ss->data.i ^= st->data.i;

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_BNOT(orcus_state* const ORCUS_CPP_restrict M) {
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* st = M->stack + M->sizeofstack - 1;
	if (st->type != ORCUS_TYPE_INT && st->type != ORCUS_TYPE_FLOAT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 1);
		return;
	}
#ifdef ORCUS_64BIT
	st->data.i ^= 0xffffffffffffffff;
#else
	st->data.i ^= 0xffffffff;
#endif
	return;
}

void ORCUS_BYTECODE_BSHL(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type != ORCUS_TYPE_INT && ss->type != ORCUS_TYPE_FLOAT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 2);
		return;
	}

	if (st->type != ORCUS_TYPE_INT && st->type != ORCUS_TYPE_FLOAT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 1);
		return;
	}

#ifdef ORCUS_64BIT
	ss->data.i <<= (st->data.i & 63);
#else
	ss->data.i <<= (st->data.i & 31);
#endif

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_BSHR(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type != ORCUS_TYPE_INT && ss->type != ORCUS_TYPE_FLOAT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 2);
		return;
	}

	if (st->type != ORCUS_TYPE_INT && st->type != ORCUS_TYPE_FLOAT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 1);
		return;
	}

#ifdef ORCUS_64BIT
	ss->data.i >>= (st->data.i & 63);
#else
	ss->data.i >>= (st->data.i & 31);
#endif

	M->sizeofstack--;
	return;
}



void ORCUS_BYTECODE_LAND(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	bool c1 = orcus_checkbool(st);
	bool c2 = orcus_checkbool(ss);

	if (c1 && c2) {
		orcus_release_var(ss);
		*ss = *st;
	}
	else {
		orcus_release_var(st);
		orcus_release_var(ss);
		ss->data.i = 0;
		ss->type = ORCUS_TYPE_BOOL;
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_LOR(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	bool c1 = orcus_checkbool(st);
	bool c2 = orcus_checkbool(ss);

	if (c2) {
		orcus_release_var(st);
	}
	else {
		if (c1) {
			orcus_release_var(ss);
			*ss = *st;
		}
		else {
			orcus_release_var(st);
			orcus_release_var(ss);
			ss->data.i = 0;
			ss->type = ORCUS_TYPE_BOOL;
		}
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_LXOR(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	bool c1=orcus_checkbool(st);
	bool c2=orcus_checkbool(ss);
	
	if ((c1 && c2) || (!c1 && !c2)) {
		orcus_release_var(st);
		orcus_release_var(ss);
		ss->data.i = 0;
		ss->type = ORCUS_TYPE_BOOL;
	}
	else if (c1) {
		orcus_release_var(ss);
		*ss = *st;
	}
	else {
		orcus_release_var(st);
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_LNOT(orcus_state* const ORCUS_CPP_restrict M) {
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	bool c= !orcus_checkbool(st);
	orcus_release_var(st);
	st->data.i = c;
	st->type = ORCUS_TYPE_BOOL;

	return;
}


void ORCUS_BYTECODE_EQL(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	bool c = orcus_vars_equal(ss, st);
	orcus_release_var(st);
	orcus_release_var(ss);
	ss->data.i = c;
	ss->type = ORCUS_TYPE_BOOL;

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_NEQ(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	bool c= !orcus_vars_equal(ss, st);
	orcus_release_var(st);
	orcus_release_var(ss);
	ss->data.i = c;
	ss->type = ORCUS_TYPE_BOOL;

	M->sizeofstack--;
	return;
}


void ORCUS_BYTECODE_GRT(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type == ORCUS_TYPE_FLOAT) {
		if (st->type == ORCUS_TYPE_INT) {
			st->type = ORCUS_TYPE_FLOAT;
			st->data.f = (orcus_float)st->data.i;
		}
	}
	else if (ss->type == ORCUS_TYPE_INT) {
		if (st->type == ORCUS_TYPE_FLOAT) {
			ss->data.f = (orcus_float)ss->data.i;
		}
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 1);
		return;
	}
	ss->type = ORCUS_TYPE_BOOL;

	if (st->type == ORCUS_TYPE_FLOAT) {
		ss->data.i = ss->data.f > st->data.f;
	}
	else if (st->type == ORCUS_TYPE_INT) {
		ss->data.i = ss->data.i > st->data.i;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 2);
		return;
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_LET(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type == ORCUS_TYPE_FLOAT) {
		if (st->type == ORCUS_TYPE_INT) {
			st->type = ORCUS_TYPE_FLOAT;
			st->data.f = (orcus_float)st->data.i;
		}
	}
	else if (ss->type == ORCUS_TYPE_INT) {
		if (st->type == ORCUS_TYPE_FLOAT) {
			ss->data.f = (orcus_float)ss->data.i;
		}
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 1);
		return;
	}
	ss->type = ORCUS_TYPE_BOOL;

	if (st->type == ORCUS_TYPE_FLOAT) {
		ss->data.i = ss->data.f < st->data.f;
	}
	else if (st->type == ORCUS_TYPE_INT) {
		ss->data.i = ss->data.i < st->data.i;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 2);
		return;
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_GTE(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type == ORCUS_TYPE_FLOAT) {
		if (st->type == ORCUS_TYPE_INT) {
			st->type = ORCUS_TYPE_FLOAT;
			st->data.f = (orcus_float)st->data.i;
		}
	}
	else if (ss->type == ORCUS_TYPE_INT) {
		if (st->type == ORCUS_TYPE_FLOAT) {
			ss->data.f = (orcus_float)ss->data.i;
		}
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 1);
		return;
	}
	ss->type = ORCUS_TYPE_BOOL;

	if (st->type == ORCUS_TYPE_FLOAT) {
		ss->data.i = ss->data.f >= st->data.f;
	}
	else if (st->type == ORCUS_TYPE_INT) {
		ss->data.i = ss->data.i >= st->data.i;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 2);
		return;
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_LTE(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;

	if (ss->type == ORCUS_TYPE_FLOAT) {
		if (st->type == ORCUS_TYPE_INT) {
			st->type = ORCUS_TYPE_FLOAT;
			st->data.f = (orcus_float)st->data.i;
		}
	}
	else if (ss->type == ORCUS_TYPE_INT) {
		if (st->type == ORCUS_TYPE_FLOAT) {
			ss->data.f = (orcus_float)ss->data.i;
		}
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, ss, 1);
		return;
	}
	ss->type = ORCUS_TYPE_BOOL;

	if (st->type == ORCUS_TYPE_FLOAT) {
		ss->data.i = ss->data.f <= st->data.f;
	}
	else if (st->type == ORCUS_TYPE_INT) {
		ss->data.i = ss->data.i <= st->data.i;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, st, 2);
		return;
	}

	M->sizeofstack--;
	return;
}

void ORCUS_BYTECODE_SENV(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* key = M->stack + M->sizeofstack -1;
	orcus_variable* value = M->stack + M->sizeofstack - 2;

	orcus_state* check_state = M;
	while (check_state) {
		orcus_subtable* st = check_state->enviroment->data+key->type;
		orcus_int sz = st->size;
		while (sz) {
			sz--;
			if (orcus_vars_equal(st->keys + sz, key)) {
				orcus_increment_variable_refrence_count(value);
				orcus_decrement_variable_refrence_count(st->values + sz);
				orcus_release_var(key);
				orcus_release_var(st->values + sz);
				st->values[sz] = *value;
				M->sizeofstack -= 2;
				return;
			}
		}


		check_state = check_state->parentstate;
	}

	orcus_subtable st = M->enviroment->data[key->type];
	void* nptr = realloc(st.keys, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.keys = (orcus_variable*)nptr;
	nptr = realloc(st.values, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.values = (orcus_variable*)nptr;

	orcus_increment_variable_refrence_count(key);
	orcus_increment_variable_refrence_count(value);

	st.keys[st.size] = *key;
	st.values[st.size] = *value;

	st.size++;
	M->enviroment->data[key->type] = st;
	M->sizeofstack -= 2;
}

void ORCUS_BYTECODE_GENV(orcus_state* const ORCUS_CPP_restrict M) {
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* key = M->stack + M->sizeofstack - 1;

	orcus_state* check_state = M;
	while (check_state) {
		orcus_subtable st= check_state->enviroment->data[key->type];
		while (st.size){
			st.size--;
			if (orcus_vars_equal(st.keys+ st.size, key)) {
				orcus_release_var(key);
				*key = st.values[st.size];
				return;
			}
		}


		check_state = check_state->parentstate;
	}
	orcus_release_var(key);
	key->data.i = 0;
	key->type = ORCUS_TYPE_NIL;
}


void ORCUS_BYTECODE_SET(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack<3) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* key = M->stack + M->sizeofstack - 1;
	orcus_variable* table = M->stack + M->sizeofstack - 2;
	orcus_variable* value = M->stack + M->sizeofstack - 3;

	switch (table->type) {
	case ORCUS_TYPE_TABLE:
		orcus_setkey((orcus_table*)table->data.p, key, value);
		break;
	case ORCUS_TYPE_ARRAY:
		if (key->type != ORCUS_TYPE_INT) {
			orcus_raise_error_nonpointer(M, ORCUS_ERROR_INVALID_INDEX, key->type);
			return;
		}
		orcus_setarray((orcus_array*)table->data.p, value, key->data.i);
		break;
	default:
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_STORAGETYPE_VARIABLEPROVIDED, &table, 1);
		return;
	}
	M->sizeofstack -= 3;
	//orcus_release_var(&table); this breaks tables defined like a={1,2,3}
}

void ORCUS_BYTECODE_GET(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* key = M->stack + M->sizeofstack - 1;
	orcus_variable* table = M->stack + M->sizeofstack - 2;
	orcus_variable value;
	switch (table->type) {
	case ORCUS_TYPE_TABLE:
		orcus_getkey((orcus_table*)table->data.p, key, &value);
		break;
	case ORCUS_TYPE_ARRAY:
		if (key->type != ORCUS_TYPE_INT) {
			orcus_raise_error_nonpointer(M, ORCUS_ERROR_INVALID_INDEX, key->type);
			return;
		}
		orcus_getarray((orcus_array*)table->data.p, key->data.i, &value);
		break;
	case ORCUS_TYPE_STRING:
		if (key->type != ORCUS_TYPE_INT) {
			orcus_raise_error_nonpointer(M, ORCUS_ERROR_INVALID_INDEX, key->type);
			return;
		}
		
		value.type = ORCUS_TYPE_STRING;
		value.data.p = orcus_mstring_substring((orcus_string*)table->data.p, key->data.i, key->data.i);
		break;
	default:
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_STORAGETYPE_VARIABLEPROVIDED, table, 1);
		return;
	}
	orcus_increment_variable_refrence_count(&value); //so the value cannot be freed before it is sent out
	orcus_release_var(table);
	orcus_decrement_variable_refrence_count(&value);
	*table = value;
	M->sizeofstack--;
}



void ORCUS_BYTECODE_SREG(orcus_state* const ORCUS_CPP_restrict M) {
	const orcus_uint regnum = *(orcus_uint*)(M->bytecode->instructions + M->programcounter);

	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;

	orcus_variable v;
	if (regnum <= register_max) {
		orcus_popstack(M, &v);
		M->registers[regnum] = v;
	}
	else {
		orcus_discard_top_of_stack(M);
	}

}


void ORCUS_BYTECODE_GREG(orcus_state* const ORCUS_CPP_restrict M) {
	const orcus_uint regnum = *(orcus_uint*)(M->bytecode->instructions + M->programcounter);

	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;

	if (regnum <= register_max) {
		orcus_pushstack(M, M->registers+regnum);
	}
	else {
		orcus_variable out;
		
		out.type = ORCUS_TYPE_NIL;
		out.data.i = 0;
		orcus_pushstack(M, &out);
	}

}

void ORCUS_BYTECODE_NINT(orcus_state* const ORCUS_CPP_restrict M) { //New INTeger
	void* const offset = M->bytecode->instructions + M->programcounter;

	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;

	orcus_variable out;
	
	out.type = ORCUS_TYPE_INT;
	out.data.i= *(orcus_int*)offset;
	orcus_pushstack(M, &out);
}

void ORCUS_BYTECODE_NFLO(orcus_state* const ORCUS_CPP_restrict M) { //New FLOat
	void* const offset = M->bytecode->instructions + M->programcounter;

	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;

	orcus_variable out;
	
	out.type = ORCUS_TYPE_FLOAT;
	out.data.f = *(orcus_float*)offset;
	orcus_pushstack(M, &out);
}

void ORCUS_BYTECODE_NTRU(orcus_state* const ORCUS_CPP_restrict M) { //New TRUe
	orcus_variable out;
	
	out.type = ORCUS_TYPE_BOOL;
	out.data.i = 1;
	orcus_pushstack(M, &out);
}

void ORCUS_BYTECODE_NFAL(orcus_state* const ORCUS_CPP_restrict M) { //New FALse
	orcus_variable out;
	
	out.type = ORCUS_TYPE_BOOL;
	out.data.i = 0;
	orcus_pushstack(M, &out);
}

void ORCUS_BYTECODE_NNIL(orcus_state* const ORCUS_CPP_restrict M) { //New NIL
	orcus_variable out;
	
	out.type = ORCUS_TYPE_NIL;
	out.data.i = 0;
	orcus_pushstack(M, &out);
}

void ORCUS_BYTECODE_NSTR(orcus_state* const ORCUS_CPP_restrict M) { //New STRing
	const orcus_uint string_size = *(orcus_uint*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;

	orcus_variable out;
	
	orcus_string* so=orcus_cstring_to_mstring( (char*)(M->bytecode->instructions + M->programcounter), string_size);
	if (!so) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	out.data.p = so;
	out.type = ORCUS_TYPE_STRING;
	M->programcounter += (string_size+ sizeof(orcus_opcode)-1)/sizeof(orcus_opcode);

	orcus_pushstack(M, &out);
}

void ORCUS_BYTECODE_NFUN(orcus_state* const ORCUS_CPP_restrict M) { //New FUNction / No FUN
	const orcus_uint function_size = *(orcus_uint*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;

	orcus_function* const fptr= (orcus_function*)malloc(sizeof(orcus_function));
	if (fptr == nullptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}

	if (M->bytecode->numberofinstructions < M->programcounter + function_size) {
		orcus_raise_error(M, ORCUS_ERROR_INSTRUCTION_FAILIURE);
		return;
	}

	fptr->references = 0;
	fptr->numberofinstructions = function_size;
	fptr->instructions = (orcus_opcode*)malloc(function_size * sizeof(orcus_opcode));
	fptr->enviromental = false;
	
	if (fptr->instructions == nullptr) {
		free(fptr);
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}


	memcpy(fptr->instructions, M->bytecode->instructions + M->programcounter, function_size * sizeof(orcus_opcode));

	if (M->bytecode->instruction_dbg_lookup) {
		fptr->instruction_dbg_lookup = (orcus_uint*)malloc(sizeof(orcus_uint) * function_size);
		if (fptr->instruction_dbg_lookup == nullptr) {
			free(fptr->instructions);
			free(fptr);
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		memcpy(fptr->instruction_dbg_lookup, M->bytecode->instruction_dbg_lookup + M->programcounter, function_size * sizeof(orcus_uint));
		fptr->dbg_tokens = (orcus_debug_token*)malloc(sizeof(orcus_debug_token) * M->bytecode->num_dbg_tokens);
		if (fptr->dbg_tokens == nullptr) {
			free(fptr->instructions);
			free(fptr->instruction_dbg_lookup);
			free(fptr);
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		for (orcus_uint i = 0; i < M->bytecode->num_dbg_tokens; i++) {
			fptr->dbg_tokens[i] = M->bytecode->dbg_tokens[i];
			fptr->dbg_tokens[i].chars = (char*)malloc(M->bytecode->dbg_tokens[i].num_chars);
			if (!fptr->dbg_tokens[i].chars) {
				free(fptr->instructions);
				free(fptr->dbg_tokens);
				free(fptr->instruction_dbg_lookup);
				free(fptr);
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			fptr->num_dbg_tokens = M->bytecode->num_dbg_tokens;
			memcpy(fptr->dbg_tokens[i].chars, M->bytecode->dbg_tokens[i].chars, M->bytecode->dbg_tokens[i].num_chars);
		}
	}
	else {
		fptr->instruction_dbg_lookup = nullptr;
		fptr->num_dbg_tokens = 0;
		fptr->dbg_tokens = nullptr;
	}


	orcus_variable out;
	out.type = ORCUS_TYPE_FUNCTION;
	out.data.p = fptr;
	M->programcounter += function_size;

	orcus_pushstack(M, &out);
}

void ORCUS_BYTECODE_NTAB(orcus_state* const ORCUS_CPP_restrict M) { //New TABle
	orcus_variable out;
	
	const orcus_table* const ntab = orcus_newtable();
	if (ntab == nullptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	
	out.type = ORCUS_TYPE_TABLE;
	out.data.p = (void*)ntab;
	orcus_pushstack(M, &out);
}

void ORCUS_BYTECODE_NARR(orcus_state* const ORCUS_CPP_restrict M) { //New ARRay
	orcus_variable out;

	const orcus_array* const narr = orcus_newarray();
	if (narr == nullptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	
	out.type = ORCUS_TYPE_ARRAY;
	out.data.p = (void*)narr;
	orcus_pushstack(M, &out);
}

void ORCUS_BYTECODE_JMP(orcus_state* const ORCUS_CPP_restrict M) { //JuMP
	M->programcounter = *(orcus_uint*)(M->bytecode->instructions + M->programcounter);
}

void ORCUS_BYTECODE_JMPR(orcus_state* const ORCUS_CPP_restrict M) { //JuMP Relative
	M->programcounter += *(orcus_int*)(M->bytecode->instructions + M->programcounter);
}

void ORCUS_BYTECODE_JIF(orcus_state* const ORCUS_CPP_restrict M) { //Jump IF
	orcus_variable ck;
	orcus_popstack(M,&ck);
	if(orcus_checkbool(&ck)){
		M->programcounter = *(orcus_uint*)(M->bytecode->instructions + M->programcounter);
	}
	else {
		M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	}
	orcus_release_var(&ck);
}

void ORCUS_BYTECODE_JNIF(orcus_state* const ORCUS_CPP_restrict M) { //Jump Not IF
	orcus_variable ck;
	orcus_popstack(M, &ck);
	if (!orcus_checkbool(&ck)) {
		M->programcounter = *(orcus_uint*)(M->bytecode->instructions + M->programcounter);
	}
	else {
		M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	}
	orcus_release_var(&ck);
}

void ORCUS_BYTECODE_JRIF(orcus_state* const ORCUS_CPP_restrict M) { //Jump Relative IF
	orcus_variable ck;
	orcus_popstack(M, &ck);
	if (orcus_checkbool(&ck)) {
		M->programcounter += *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	}
	else {
		M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	}
	orcus_release_var(&ck);
}

void ORCUS_BYTECODE_JRNI(orcus_state* const ORCUS_CPP_restrict M) { //Jump Relative Not If
	orcus_variable ck;
	orcus_popstack(M, &ck);
	if (!orcus_checkbool(&ck)) {
		M->programcounter += *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	}
	else {
		M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	}
	orcus_release_var(&ck);
}

void ORCUS_BYTECODE_CALL(orcus_state* const ORCUS_CPP_restrict M) { //CALL function
	const orcus_int args_in = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	orcus_int initial_stack_size = M->sizeofstack;
	if (M->sizeofstack < (args_in+1) ) { //we need 1 extra arg for the function on the stack.
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	const orcus_int args_out = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	
	orcus_variable ck;
	orcus_popstack(M,&ck);
	switch (ck.type)
	{
	case ORCUS_TYPE_FUNCTION:
		{
		orcus_state* const FM= orcus_get_child_state(M);
		if (!FM) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		orcus_function* previous = FM->bytecode;

		//FM->bytecode->instructions = func->instructions;
		//FM->bytecode->numberofinstructions = func->numberofinstructions;
		//memcpy(&FM->bytecode, ck->data.p, sizeof(orcus_function));
		FM->bytecode = ((orcus_function*)ck.data.p);

		
		if (FM->allocatedstacksize < FM->sizeofstack+args_in) {
			void* nptr=realloc(FM->stack, sizeof(orcus_variable) * (FM->sizeofstack + args_in));
			if (!nptr) {
				orcus_release_var(&ck);
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			FM->stack = (orcus_variable*)nptr;
			FM->allocatedstacksize = FM->sizeofstack + args_in;
		}
		for (orcus_int i = 0; i < args_in; i++) {
			FM->stack[FM->sizeofstack + i] = M->stack[M->sizeofstack - i - 1];
		}
		FM->sizeofstack += args_in;
		M->sizeofstack -= args_in;


		while (orcus_stepstate(FM)) {};

		if (M->allocatedstacksize < M->sizeofstack + args_out) {
			void* nptr = realloc(M->stack, sizeof(orcus_variable) * (M->sizeofstack + args_out));
			if (!nptr) {
				orcus_release_var(&ck);
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			M->stack = (orcus_variable*)nptr;
			M->allocatedstacksize = M->sizeofstack + args_out;
		}
		orcus_int arg_defecit = args_out - FM->sizeofstack;
		if (arg_defecit < 0)arg_defecit = 0;
		memcpy(M->stack + M->sizeofstack, FM->stack, (args_out- arg_defecit) * sizeof(orcus_variable));
		orcus_int i = arg_defecit;
		while (i) {
			M->stack[M->sizeofstack + args_out - i].type = ORCUS_TYPE_NIL;
			M->stack[M->sizeofstack + args_out - i].data.i = 0;
			i--;
		}
		FM->sizeofstack -= args_out - arg_defecit;
		M->sizeofstack += args_out;
		
		//protect the output variables from being freed if their only refrenes are stored in the to-be-deleted enviromental table
		for (orcus_int n = M->sizeofstack; n > M->sizeofstack - args_out; n--) {
			orcus_increment_variable_refrence_count(M->stack + n -1);
		}
		FM->bytecode = previous;//so the bytecode isn't freed
		orcus_clearstate(FM);
		//then remove the refs we just added
		for (orcus_int n = M->sizeofstack; n > M->sizeofstack - args_out; n--) {
			orcus_decrement_variable_refrence_count(M->stack + n - 1);
		}

		}
		
		break;
	case ORCUS_TYPE_CFUNC:
		((orcus_cfunc)(ck.data.p))(M,args_in,args_out);
		break;
	default:
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_FUNCTION, &ck, 0);
		orcus_release_var(&ck);
		return;
	}
	orcus_release_var(&ck);

	if (initial_stack_size + args_out - args_in - 1 !=M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

}

void ORCUS_BYTECODE_END(orcus_state* const ORCUS_CPP_restrict M) { //end state execution
	M->programcounter = M->bytecode->numberofinstructions;
}

void ORCUS_BYTECODE_LEN(orcus_state* const ORCUS_CPP_restrict M) { //LENgth
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* var = M->stack + M->sizeofstack -1;
	orcus_int len;
	switch (var->type) {
	case ORCUS_TYPE_ARRAY:
		len = orcus_array_len((orcus_array*)var->data.p) + 1;
		orcus_release_var(var);
		var->type = ORCUS_TYPE_INT;
		var->data.i = len;
		break;
	case ORCUS_TYPE_STRING:
		
		
		len = ((orcus_string*)var->data.p)->size;
		orcus_release_var(var);
		var->type = ORCUS_TYPE_INT;
		var->data.i = len;
		break;
	case ORCUS_TYPE_TABLE:
		{
			len = 0;
			orcus_table* t = ((orcus_table*)var->data.p);
			for (uint8_t i = 0; i < ORCUS_NUMBER_OF_TYPES; i++) {
				len += t->data[i].size;
			}
			orcus_release_var(var);
			var->type = ORCUS_TYPE_INT;
			var->data.i = len;
		}	
		break;
	default:
		orcus_raise_error_nonpointer(M, ORCUS_ERROR_INDEX_INVALID_TYPE, var->type);
		return;
	}
	return;
}

void ORCUS_BYTECODE_CNCT(orcus_state* const ORCUS_CPP_restrict M) { // CoNCaTenate
	if (M->sizeofstack<2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* st = M->stack + M->sizeofstack - 1;
	orcus_variable* ss = M->stack + M->sizeofstack - 2;
	orcus_string* s1;
	orcus_string* s2;
	if (st->type == ORCUS_TYPE_STRING) {
		s2 = (orcus_string*)st->data.p;
	}
	else {
		s2 = orcus_tostring(st);
		if (!s2) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
	}

	if (ss->type == ORCUS_TYPE_STRING) {
		s1 = (orcus_string*)ss->data.p;
	}
	else {
		s1 = orcus_tostring(ss);
		if (!s1) {
			if(st->type!=ORCUS_TYPE_STRING)orcus_mstring_delete(s2);
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
	}

	if (!s2->size) {
		if (st->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s2);
		if (ss->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s1);
		orcus_release_var(st);
		M->sizeofstack--;
		return;
	}

	orcus_string* s3;
	char* buffer;
	if (!s1->references) {
		s3 = s1;
		if (!s1->constant) {
			buffer = (char*)realloc(s1->ptr,s1->size+s2->size);
			if (!buffer) {
				if (st->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s2);
				if (ss->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s1);
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			memcpy(buffer + s1->size, s2->ptr, s2->size);
		}
		else {
			buffer = (char*)malloc(s1->size + s2->size);
			if (!buffer) {
				if (st->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s2);
				if (ss->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s1);
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			memcpy(buffer, s1->ptr, s1->size);
			memcpy(buffer + s1->size, s2->ptr, s2->size);
		}
		s3->size += s2->size;
		if (st->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s2);
		orcus_release_var(st);
	}
	else if (!s2->references) {
		s3 = s2;
		buffer = (char*)malloc(s1->size + s2->size);
		if (!buffer) {
			if (st->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s2);
			if (ss->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s1);
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		memcpy(buffer, s1->ptr, s1->size);
		memcpy(buffer + s1->size, s2->ptr, s2->size);
		s3->size += s1->size;
		if (ss->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s1);
		orcus_release_var(ss);
		if(!s2->constant)free(s2->ptr);
	}
	else {
		s3 = (orcus_string*)malloc(sizeof(orcus_string));
		if (!s3) {
			if (st->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s2);
			if (ss->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s1);
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		buffer = (char*)malloc(s1->size + s2->size);
		if (!buffer) {
			if (st->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s2);
			if (ss->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s1);
			free(s3);
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		
		memcpy(buffer, s1->ptr, s1->size);
		memcpy(buffer+ s1->size, s2->ptr, s2->size);
		s3->references = 0;
		s3->size = s1->size + s2->size;
		if (st->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s2);
		if (ss->type != ORCUS_TYPE_STRING)orcus_mstring_delete(s1);

		orcus_release_var(ss);
		orcus_release_var(st);
	}
	
	s3->ptr = buffer;
	s3->constant = false;

	ss->type = ORCUS_TYPE_STRING;
	ss->data.p = s3;

	M->sizeofstack--;
	return;
}


void ORCUS_BYTECODE_CLS(orcus_state* const ORCUS_CPP_restrict M) { // CLear Stack
	//as simple as it gets, really.
	while (M->sizeofstack) {
		M->sizeofstack--;
		orcus_release_var(M->stack + M->sizeofstack);
	}
}


void ORCUS_BYTECODE_GETL(orcus_state* const ORCUS_CPP_restrict M) { //GET Local
	//yeah, this is pretty simple.
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* k = M->stack + M->sizeofstack - 1;

	orcus_subtable st = M->enviroment->data[k->type];
	orcus_int sz = st.size;
	while (sz) {
		sz--;
		if (orcus_vars_equal(st.keys+sz,k)) {
			orcus_release_var(k);
			*k=st.values[sz];
			return;
		}
	}
	orcus_release_var(k);
	k->type = ORCUS_TYPE_NIL;
	k->data.i = 0;
}

void ORCUS_BYTECODE_SETL(orcus_state* const ORCUS_CPP_restrict M) { //SET Local
	//ditto.
	
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* key = M->stack + M->sizeofstack - 1;
	orcus_variable* value = M->stack + M->sizeofstack - 2;

	orcus_subtable st = M->enviroment->data[key->type];
	orcus_int sz = st.size;
	while (sz) {
		sz--;
		if (orcus_vars_equal(st.keys + sz, key)) {
			orcus_increment_variable_refrence_count(value);
			orcus_decrement_variable_refrence_count(st.values + sz);
			orcus_release_var(key);
			orcus_release_var(st.values + sz);
			st.values[sz] = *value;
			M->sizeofstack -= 2;
			return;
		}
	}

	void* nptr=realloc(st.keys, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.keys = (orcus_variable*)nptr;
	nptr=realloc(st.values, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.values = (orcus_variable*)nptr;

	orcus_increment_variable_refrence_count(key);
	orcus_increment_variable_refrence_count(value);
	st.keys[st.size] = *key;
	st.values[st.size] = *value;

	st.size++;
	M->enviroment->data[key->type] = st;
	M->sizeofstack -= 2;
}

void ORCUS_BYTECODE_GETG(orcus_state* const ORCUS_CPP_restrict M) { //GET Global
	//ditto.
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* k = M->stack + M->sizeofstack - 1;

	orcus_subtable st = M->masterstate->enviroment->data[k->type];
	orcus_int sz = st.size;
	while (sz) {
		sz--;
		if (orcus_vars_equal(st.keys + sz, k)) {
			orcus_release_var(k);
			*k = st.values[sz];
			return;
		}
	}
	orcus_release_var(k);
	k->type = ORCUS_TYPE_NIL;
	k->data.i = 0;
}

void ORCUS_BYTECODE_SETG(orcus_state* const ORCUS_CPP_restrict M) { //SET Global
		//ditto.
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* key = M->stack + M->sizeofstack - 1;
	orcus_variable* value = M->stack + M->sizeofstack - 2;

	orcus_subtable st = M->masterstate->enviroment->data[key->type];
	orcus_int sz = st.size;
	while (sz) {
		sz--;
		if (orcus_vars_equal(st.keys + sz, key)) {
			orcus_increment_variable_refrence_count(value);
			orcus_decrement_variable_refrence_count(st.values + sz);
			orcus_release_var(key);
			orcus_release_var(st.values + sz);
			st.values[sz] = *value;
			M->sizeofstack -= 2;
			return;
		}
	}

	void* nptr = realloc(st.keys, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.keys = (orcus_variable*)nptr;
	nptr = realloc(st.values, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.values = (orcus_variable*)nptr;

	orcus_increment_variable_refrence_count(key);
	orcus_increment_variable_refrence_count(value);
	st.keys[st.size] = *key;
	st.values[st.size] = *value;

	st.size++;
	M->masterstate->enviroment->data[key->type] = st;
	M->sizeofstack -= 2;
}

void ORCUS_BYTECODE_CPYT(orcus_state* const ORCUS_CPP_restrict M) { // CoPY Top (of stack)
	if (!M->sizeofstack)return; //nothing on stack, nothing to copy.

	orcus_variable v = *(M->stack + (M->sizeofstack - 1));
	orcus_pushstack(M, &v );
}


void ORCUS_BYTECODE_SWPT(orcus_state* const ORCUS_CPP_restrict M) { //SWaP Top. swaps the top and second top of stack.
	//so basically, 1,2 -> 2,1

	if (M->sizeofstack > 1) { //switch the top 2 elements
		const orcus_variable top = M->stack[M->sizeofstack - 1];
		M->stack[M->sizeofstack - 1] = M->stack[M->sizeofstack - 2];
		M->stack[M->sizeofstack - 2] = top;
	}
	else if (M->sizeofstack == 1) { //if there's 1 element, pushing nil does the same thing, basically.
		orcus_variable nn;
		nn.data.i = 0;
		nn.type = ORCUS_TYPE_NIL;
		orcus_pushstack(M, &nn);
	}
	//no stack? nothing to do.

}

void ORCUS_BYTECODE_CPYX(orcus_state* const ORCUS_CPP_restrict M) { // CoPY X elements (from top of stack)
	if (!M->sizeofstack)return; //nothing on stack, nothing to copy.

	const orcus_uint num = *(orcus_uint*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;

	orcus_uint start_stack_size = M->sizeofstack;
	for (orcus_uint i = 0; i < num; i++) {

		const orcus_int index = start_stack_size - num + i;
		if (index < 0) { //do not break, we need to gaurentee the stack size.
			ORCUS_BYTECODE_NNIL(M);
			continue;
		}

		orcus_pushstack(M, M->stack+index);
	}

}




void ORCUS_BYTECODE_UNM(orcus_state* const ORCUS_CPP_restrict M) { //UNary Minus
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* var = M->stack + M->sizeofstack - 1;

	switch (var->type)
	{
	case ORCUS_TYPE_INT:
		var->data.i*=-1;
		break;
	case ORCUS_TYPE_FLOAT:
		var->data.f *= -1.0;
		break;
	default:
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, var, 1);
		return;
	}
	return;
}

void ORCUS_BYTECODE_INC(orcus_state* const ORCUS_CPP_restrict M) { //INCrement
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* var = M->stack + M->sizeofstack - 1;

	switch (var->type)
	{
	case ORCUS_TYPE_INT:
		var->data.i++;
		break;
	case ORCUS_TYPE_FLOAT:
		var->data.f += 1.0;
		break;
	default:
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, var, 1);
		return;
	}
	return;
}

void ORCUS_BYTECODE_DEC(orcus_state* const ORCUS_CPP_restrict M) { //DECrement
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* var = M->stack+M->sizeofstack - 1;

	switch (var->type)
	{
	case ORCUS_TYPE_INT:
		var->data.i--;
		break;
	case ORCUS_TYPE_FLOAT:
		var->data.f -= 1.0;
		break;
	default:
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, var, 1);
		return;
	}
	return;
}

void ORCUS_BYTECODE_SCON(orcus_state* const ORCUS_CPP_restrict M) { //Set CONstant
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	const orcus_uint con_num = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;

	if (con_num >= M->num_constants) {
		void* const nptr=realloc(M->constants, sizeof(orcus_variable) * (con_num+1) );
		if (!nptr) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		M->num_constants = con_num+1;
		M->constants = (orcus_variable*)nptr;
	}
	
	M->sizeofstack--;
	orcus_increment_variable_refrence_count(M->stack+M->sizeofstack);
	M->constants[con_num] = M->stack[M->sizeofstack];
}

void ORCUS_BYTECODE_GCON(orcus_state* const ORCUS_CPP_restrict M) { //Get CONstant
	const orcus_uint con_num = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	if (con_num >= M->num_constants) {
		ORCUS_BYTECODE_NNIL(M);
		return;
	}

	orcus_pushstack(M, M->constants + con_num);
}

void ORCUS_BYTECODE_SWXY(orcus_state* const ORCUS_CPP_restrict M) { //SWap stack X and Y
	orcus_uint X = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	orcus_uint Y = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;

	if (X >= M->sizeofstack || Y >= M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_INSTRUCTION_FAILIURE);
		return;
	}

	X = M->sizeofstack - X - 1;
	Y = M->sizeofstack - Y - 1;

	orcus_variable v_X = M->stack[X];
	M->stack[X] = M->stack[Y];
	M->stack[Y] = v_X;
}

void ORCUS_BYTECODE_NNSE(orcus_state* const ORCUS_CPP_restrict M) { //New Nil if Stack Empty
	if (!M->sizeofstack) {
		if (!M->allocatedstacksize) {
			void* nptr=realloc(M->stack, sizeof(orcus_variable));
			if (!nptr) {
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			M->stack = (orcus_variable*)nptr;
		}
		M->stack[0].type = ORCUS_TYPE_NIL;
		M->stack[0].data.i = 0;
		M->sizeofstack++;
	}
}


void ORCUS_BYTECODE_SETL_KCS(orcus_state* const ORCUS_CPP_restrict M) { //SET Local, Key is Constant String
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* value = M->stack + M->sizeofstack - 1;

	const orcus_int length = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	char* chars = (char*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += (length + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);

	orcus_subtable st = M->enviroment->data[ORCUS_TYPE_STRING];
	orcus_int sz = st.size;
	while (sz) {
		sz--;
		orcus_string* s = (orcus_string*)st.keys[sz].data.p;
		if (s->size != length)continue;
		for (orcus_int n = 0; n < length; n++) {
			if (chars[n] != s->ptr[n])goto nextstring;
		}
		orcus_increment_variable_refrence_count(value);
		orcus_decrement_variable_refrence_count(st.values + sz);
		orcus_release_var(st.values + sz);
		st.values[sz] = *value;
		M->sizeofstack--;
		return;
		nextstring:
		{}
	}

	void* nptr = realloc(st.keys, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.keys = (orcus_variable*)nptr;
	nptr = realloc(st.values, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.values = (orcus_variable*)nptr;

	orcus_variable key;
	key.type = ORCUS_TYPE_STRING;
	orcus_string* keystr = orcus_cstring_to_mstring(chars, length);
	if (!keystr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	keystr->references = 1;
	key.data.p = keystr;

	orcus_increment_variable_refrence_count(value);
	st.keys[st.size] = key;
	st.values[st.size] = *value;

	st.size++;
	M->enviroment->data[ORCUS_TYPE_STRING] = st;
	M->sizeofstack --;
}

void ORCUS_BYTECODE_GETL_KCS(orcus_state* const ORCUS_CPP_restrict M) { //GET Local, Key is Constant String
	const orcus_int length = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	char* chars = (char*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += (length + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);

	orcus_subtable st = M->enviroment->data[ORCUS_TYPE_STRING];
	orcus_int sz = st.size;
	while (sz) {
		sz--;
		orcus_string* s = (orcus_string*)st.keys[sz].data.p;
		if (s->size != length)continue;
		for (orcus_int n = 0; n < length; n++) {
			if (chars[n] != s->ptr[n])goto nextstring;
		}
		orcus_pushstack(M, st.values + sz);
		return;
		nextstring:
		{}
	}
	orcus_variable k;
	k.type = ORCUS_TYPE_NIL;
	k.data.i = 0;
	orcus_pushstack(M, &k);
}

void ORCUS_BYTECODE_SETG_KCS(orcus_state* const ORCUS_CPP_restrict M) { //SET Global, Key is Constant String
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* value = M->stack + M->sizeofstack - 1;

	const orcus_int length = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	char* chars = (char*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += (length + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);

	orcus_subtable st = M->masterstate->enviroment->data[ORCUS_TYPE_STRING];
	orcus_int sz = st.size;
	while (sz) {
		sz--;
		orcus_string* s = (orcus_string*)st.keys[sz].data.p;
		if (s->size != length)continue;
		for (orcus_int n = 0; n < length; n++) {
			if (chars[n] != s->ptr[n])goto nextstring;
		}
		orcus_increment_variable_refrence_count(value);
		orcus_decrement_variable_refrence_count(st.values + sz);
		orcus_release_var(st.values + sz);
		st.values[sz] = *value;
		M->sizeofstack--;
		return;
	nextstring:
		{}
	}

	void* nptr = realloc(st.keys, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.keys = (orcus_variable*)nptr;
	nptr = realloc(st.values, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.values = (orcus_variable*)nptr;

	orcus_variable key;
	key.type = ORCUS_TYPE_STRING;
	orcus_string* keystr = orcus_cstring_to_mstring(chars, length);
	if (!keystr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	keystr->references = 1;
	key.data.p = keystr;

	orcus_increment_variable_refrence_count(value);
	st.keys[st.size] = key;
	st.values[st.size] = *value;

	st.size++;
	M->masterstate->enviroment->data[ORCUS_TYPE_STRING] = st;
	M->sizeofstack--;
}

void ORCUS_BYTECODE_GETG_KCS(orcus_state* const ORCUS_CPP_restrict M) { //GET Global, Key is Constant String
	const orcus_int length = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	char* chars = (char*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += (length + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);

	orcus_subtable st = M->masterstate->enviroment->data[ORCUS_TYPE_STRING];
	orcus_int sz = st.size;
	while (sz) {
		sz--;
		orcus_string* s = (orcus_string*)st.keys[sz].data.p;
		if (s->size != length)continue;
		for (orcus_int n = 0; n < length; n++) {
			if (chars[n] != s->ptr[n])goto nextstring;
		}
		orcus_pushstack(M, st.values + sz);
		return;
	nextstring:
		{}
	}
	orcus_variable k;
	k.type = ORCUS_TYPE_NIL;
	k.data.i = 0;
	orcus_pushstack(M, &k);
}

void ORCUS_BYTECODE_SENV_KCS(orcus_state* const ORCUS_CPP_restrict M) {
	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* value = M->stack + M->sizeofstack - 1;

	const orcus_int length = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	char* chars = (char*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += (length + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);

	orcus_state* check_state = M;
	while (check_state) {
		orcus_subtable* st = check_state->enviroment->data + ORCUS_TYPE_STRING;
		orcus_int sz = st->size;
		while (sz) {
			sz--;
			orcus_string* s = (orcus_string*)st->keys[sz].data.p;
			if (s->size != length)continue;
			for (orcus_int n = 0; n < length; n++) {
				if (chars[n] != s->ptr[n])goto nextstring;
			}
			orcus_increment_variable_refrence_count(value);
			orcus_decrement_variable_refrence_count(st->values + sz);
			orcus_release_var(st->values + sz);
			st->values[sz] = *value;
			M->sizeofstack--;
			return;
			nextstring:
			{}
		}


		check_state = check_state->parentstate;
	}

	orcus_subtable st = M->enviroment->data[ORCUS_TYPE_STRING];
	void* nptr = realloc(st.keys, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.keys = (orcus_variable*)nptr;
	nptr = realloc(st.values, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.values = (orcus_variable*)nptr;

	orcus_variable key;
	key.type = ORCUS_TYPE_STRING;
	orcus_string* keystr = orcus_cstring_to_mstring(chars, length);
	if (!keystr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	keystr->references = 1;
	key.data.p = keystr;

	orcus_increment_variable_refrence_count(value);

	st.keys[st.size] = key;
	st.values[st.size] = *value;

	st.size++;
	M->enviroment->data[ORCUS_TYPE_STRING] = st;
	M->sizeofstack --;
}

void ORCUS_BYTECODE_GENV_KCS(orcus_state* const ORCUS_CPP_restrict M) {
	const orcus_int length = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	char* chars = (char*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += (length + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);

	orcus_state* check_state = M;
	while (check_state) {
		orcus_subtable st = check_state->enviroment->data[ORCUS_TYPE_STRING];
		while (st.size) {
			st.size--;
			orcus_string* s = (orcus_string*)st.keys[st.size].data.p;
			if (s->size != length)continue;
			for (orcus_int n = 0; n < length; n++) {
				if (chars[n] != s->ptr[n])goto nextstring;
			}
			orcus_pushstack(M, st.values + st.size);
			return;
			nextstring:
			{}
		}


		check_state = check_state->parentstate;
	}
	orcus_variable k;
	k.type = ORCUS_TYPE_NIL;
	k.data.i = 0;
	orcus_pushstack(M, &k);
}


void ORCUS_BYTECODE_SET_KCS(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack < 2) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	const orcus_int length = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	char* chars = (char*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += (length + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);

	orcus_variable* table = M->stack + M->sizeofstack - 1;
	orcus_variable* value = M->stack + M->sizeofstack - 2;

	if (table->type != ORCUS_TYPE_TABLE) {
		if (table->type != ORCUS_TYPE_STRING && table->type != ORCUS_TYPE_ARRAY) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_STORAGETYPE, table, 1);
			return;
		}
		orcus_raise_error_nonpointer(M, ORCUS_ERROR_INVALID_INDEX, ORCUS_TYPE_STRING);
		return;
	}

	orcus_subtable st = ((orcus_table*)table->data.p)->data[ORCUS_TYPE_STRING];
	orcus_int sz = st.size;
	while (sz) {
		sz--;
		orcus_string* s = (orcus_string*)st.keys[sz].data.p;
		if (s->size != length)continue;
		for (orcus_int n = 0; n < length; n++) {
			if (chars[n] != s->ptr[n])goto nextstring;
		}
		orcus_increment_variable_refrence_count(value);
		orcus_decrement_variable_refrence_count(st.values + sz);
		orcus_release_var(st.values + sz);
		st.values[sz] = *value;
		M->sizeofstack-=2;
		return;
	nextstring:
		{}
	}

	void* nptr = realloc(st.keys, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.keys = (orcus_variable*)nptr;
	nptr = realloc(st.values, sizeof(orcus_variable) * (st.size + 1));
	if (!nptr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st.values = (orcus_variable*)nptr;

	orcus_variable key;
	key.type = ORCUS_TYPE_STRING;
	orcus_string* keystr = orcus_cstring_to_mstring(chars, length);
	if (!keystr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	keystr->references = 1;
	key.data.p = keystr;

	orcus_increment_variable_refrence_count(value);
	st.keys[st.size] = key;
	st.values[st.size] = *value;

	st.size++;
	((orcus_table*)table->data.p)->data[ORCUS_TYPE_STRING] = st;

	M->sizeofstack -= 2;
}

void ORCUS_BYTECODE_GET_KCS(orcus_state* const ORCUS_CPP_restrict M) {
	const orcus_int length = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
	char* chars = (char*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += (length + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);

	if (!M->sizeofstack) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}
	orcus_variable* table = M->stack + M->sizeofstack - 1;
	if (table->type != ORCUS_TYPE_TABLE) {
		if (table->type != ORCUS_TYPE_STRING && table->type != ORCUS_TYPE_ARRAY) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_STORAGETYPE, table, 1);
			return;
		}
		orcus_raise_error_nonpointer(M, ORCUS_ERROR_INVALID_INDEX, ORCUS_TYPE_STRING);
		return;
	}

	orcus_subtable st = ((orcus_table*)table->data.p)->data[ORCUS_TYPE_STRING];
	orcus_int sz = st.size;
	while (sz) {
		sz--;
		orcus_string* s = (orcus_string*)st.keys[sz].data.p;
		if (s->size != length)continue;
		for (orcus_int n = 0; n < length; n++) {
			if (chars[n] != s->ptr[n])goto nextstring;
		}
		orcus_release_var(table);
		*table = st.values[sz];
		return;
	nextstring:
		{}
	}
	orcus_release_var(table);
	table->data.i = 0;
	table->type = ORCUS_TYPE_NIL;
}

void ORCUS_BYTECODE_MSET(orcus_state* const ORCUS_CPP_restrict M) { //Multi SET variables
	const orcus_int number = *(orcus_int*)(M->bytecode->instructions + M->programcounter);
	M->programcounter += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;

	if (M->sizeofstack < (number * 2) + 1) {
		orcus_raise_error(M, ORCUS_ERROR_STACK_CORRUPT);
		return;
	}

	orcus_variable* storage_var = M->stack + M->sizeofstack - 1;

	orcus_int n;

	switch (storage_var->type) {
	case ORCUS_TYPE_TABLE:
		for (n = 0; n < number; n++) {
			orcus_setkey((orcus_table*)storage_var->data.p, storage_var - (n*2) -1, storage_var - (n * 2) - 2);
		}
		break;
	case ORCUS_TYPE_ARRAY:
		for (n = 0; n < number; n++) {
			orcus_variable* key = storage_var - (n * 2) - 1;
			if (key->type != ORCUS_TYPE_INT) {
				orcus_raise_error_nonpointer(M, ORCUS_ERROR_INVALID_INDEX, key->type);
				return;
			}
			orcus_setarray((orcus_array*)storage_var->data.p, storage_var - (n * 2) - 2 , key->data.i);
			orcus_release_var(key);
		}
		break;
	default:
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_STORAGETYPE_VARIABLEPROVIDED, &storage_var, 1);
		return;
	}

	M->sizeofstack -= number * 2;
	M->stack[M->sizeofstack - 1] = *storage_var; //keep the array/table on the stack
}


orcus_instruction orcus_bytecode_list[] = {
	ORCUS_BYTECODE_NOP, //0
	//arithmetic
	ORCUS_BYTECODE_ADD, //1
	ORCUS_BYTECODE_SUB,
	ORCUS_BYTECODE_MUL,
	ORCUS_BYTECODE_DIV,
	ORCUS_BYTECODE_POW,
	ORCUS_BYTECODE_IDIV,
	ORCUS_BYTECODE_MOD, //7
	//bitwise
	ORCUS_BYTECODE_BAND, //8
	ORCUS_BYTECODE_BOR,
	ORCUS_BYTECODE_BXOR,
	ORCUS_BYTECODE_BNOT,
	ORCUS_BYTECODE_BSHL,
	ORCUS_BYTECODE_BSHR, //13
	//logical
	ORCUS_BYTECODE_LAND, //14
	ORCUS_BYTECODE_LOR,
	ORCUS_BYTECODE_LXOR,
	ORCUS_BYTECODE_LNOT, //17
	//comparison
	ORCUS_BYTECODE_EQL, //18
	ORCUS_BYTECODE_NEQ,
	ORCUS_BYTECODE_GRT,
	ORCUS_BYTECODE_LET,
	ORCUS_BYTECODE_GTE,
	ORCUS_BYTECODE_LTE, //23
	//variables
	ORCUS_BYTECODE_SENV, //24
	ORCUS_BYTECODE_GENV,
	ORCUS_BYTECODE_SET,
	ORCUS_BYTECODE_GET,
	ORCUS_BYTECODE_SREG,
	ORCUS_BYTECODE_GREG, //29
	//variable creation
	ORCUS_BYTECODE_NINT, //30
	ORCUS_BYTECODE_NFLO,
	ORCUS_BYTECODE_NTRU,
	ORCUS_BYTECODE_NFAL,
	ORCUS_BYTECODE_NNIL,
	ORCUS_BYTECODE_NSTR,
	ORCUS_BYTECODE_NFUN,
	ORCUS_BYTECODE_NTAB,
	ORCUS_BYTECODE_NARR, //38
	//jumps
	ORCUS_BYTECODE_JMP, //39
	ORCUS_BYTECODE_JMPR,
	ORCUS_BYTECODE_JIF,
	ORCUS_BYTECODE_JRIF,
	ORCUS_BYTECODE_JNIF,
	ORCUS_BYTECODE_JRNI, //44
	//program flow
	ORCUS_BYTECODE_CALL, //45
	ORCUS_BYTECODE_END,
	ORCUS_BYTECODE_LEN,
	ORCUS_BYTECODE_CNCT,
	ORCUS_BYTECODE_CLS, //49

	ORCUS_BYTECODE_GETL, //50
	ORCUS_BYTECODE_SETL,
	ORCUS_BYTECODE_GETG,
	ORCUS_BYTECODE_SETG, //53

	ORCUS_BYTECODE_CPYT, //54
	ORCUS_BYTECODE_SWPT, //55
	ORCUS_BYTECODE_CPYX, //56

	ORCUS_BYTECODE_UNM, //57
	ORCUS_BYTECODE_INC,
	ORCUS_BYTECODE_DEC, //59

	ORCUS_BYTECODE_SCON, //60
	ORCUS_BYTECODE_GCON, //61

	ORCUS_BYTECODE_SWXY, //62

	ORCUS_BYTECODE_NNSE, //63

	ORCUS_BYTECODE_SETL_KCS, //64
	ORCUS_BYTECODE_GETL_KCS,
	ORCUS_BYTECODE_SETG_KCS,
	ORCUS_BYTECODE_GETG_KCS,
	ORCUS_BYTECODE_SENV_KCS,
	ORCUS_BYTECODE_GENV_KCS,
	ORCUS_BYTECODE_SET_KCS,
	ORCUS_BYTECODE_GET_KCS, //71

	ORCUS_BYTECODE_MSET, //72
};

