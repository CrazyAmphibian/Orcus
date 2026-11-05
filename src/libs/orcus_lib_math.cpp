#include"../orcus.hpp"
#include"../orcus_error.hpp"

#include "orcus_lib_math.hpp"

#include <math.h>
#include <stdlib.h>

//returns the smallest value. takes vararg. if no values are provided, returns infinity
void orcus_lib_math_min(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (!args_out) { 
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0);
		return; 
	}
	orcus_variable out;
	
#ifdef ORCUS_64BIT
	out.data.i = 0x7ff0000000000000;
#else
	out.data.i = 0x7f800000;
#endif
	out.type = ORCUS_TYPE_FLOAT;	

	//we can do a backwards stack loop with no issue because the order doesn't matter here
	for (orcus_int a = 0; a < args_in;a++) {
		orcus_variable var;
		orcus_popstack(M,&var);

		//we just ignore non-number variables.
		if (var.type == ORCUS_TYPE_FLOAT) {
			if (out.type == ORCUS_TYPE_FLOAT) {
				if (var.data.f< out.data.f) {
					out.data.f = var.data.f;
				}
			}
			else {
				if (var.data.f< (orcus_float)out.data.i) {
					out.type = ORCUS_TYPE_FLOAT;
					out.data.f = var.data.f;
				}
			}
		}
		else if (var.type == ORCUS_TYPE_INT) {
			if (out.type == ORCUS_TYPE_FLOAT) {
				if ((orcus_float)var.data.i < out.data.f) {
					out.type = ORCUS_TYPE_INT;
					out.data.i = var.data.i;
				}
			}
			else {
				if (var.data.i < out.data.i) {
					out.data.i = var.data.i;
				}
			}
		}

		orcus_release_var(&var);
	}

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



//returns the biggestvalue. takes vararg. if no values are provided, returns negative infinity
void orcus_lib_math_max(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (!args_out) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0);
		return;
	}
	orcus_variable out;
	
#ifdef ORCUS_64BIT
	out.data.i = 0xfff0000000000000;
#else
	out.data.i = 0xff800000;
#endif
	out.type = ORCUS_TYPE_FLOAT;

	//we can do a backwards stack loop with no issue because the order doesn't matter here
	for (orcus_int a = 0; a < args_in; a++) {
		orcus_variable var;
		orcus_popstack(M, &var);

		//we just ignore non-number variables.
		if (var.type == ORCUS_TYPE_FLOAT) {
			if (out.type == ORCUS_TYPE_FLOAT) {
				if (var.data.f > out.data.f) {
					out.data.f = var.data.f;
				}
			}
			else {
				if (var.data.f > (orcus_float)out.data.i) {
					out.type = ORCUS_TYPE_FLOAT;
					out.data.f = var.data.f;
				}
			}
		}
		else if (var.type == ORCUS_TYPE_INT) {
			if (out.type == ORCUS_TYPE_FLOAT) {
				if ((orcus_float)var.data.i > out.data.f) {
					out.type = ORCUS_TYPE_INT;
					out.data.i = var.data.i;
				}
			}
			else {
				if (var.data.i > out.data.i) {
					out.data.i = var.data.i;
				}
			}
		}

		orcus_release_var(&var);
	}

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

//will return an int.
void orcus_lib_math_floor(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable out;
	
	out.data.i = 0;
	out.type = ORCUS_TYPE_INT;

	orcus_variable val;
	orcus_popstack(M,&val);
	if (val.type == ORCUS_TYPE_INT) {
		out.data.i = val.data.i;
	}
	else if (val.type == ORCUS_TYPE_FLOAT) {
		out.data.i=(orcus_int)floor(val.data.f);
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &val, 1);
		orcus_release_var(&val);
		return;
	}

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}


