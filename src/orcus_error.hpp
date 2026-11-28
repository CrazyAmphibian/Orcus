#pragma once
#include <stdint.h>
#include <malloc.h>
#include "orcus.hpp"
#include "orcus_compiler.hpp"


enum ORCUS_ERROR_TYPES:uint32_t {
	ORCUS_ERROR_NONE = 0,			// args: 
	ORCUS_ERROR_ALLOCATION = 1,		//args:
	ORCUS_ERROR_WRONG_TYPE = 2,		//args: provided, expected, arg number
	ORCUS_ERROR_DIV_ZERO = 3,		//args:
	ORCUS_ERROR_INVALID_INDEX = 4, // args: expected, provided
	ORCUS_ERROR_INSTRUCTION_FAILIURE = 5, //args:
	ORCUS_ERROR_CALL_NOT_FUNCTION = 6 ,// args: provided
	ORCUS_ERROR_INDEX_INVALID_TYPE = 7, // args: provided
	ORCUS_ERROR_NOT_ENOUGH_ARGS = 8, //args: provided, expected
	ORCUS_ERROR_CUSTOM_STRING = 9, //args: c string
	ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER, //args: provided, arg number. for functions that can take ints or floats
	ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_FUNCTION, //args: provided, arg number. for functions that can take C and M functions
	ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_STORAGETYPE,//args: provided, arg number. for functions that can take arrays or tables.
	ORCUS_ERROR_EXECUTION_ABORT,
	ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, //args: variable pointer, expected type, arg number
	ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, //args: variable pointer, arg number. for functions that can take ints or floats
	ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_FUNCTION_VARIABLEPROVIDED, //args: variable pointer, arg number. for functions that can take C and M functions
	ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_STORAGETYPE_VARIABLEPROVIDED,//args: variable pointer, arg number. for functions that can take arrays or tables.
	ORCUS_ERROR_STACK_CORRUPT,
};

orcus_string* orcus_generate_error_string(orcus_state* M, const uint32_t errorcode, const orcus_int* data1 = nullptr, const orcus_int* data2 = nullptr, const orcus_int* data3 = nullptr);
void orcus_raise_error(orcus_state* M, const uint32_t errorcode, const orcus_int* data1 = nullptr, const orcus_int* data2 = nullptr, const orcus_int* data3 = nullptr);


inline void orcus_raise_error_nonpointer(orcus_state* M, const uint32_t errorcode, const orcus_int data1 = 0, const orcus_int data2 = 0, const orcus_int data3 = 0) {
	orcus_raise_error(M, errorcode, &data1, &data2, &data3);
}

inline void orcus_raise_error_firstargpointeronly(orcus_state* M, const uint32_t errorcode, void* data1 = 0, const orcus_int data2 = 0, const orcus_int data3 = 0) {
	orcus_raise_error(M, errorcode, (const orcus_int*)data1, &data2, &data3);
}


orcus_string* orcus_get_state_traceback(orcus_state* M);

//helper functions for making c functions. returns true if there's a problem, false if nothing went wrong. if()return;

//variadic functions need not apply. also if you want to take optional args, you should specify max_args as the largest number you want to take. this function ensures that additional args will be discarded, and if args are missing, an error will be thrown.
inline bool ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int min_args, orcus_int max_args=0) {
	if (!max_args) {
		max_args = min_args; //by default, args are specific
	}
	if (args_in < min_args) {
		orcus_raise_error_nonpointer(M, ORCUS_ERROR_NOT_ENOUGH_ARGS, args_in, min_args);
		return true;
	}
	for (orcus_int i = (const orcus_int)max_args; i < args_in; i++) {
		orcus_discard_top_of_stack(M);
	}
	return false;
}

//pretty simple will output any args that haven't been outputted. is nil.
inline bool ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_out, const orcus_int sent_args=0) {
	if (args_out <= 0)return true;
	orcus_int diff = args_out - sent_args;
	if (diff > 0) {
		if (M->allocatedstacksize < M->sizeofstack + diff) {
			orcus_variable* nptr = (orcus_variable*)realloc(M->stack, sizeof(orcus_variable) * (M->sizeofstack + diff));
			if (!nptr) {
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return false;
			}
			M->stack = nptr;
			M->allocatedstacksize = M->sizeofstack + diff;
		}
		memset(M->stack + M->sizeofstack, '\0', diff*sizeof(orcus_variable));
		M->sizeofstack += diff;
		
		return true;
	}
	return true;
}