//will return an int.
void orcus_lib_math_ceil(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable out;
	
	out.data.f = 0;
	out.type = ORCUS_TYPE_INT;

	orcus_variable val;
	orcus_popstack(M, &val);
	if (val.type == ORCUS_TYPE_INT) {
		out.data.i = val.data.i;
	}
	else if (val.type == ORCUS_TYPE_FLOAT) {
		out.data.i = (orcus_int)ceil(val.data.f);
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &val, 1);
		orcus_release_var(&val);
		return;
	}

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}




void orcus_lib_math_to_radians(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //degrees to radians
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable out;
	
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_variable val;
	orcus_popstack(M, &val);
	if (val.type == ORCUS_TYPE_INT) {
		out.data.f = (orcus_float)val.data.i;
	}
	else if (val.type == ORCUS_TYPE_FLOAT) {
		out.data.f = val.data.f;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &val, 1);
		orcus_release_var(&val);
		return;
	}

	out.data.f = (orcus_math_pi*out.data.f)/180.0;

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_math_to_degrees(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //radians to degrees
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable out;
	
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_variable val;
	orcus_popstack(M, &val);
	if (val.type == ORCUS_TYPE_INT) {
		out.data.f = (orcus_float)val.data.i;
	}
	else if (val.type == ORCUS_TYPE_FLOAT) {
		out.data.f = val.data.f;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &val, 1);
		orcus_release_var(&val);
		return;
	}

	out.data.f = (180.0 * out.data.f) / orcus_math_pi;

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_math_log(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1,2)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		if (args_in > 1)orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable out;
	
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_variable base;
	
	orcus_variable num;

	if (args_in == 1) {
		orcus_popstack(M,&num);
		base.type = ORCUS_TYPE_NIL;
	}
	else {
		orcus_popstack(M,&base);
		if (base.type == ORCUS_TYPE_INT) {
			base.data.f = (orcus_float)base.data.i;
			base.type = ORCUS_TYPE_FLOAT;
		}
		else if (base.type != ORCUS_TYPE_FLOAT) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &base, 2);
			orcus_release_var(&base);
			return;
		}
		orcus_popstack(M, &num);
	}

	if (num.type == ORCUS_TYPE_INT) {
		num.data.f = (orcus_float)num.data.i;
		num.type = ORCUS_TYPE_FLOAT;
	}
	else if (num.type != ORCUS_TYPE_FLOAT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &num, 1);
		orcus_release_var(&num);
		return;
	}


	if (base.type) {
		out.data.f = log(num.data.f) / log(base.data.f);
	}
	else {
		out.data.f = log(num.data.f);
	}

	orcus_pushstack(M,&out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}



void orcus_lib_math_to_absolute(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //absolute value
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable out;
	
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_variable val;
	orcus_popstack(M, &val);
	if (val.type == ORCUS_TYPE_INT) {
		out.type = ORCUS_TYPE_INT;
		out.data.i =abs(val.data.i);
	}
	else if (val.type == ORCUS_TYPE_FLOAT) { //since floats use a sign bit, we can use a simple bitwise to do stuff fast.
#ifdef ORCUS_64BIT
		out.data.i = 0x7FFFFFFFFFFFFFFF & val.data.i;
#else
		out.data.i = 0x7FFFFFFF &val.data.i;
#endif
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &val, 1);
		orcus_release_var(&val);
		return;
	}

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}




void orcus_lib_math_to_sin(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //sine
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable out;
	
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_variable val;
	orcus_popstack(M, &val);
	if (val.type == ORCUS_TYPE_INT) {
		out.data.f = (orcus_float)val.data.i;
	}
	else if (val.type == ORCUS_TYPE_FLOAT) {
		out.data.f = val.data.f;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &val, 1);
		orcus_release_var(&val);
		return;
	}

	out.data.f = sin( out.data.f);

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

void orcus_lib_math_to_cos(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //cosine
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable out;
	
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_variable val;
	orcus_popstack(M, &val);
	if (val.type == ORCUS_TYPE_INT) {
		out.data.f = (orcus_float)val.data.i;
	}
	else if (val.type == ORCUS_TYPE_FLOAT) {
		out.data.f = val.data.f;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &val, 1);
		orcus_release_var(&val);
		return;
	}

	out.data.f = cos(out.data.f);

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_math_to_tan(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //tangent
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable out;
	
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_variable val;
	orcus_popstack(M, &val);
	if (val.type == ORCUS_TYPE_INT) {
		out.data.f = (orcus_float)val.data.i;
	}
	else if (val.type == ORCUS_TYPE_FLOAT) {
		out.data.f = val.data.f;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &val, 1);
		orcus_release_var(&val);
		return;
	}

	out.data.f = tan(out.data.f);

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

void orcus_lib_math_to_asin(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //arcsine
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable out;
	
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_variable val;
	orcus_popstack(M, &val);
	if (val.type == ORCUS_TYPE_INT) {
		out.data.f = (orcus_float)val.data.i;
	}
	else if (val.type == ORCUS_TYPE_FLOAT) {
		out.data.f = val.data.f;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &val, 1);
		orcus_release_var(&val);
		return;
	}

	out.data.f = asin(out.data.f);

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

void orcus_lib_math_to_acos(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //arccosine
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable out;
	
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_variable val;
	orcus_popstack(M, &val);
	if (val.type == ORCUS_TYPE_INT) {
		out.data.f = (orcus_float)val.data.i;
	}
	else if (val.type == ORCUS_TYPE_FLOAT) {
		out.data.f = val.data.f;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &val, 1);
		orcus_release_var(&val);
		return;
	}

	out.data.f = acos(out.data.f);

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_math_to_atan(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //arctangent
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable out;
	
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_variable val;
	orcus_popstack(M, &val);
	if (val.type == ORCUS_TYPE_INT) {
		out.data.f = (orcus_float)val.data.i;
	}
	else if (val.type == ORCUS_TYPE_FLOAT) {
		out.data.f = val.data.f;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &val, 1);
		orcus_release_var(&val);
		return;
	}

	out.data.f = atan(out.data.f);

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

void orcus_lib_math_to_atan2(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable out;
	
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_variable val;
	orcus_popstack(M, &val);
	if (val.type == ORCUS_TYPE_INT) {
		out.data.f = (orcus_float)val.data.i;
	}
	else if (val.type == ORCUS_TYPE_FLOAT) {
		out.data.f = val.data.f;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &val, 2);
		orcus_release_var(&val);
		return;
	}

	orcus_variable y;
	orcus_popstack(M,&y);
	if (y.type == ORCUS_TYPE_INT) {
		y.data.f = (orcus_float)y.data.i;
	}
	else if (y.type == ORCUS_TYPE_FLOAT) {
		y.data.f = y.data.f;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &y, 1);
		orcus_release_var(&y);
		return;
	}

	out.data.f = atan2(y.data.f,out.data.f);

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



/*
modified xorshift*.
64 bit-ish. 32 bit saved state, and 32 bit output. better than stdlib's 15 bits on windows, at least.
this gives us compatability between 32 and 64 bit systems (not accounting for float math differences with float random).
also, since we controll it, we can easily save the state, or change it.
*/
uint64_t M_RANDOM_STATE =0x0000BEEFFACE0000 ;
const uint32_t M_RANDOM_MAX = 0xFFFFFFFF;
inline uint32_t m_random() {
	M_RANDOM_STATE ^= M_RANDOM_STATE >> 12;
	M_RANDOM_STATE ^= M_RANDOM_STATE << 25;
	M_RANDOM_STATE ^= M_RANDOM_STATE >> 27;
	M_RANDOM_STATE ;
	uint32_t out = ((M_RANDOM_STATE * 0x2545F4914F6CDD1Du) & 0x0000FFFFFFFF0000) >> 16;
	// AAAABBBBCCCCDDDD -> ----CCCCBBBB----
	M_RANDOM_STATE = ((M_RANDOM_STATE & 0xFFFF0000) << 16) | ((M_RANDOM_STATE & 0xFFFF00000000) >> 16);
	return out;
}





void orcus_lib_math_random(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0,2)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (args_in ==1) {
		orcus_raise_error_nonpointer(M, ORCUS_ERROR_NOT_ENOUGH_ARGS, args_in, 2);
		return;
	};

	orcus_variable v2;
	orcus_variable v1;
	if (args_in>0)orcus_popstack(M, &v2);
	if (args_in>1)orcus_popstack(M, &v1);

	if (args_in && (v1.type != ORCUS_TYPE_INT) && (v1.type != ORCUS_TYPE_FLOAT) ) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &v1,1);
		orcus_release_var(&v1);
		return;
	}

	if (args_in &&  (v2.type != ORCUS_TYPE_INT) && (v2.type != ORCUS_TYPE_FLOAT)) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &v2, 2);
		orcus_release_var(&v2);
		return;
	}

	uint32_t r = m_random();
	orcus_float f = (orcus_float)r / (orcus_float)M_RANDOM_MAX;

	if (!args_out)return;


	orcus_variable out;
	
	out.type = ORCUS_TYPE_FLOAT;
	if (!args_in) {
		out.data.f = f;
	}else{
		orcus_float min=0.0;
		orcus_float max=0.0;
		if (v2.type == ORCUS_TYPE_INT) {
			max = (orcus_float)v2.data.i;
		}
		else {
			max = v2.data.f;
		}

		if (v1.type == ORCUS_TYPE_INT) {
			min = (orcus_float)v1.data.i;
		}
		else {
			min = v1.data.f;
		}

		out.data.f = (f*(max-min)) + min;
	}

	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_math_randomint(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}

	orcus_variable v2;
	orcus_popstack(M,&v2);
	orcus_variable v1;
	orcus_popstack(M,&v1);

	if ( (v1.type != ORCUS_TYPE_INT)) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &v1, ORCUS_TYPE_INT, 1);
		orcus_release_var(&v1);
		return;
	}

	if ( (v2.type != ORCUS_TYPE_INT)) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &v2, ORCUS_TYPE_INT, 2);
		orcus_release_var(&v2);
		return;
	}

	uint32_t r = m_random();

	if (!args_out)return;


	orcus_variable out;
	
	out.type = ORCUS_TYPE_INT;

	orcus_int min = v1.data.i;
	orcus_int max = v2.data.i;

	out.data.i = ((r) % (max - min + 1)) + min;

	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



void orcus_lib_math_randomseed(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}

	if (args_in) {
		orcus_variable v1;
		orcus_popstack(M,&v1);

		if ((v1.type != ORCUS_TYPE_INT)) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &v1, ORCUS_TYPE_INT, 1);
			orcus_release_var(&v1);
			return;
		}

		M_RANDOM_STATE = ((uint64_t)v1.data.u&0xFFFFFFFF)<<16;
	}


	if (args_out) {
		orcus_variable o;
		o.type = ORCUS_TYPE_INT;
		o.data.u = (M_RANDOM_STATE & 0x0000FFFFFFFF0000) >> 16;
		orcus_pushstack(M, &o);
	}
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);

}


void orcus_lib_math_isnan(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable out;
	
	out.data.i = 0;
	
	out.type = ORCUS_TYPE_BOOL;

	orcus_variable val;
	orcus_popstack(M, &val);

	if (val.type == ORCUS_TYPE_FLOAT) {
#ifdef ORCUS_64BIT
		out.data.i = (val.data.i & 0x7ff0000000000000)== 0x7ff0000000000000; //NaN will always have all exponent bits checked. this also will catch infinity, which i suppose technically isn't a number.
#else
		out.data.i = (val.data.i & 0x7f800000)== 0x7f800000;
#endif
	}
	orcus_release_var(&val);

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}



void orcus_lib_math_min_array(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1);
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable in;
	orcus_popstack(M, &in);
	if (in.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_ARRAY, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_variable out;
#ifdef ORCUS_64BIT
	out.data.i = 0x7ff0000000000000;
#else
	out.data.i = 0x7f800000;
#endif
	out.type = ORCUS_TYPE_FLOAT;

	orcus_array* arr = (orcus_array*)in.data.p;

	if (arr->values) {
#ifdef ORCUS_64BIT
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable***** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable**** const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable*** const st3 = st2[i3];
					if (!st3)continue;
					for (int i4 = 0; i4 < ORCUS_SIZE_SUBARRAY_4; i4++) {
						orcus_variable** const st4 = st3[i4];
						if (!st4)continue;
						for (int i5 = 0; i5 < ORCUS_SIZE_SUBARRAY_5; i5++) {
							orcus_variable* const st5 = st4[i5];
							if (!st5)continue;
							for (int i6 = 0; i6 < ORCUS_SIZE_SUBARRAY_6; i6++) {
								orcus_variable const var = st5[i6];
								const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3, i4, i5, i6);
#else
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable* const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable const var = st2[i3];
					const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3);
#endif
					if (var.type==ORCUS_TYPE_INT) {
						if (out.type == ORCUS_TYPE_INT) {
							if (var.data.i < out.data.i)out = var;
						}
						else {
							if (var.data.i < out.data.f)out = var;
						}
					}
					else if (var.type == ORCUS_TYPE_FLOAT) {
						if (out.type == ORCUS_TYPE_INT) {
							if (var.data.f < out.data.i)out = var;
						}
						else {
							if (var.data.f < out.data.f)out = var;
						}
					}

#ifdef ORCUS_64BIT
				}
			}
		}
							}
						}
					}
#else
				}
			}
		}
#endif				
	}


	orcus_release_var(&in);
	
	orcus_pushstack(M,&out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



void orcus_lib_math_max_array(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1);
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable in;
	orcus_popstack(M, &in);
	if (in.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_ARRAY, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_variable out;
#ifdef ORCUS_64BIT
	out.data.i = 0xfff0000000000000;
#else
	out.data.i = 0xff800000;
#endif
	out.type = ORCUS_TYPE_FLOAT;

	orcus_array* arr = (orcus_array*)in.data.p;

	if (arr->values) {
#ifdef ORCUS_64BIT
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable***** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable**** const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable*** const st3 = st2[i3];
					if (!st3)continue;
					for (int i4 = 0; i4 < ORCUS_SIZE_SUBARRAY_4; i4++) {
						orcus_variable** const st4 = st3[i4];
						if (!st4)continue;
						for (int i5 = 0; i5 < ORCUS_SIZE_SUBARRAY_5; i5++) {
							orcus_variable* const st5 = st4[i5];
							if (!st5)continue;
							for (int i6 = 0; i6 < ORCUS_SIZE_SUBARRAY_6; i6++) {
								orcus_variable const var = st5[i6];
								const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3, i4, i5, i6);
#else
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable* const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable const var = st2[i3];
					const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3);
#endif
					if (var.type == ORCUS_TYPE_INT) {
						if (out.type == ORCUS_TYPE_INT) {
							if (var.data.i > out.data.i)out = var;
						}
						else {
							if (var.data.i > out.data.f)out = var;
						}
					}
					else if (var.type == ORCUS_TYPE_FLOAT) {
						if (out.type == ORCUS_TYPE_INT) {
							if (var.data.f > out.data.i)out = var;
						}
						else {
							if (var.data.f > out.data.f)out = var;
						}
					}

#ifdef ORCUS_64BIT
				}
			}
		}
							}
						}
					}
#else
				}
			}
		}
#endif				
	}


	orcus_release_var(&in);

	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



void orcus_lib_math_mean(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (!args_out) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M,args_in, 0);
		return;
	}
	orcus_variable out;

	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	//we can do a backwards stack loop with no issue because the order doesn't matter here
	for (orcus_int a = 0; a < args_in; a++) {
		orcus_variable var;
		orcus_popstack(M, &var);

		//we just ignore non-number variables.
		if (var.type == ORCUS_TYPE_FLOAT) {
			out.data.f += var.data.f;
		}
		else if (var.type == ORCUS_TYPE_INT) {
			out.data.f += (orcus_float)var.data.i;
		}

		orcus_release_var(&var);
	}

	out.data.f /= args_in;

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



void orcus_lib_math_mean_array(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1);
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable in;
	orcus_popstack(M, &in);
	if (in.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_ARRAY, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_variable out;
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_uint elements_traversed = 0;

	orcus_array* arr = (orcus_array*)in.data.p;

	if (arr->values) {
#ifdef ORCUS_64BIT
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable***** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable**** const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable*** const st3 = st2[i3];
					if (!st3)continue;
					for (int i4 = 0; i4 < ORCUS_SIZE_SUBARRAY_4; i4++) {
						orcus_variable** const st4 = st3[i4];
						if (!st4)continue;
						for (int i5 = 0; i5 < ORCUS_SIZE_SUBARRAY_5; i5++) {
							orcus_variable* const st5 = st4[i5];
							if (!st5)continue;
							for (int i6 = 0; i6 < ORCUS_SIZE_SUBARRAY_6; i6++) {
								orcus_variable const var = st5[i6];
								const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3, i4, i5, i6);
#else
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable* const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable const var = st2[i3];
					const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3);
#endif
					if (var.type == ORCUS_TYPE_INT) {
						elements_traversed++;
						out.data.f += (orcus_float)var.data.i;
					}
					else if (var.type == ORCUS_TYPE_FLOAT) {
						elements_traversed++;
						out.data.f += var.data.f;
					}

#ifdef ORCUS_64BIT
				}
			}
		}
							}
						}
					}
#else
				}
			}
		}
#endif				
	}

	out.data.f /= (orcus_float)elements_traversed;

	orcus_release_var(&in);

	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



void orcus_lib_math_stddev(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (!args_out) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0);
		return;
	}
	

	orcus_float mean=0.0;
	orcus_int n = 0;
	orcus_variable out;
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;


	for (orcus_int a = 0; a < args_in; a++) {
		orcus_variable* v =M->stack + M->sizeofstack - a - 1;
		if (v->type == ORCUS_TYPE_FLOAT) {
			mean += v->data.f;
			n++;
		}
		else if (v->type == ORCUS_TYPE_INT) {
			mean += (orcus_float)v->data.i;
			n++;
		}
	}
	mean /= (orcus_float)n;

	for (orcus_int a = 0; a < args_in; a++) {
		orcus_variable* v = M->stack + M->sizeofstack - a - 1;
		if (v->type == ORCUS_TYPE_FLOAT) {
			orcus_float f = mean - v->data.f;
			out.data.f += f * f;
		}
		else if (v->type == ORCUS_TYPE_INT) {
			orcus_float f = mean - (orcus_float)v->data.i;
			out.data.f += f * f;
		}
	}
	out.data.f /= (orcus_float)(n-1);
	out.data.f = sqrt(out.data.f);
	if (n <= 1)out.data.f = 0.0;
	for (orcus_int a = 0; a < args_in; a++) {
		orcus_release_var(M->stack + M->sizeofstack - a - 1);
	}
	M->sizeofstack -= args_in;

	orcus_pushstack(M, &out);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_math_stddev_array(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1);
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable in;
	orcus_popstack(M, &in);
	if (in.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_ARRAY, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_float mean = 0.0;
	orcus_int n = 0;
	orcus_variable out;
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_array* arr = (orcus_array*)in.data.p;

	if (arr->values) {
#ifdef ORCUS_64BIT
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable***** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable**** const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable*** const st3 = st2[i3];
					if (!st3)continue;
					for (int i4 = 0; i4 < ORCUS_SIZE_SUBARRAY_4; i4++) {
						orcus_variable** const st4 = st3[i4];
						if (!st4)continue;
						for (int i5 = 0; i5 < ORCUS_SIZE_SUBARRAY_5; i5++) {
							orcus_variable* const st5 = st4[i5];
							if (!st5)continue;
							for (int i6 = 0; i6 < ORCUS_SIZE_SUBARRAY_6; i6++) {
								orcus_variable const var = st5[i6];
								const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3, i4, i5, i6);
#else
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable* const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable const var = st2[i3];
					const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3);
#endif
					if (var.type == ORCUS_TYPE_INT) {
						n++;
						mean += (orcus_float)var.data.i;
					}
					else if (var.type == ORCUS_TYPE_FLOAT) {
						n++;
						mean += var.data.f;
					}

#ifdef ORCUS_64BIT
				}
			}
		}
							}
						}
					}
#else
				}
			}
		}
#endif				
	}

	mean /= (orcus_float)n;

	if (arr->values) {
#ifdef ORCUS_64BIT
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable***** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable**** const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable*** const st3 = st2[i3];
					if (!st3)continue;
					for (int i4 = 0; i4 < ORCUS_SIZE_SUBARRAY_4; i4++) {
						orcus_variable** const st4 = st3[i4];
						if (!st4)continue;
						for (int i5 = 0; i5 < ORCUS_SIZE_SUBARRAY_5; i5++) {
							orcus_variable* const st5 = st4[i5];
							if (!st5)continue;
							for (int i6 = 0; i6 < ORCUS_SIZE_SUBARRAY_6; i6++) {
								orcus_variable const var = st5[i6];
								const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3, i4, i5, i6);
#else
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable* const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable const var = st2[i3];
					const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3);
#endif
					if (var.type == ORCUS_TYPE_INT) {
						orcus_float f = mean - (orcus_float)var.data.i;
						out.data.f += f * f;
					}
					else if (var.type == ORCUS_TYPE_FLOAT) {
						orcus_float f = mean - var.data.f;
						out.data.f += f * f;
					}

#ifdef ORCUS_64BIT
				}
			}
		}
							}
						}
					}
#else
				}
			}
		}
#endif				
	}

	out.data.f /= (orcus_float)(n-1);
	out.data.f = sqrt(out.data.f);

	if (n <= 1)out.data.f = 0.0;

	orcus_release_var(&in);

	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

//might actually be greater to lesser. doesn't matter, actually. don't care.
int sort_bitwidth_floats_lesser_to_greater(const void* a, const void* b) {
	if (*(orcus_float*)a < *(orcus_float*)b) return -1;
	if (*(orcus_float*)a > *(orcus_float*)b) return 1;
	return 0;
}



void orcus_lib_math_median(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (!args_out) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0);
		return;
	}

	if (!args_in) {
		orcus_variable val;
		val.type = ORCUS_TYPE_FLOAT;
		val.data.f = 0.0;
		val.data.f /= 0.0;
		orcus_pushstack(M, &val);
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
		return;
	}

	orcus_float* data = (orcus_float*)malloc(sizeof(orcus_float) * args_in);
	orcus_int elements = 0;
	if (!data) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	orcus_variable v;
	for (orcus_int n = 0; n < args_in; n++) {
		orcus_popstack(M, &v);
		if (v.type == ORCUS_TYPE_FLOAT) {
			data[elements] = v.data.f;
			elements++;
		}
		else if (v.type == ORCUS_TYPE_INT) {
			data[elements] = (orcus_float)v.data.i;
			elements++;
		}
	}

	v.type = ORCUS_TYPE_FLOAT;
	if (elements < 2) {
		v.data.f = data[0];
	}
	else {
		qsort(data, elements, sizeof(orcus_float), sort_bitwidth_floats_lesser_to_greater);
		if (elements % 2) {
			v.data.f = data[elements / 2];
		}
		else {
			v.data.f = (data[(elements / 2 - 1)] + data[elements / 2]) / 2;
		}
	}
	free(data);
	orcus_pushstack(M, &v);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



void orcus_lib_math_median_array(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1);
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable in;
	orcus_popstack(M, &in);
	if (in.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_ARRAY, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_variable out;
	out.data.f = 0.0;
	out.type = ORCUS_TYPE_FLOAT;

	orcus_int elements = 0;

	orcus_array* arr = (orcus_array*)in.data.p;

	if (arr->values) {
#ifdef ORCUS_64BIT
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable***** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable**** const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable*** const st3 = st2[i3];
					if (!st3)continue;
					for (int i4 = 0; i4 < ORCUS_SIZE_SUBARRAY_4; i4++) {
						orcus_variable** const st4 = st3[i4];
						if (!st4)continue;
						for (int i5 = 0; i5 < ORCUS_SIZE_SUBARRAY_5; i5++) {
							orcus_variable* const st5 = st4[i5];
							if (!st5)continue;
							for (int i6 = 0; i6 < ORCUS_SIZE_SUBARRAY_6; i6++) {
								orcus_variable const var = st5[i6];
								const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3, i4, i5, i6);
#else
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable* const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable const var = st2[i3];
					const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3);
#endif
					if (var.type == ORCUS_TYPE_INT) {
						elements++;
					}
					else if (var.type == ORCUS_TYPE_FLOAT) {
						elements++;
					}

#ifdef ORCUS_64BIT
				}
			}
		}
							}
						}
					}
#else
				}
			}
		}
#endif				
	}

	if (!elements) {
		orcus_variable val;
		val.type = ORCUS_TYPE_FLOAT;
		val.data.f = 0.0;
		val.data.f /= 0.0;
		orcus_pushstack(M, &val);
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_float* data = (orcus_float*)malloc(sizeof(orcus_float) * elements);
	if (!data) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}

	orcus_int n = 0;

	if (arr->values) {
#ifdef ORCUS_64BIT
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable***** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable**** const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable*** const st3 = st2[i3];
					if (!st3)continue;
					for (int i4 = 0; i4 < ORCUS_SIZE_SUBARRAY_4; i4++) {
						orcus_variable** const st4 = st3[i4];
						if (!st4)continue;
						for (int i5 = 0; i5 < ORCUS_SIZE_SUBARRAY_5; i5++) {
							orcus_variable* const st5 = st4[i5];
							if (!st5)continue;
							for (int i6 = 0; i6 < ORCUS_SIZE_SUBARRAY_6; i6++) {
								orcus_variable const var = st5[i6];
								const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3, i4, i5, i6);
#else
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable* const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable const var = st2[i3];
					const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3);
#endif
					if (var.type == ORCUS_TYPE_INT) {
						data[n] = (orcus_float)var.data.i;
						n++;
					}
					else if (var.type == ORCUS_TYPE_FLOAT) {
						data[n] = var.data.f;
						n++;
					}

#ifdef ORCUS_64BIT
				}
			}
		}
							}
						}
					}
#else
				}
			}
		}
#endif				
	}


	orcus_release_var(&in);

	orcus_variable v;
	v.type = ORCUS_TYPE_FLOAT;
	if (elements < 2) {
		v.data.f = data[0];
	}
	else {
		qsort(data, elements, sizeof(orcus_float), sort_bitwidth_floats_lesser_to_greater);
		if (elements % 2) {
			v.data.f = data[elements / 2];
		}
		else {
			v.data.f = (data[(elements / 2 - 1)] + data[elements / 2]) / 2;
		}
	}
	free(data);
	orcus_pushstack(M, &v);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}