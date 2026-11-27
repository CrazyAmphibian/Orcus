#include "orcus_error.hpp"
#include "orcus_compiler.hpp"
#include "orcus.hpp"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>



const char* typetostring[256] = {
	"nil",
	"integer",
	"float",
	"boolean",
	"table",
	"string",
	"c function",
	"array",
	"function",
	"file",
	"thread"
};

inline const char* get_type_string(uint8_t type) {
	if (type >= ORCUS_NUMBER_OF_TYPES) {
		return "unknown";
	}
	return typetostring[type];
}

orcus_string* orcus_get_state_traceback(orcus_state* M) {
	orcus_string* out = (orcus_string*)malloc(sizeof(orcus_string));
	if (!out)return out;
	memset(out, NULL, sizeof(orcus_string));
	const size_t buffer_size = 1000;
	char buffer[buffer_size];
	memset(buffer, '\0', buffer_size);

	while (M) {
		orcus_string* prepend = (orcus_string*)malloc(sizeof(orcus_string));
		if (!prepend) {
			orcus_mstring_delete(out);
			return nullptr;
		}
		memset(prepend, NULL, sizeof(orcus_string));

		if (M->bytecode && M->bytecode->instruction_dbg_lookup) {
			orcus_uint dbg_tok_num = M->bytecode->instruction_dbg_lookup[M->programcounter - 1];
			orcus_debug_token T, T_n, T_p;
			bool next = false;
			bool prev = false;
			if (dbg_tok_num)prev = true;
			if (dbg_tok_num + 1 < M->bytecode->num_dbg_tokens)next = true;
			T = M->bytecode->dbg_tokens[dbg_tok_num];
			if (prev)T_p = M->bytecode->dbg_tokens[dbg_tok_num - 1];
			if (next)T_n = M->bytecode->dbg_tokens[dbg_tok_num + 1];


			char* tchars = (char*)malloc(T.num_chars + 1);
			if (!tchars) {
				orcus_mstring_delete(out);
				orcus_mstring_delete(prepend);
				return nullptr;
			}
			memcpy(tchars, T.chars, T.num_chars);
			tchars[T.num_chars] = '\0';

			char* n_tchars = nullptr;
			if (next) {
				n_tchars = (char*)malloc(T_n.num_chars + 1);
				if (!n_tchars) {
					free(tchars);
					orcus_mstring_delete(out);
					orcus_mstring_delete(prepend);
					return nullptr;
				}
				memcpy(n_tchars, T_n.chars, T_n.num_chars);
				n_tchars[T_n.num_chars] = '\0';
			}

			char* p_tchars = nullptr;
			if (prev) {
				p_tchars = (char*)malloc(T_p.num_chars + 1);
				if (!p_tchars) {
					free(tchars);
					orcus_mstring_delete(out);
					orcus_mstring_delete(prepend);
					return nullptr;
				}
				memcpy(p_tchars, T_p.chars, T_p.num_chars);
				p_tchars[T_p.num_chars] = '\0';
			}

			if (next && prev) {
				snprintf(buffer, buffer_size, "line %zi col %zi at %s%s%s", T.line, T.col, p_tchars, tchars, n_tchars);
				free(p_tchars);
				free(n_tchars);
			}
			else if (next) {
				snprintf(buffer, buffer_size, "line %zi col %zi at %s%s", T.line, T.col, tchars, n_tchars);
				free(n_tchars);
			}
			else if (prev) {
				snprintf(buffer, buffer_size, "line %zi col %zi at %s%s", T.line, T.col, p_tchars, tchars);
				free(p_tchars);
			}
			else {
				snprintf(buffer, buffer_size, "line %zi col %zi at %s", T.line, T.col, tchars);
			}
			free(tchars);
		}
		else {
			snprintf(buffer, buffer_size, "line ? col ? at ? (debug information not avalible)");
		}
		if (M != M->masterstate) {
			orcus_mstring_addchars(prepend, " in:\n", 5);
		}
		orcus_mstring_addchars(prepend, buffer, strlen(buffer));
		orcus_mstrings_append(prepend, out);
		orcus_mstring_delete(out);
		out = prepend;

		M = M->parentstate;
	}
	return out;
}



orcus_string* orcus_generate_error_string(orcus_state* M, const uint32_t errorcode, const orcus_int* data1, const orcus_int* data2, const orcus_int* data3) {
	orcus_string* out = orcus_get_state_traceback(M);
	if (!out)return nullptr;
	const size_t buffer_size = 1000;
	char buffer[buffer_size];
	
	switch (errorcode) {
		case ORCUS_ERROR_ALLOCATION:
			orcus_mstring_addchars(out, ": memory allocation failiure.\n", 29);
			break;
		case ORCUS_ERROR_WRONG_TYPE:
			snprintf(buffer, buffer_size, ": arg %zi wrong type. expected %s, got %s.\n", *data3, get_type_string((uint8_t)*data2), get_type_string((uint8_t)*data1));
			orcus_mstring_addchars(out, buffer, strlen(buffer));
			break;
		case ORCUS_ERROR_DIV_ZERO:
			orcus_mstring_addchars(out, ": integer division by 0.\n", 23);
			break;
		case ORCUS_ERROR_INSTRUCTION_FAILIURE:
			orcus_mstring_addchars(out, ": failiure to execute instruction.\n", 23);
			break;
		case ORCUS_ERROR_CALL_NOT_FUNCTION:
			snprintf(buffer, buffer_size, ": attempt to call non-function value %s.\n", get_type_string((uint8_t)*data1));
			orcus_mstring_addchars(out, buffer, strlen(buffer));
			break;
		case ORCUS_ERROR_INVALID_INDEX:
			snprintf(buffer, buffer_size, ": invalid index type %s.\n", get_type_string((uint8_t)*data1));
			orcus_mstring_addchars(out, buffer, strlen(buffer));
			break;
		case ORCUS_ERROR_INDEX_INVALID_TYPE:
			snprintf(buffer, buffer_size, ": attempt to index invalid variable type %s.\n", get_type_string((uint8_t)*data1));
			orcus_mstring_addchars(out, buffer, strlen(buffer));
			break;
		case ORCUS_ERROR_NOT_ENOUGH_ARGS:
			snprintf(buffer, buffer_size, ": incorrect number of args. expected %zi, got %zi.\n", *data2, *data1);
			orcus_mstring_addchars(out, buffer, strlen(buffer));
			break;
		case ORCUS_ERROR_CUSTOM_STRING:
			orcus_mstring_addchars(out,": ", 2);
			orcus_mstring_addchars(out, (char*)data1, strlen((char*)data1));
			break;
		case ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER:
			snprintf(buffer, buffer_size, ": arg %zi wrong type. expected a number, got %s.\n", *data2, get_type_string((uint8_t)*data1));
			orcus_mstring_addchars(out, buffer, strlen(buffer));
			break;
		case ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_FUNCTION:
			snprintf(buffer, buffer_size, ": arg %zi wrong type. expected any function type, got %s.\n", *data2, get_type_string((uint8_t)*data1));
			orcus_mstring_addchars(out, buffer, strlen(buffer));
			break;
		case ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_STORAGETYPE:
			snprintf(buffer, buffer_size, ": arg %zi wrong type. expected an array or table, got %s\n", *data2, get_type_string((uint8_t)*data1));
			orcus_mstring_addchars(out, buffer, strlen(buffer));
			break;
		case ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED:
			{
			orcus_string* str=orcus_tostring((orcus_variable*)data1);
			snprintf(buffer, buffer_size, ": arg %zi wrong type. expected %s, got %s value ", *data3, get_type_string((uint8_t)*data2), get_type_string( ((orcus_variable*)data1)->type));
			orcus_mstring_addchars(out, buffer, strlen(buffer));
			orcus_mstrings_append(out, str);
			orcus_mstring_delete(str);
			orcus_mstring_addchars(out, ".\n", 2);
			}
			break;
		case ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED:
			{
			orcus_string* str = orcus_tostring((orcus_variable*)data1);
			snprintf(buffer, buffer_size, ": arg %zi wrong type. expected a number, got %s value ", *data2, get_type_string(((orcus_variable*)data1)->type));
			orcus_mstring_addchars(out, buffer, strlen(buffer));
			orcus_mstrings_append(out, str);
			orcus_mstring_delete(str);
			orcus_mstring_addchars(out, ".\n", 2);
			}
			break;
		case ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_FUNCTION_VARIABLEPROVIDED:
			{
			orcus_string* str = orcus_tostring((orcus_variable*)data1);
			snprintf(buffer, buffer_size, ": arg %zi wrong type. expected any function type, got %s value ", *data2, get_type_string(((orcus_variable*)data1)->type));
			orcus_mstring_addchars(out, buffer, strlen(buffer));
			orcus_mstrings_append(out, str);
			orcus_mstring_delete(str);
			orcus_mstring_addchars(out, ".\n", 2);
			}
			break;
		case ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_STORAGETYPE_VARIABLEPROVIDED:
			{
			orcus_string* str = orcus_tostring((orcus_variable*)data1);
			snprintf(buffer, buffer_size, ": arg %zi wrong type. expected an array or table, got %s value ", *data2, get_type_string(((orcus_variable*)data1)->type));
			orcus_mstring_addchars(out, buffer, strlen(buffer));
			orcus_mstrings_append(out, str);
			orcus_mstring_delete(str);
			orcus_mstring_addchars(out, ".\n", 2);
			}
			break;
		case ORCUS_ERROR_STACK_CORRUPT:
			orcus_mstring_addchars(out, ": stack has been corrupted.\n", 28);
			break;
		default:
			orcus_mstring_addchars(out, ": unknown error.",16);
			break;
	}

	/*ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, //args: variable pointer, expected type, arg number
	ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, //args: variable pointer, arg number. for functions that can take ints or floats
	ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_FUNCTION_VARIABLEPROVIDED, //args: variable pointer, arg number. for functions that can take C and M functions
	ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_STORAGETYPE_VARIABLEPROVIDED*/
	return out;
}

void orcus_raise_error(orcus_state* M, const uint32_t errorcode, const orcus_int* data1, const orcus_int* data2, const orcus_int* data3) {

	orcus_string* str = orcus_generate_error_string(M,errorcode, data1, data2, data3);


	if (str) {
		for (orcus_int i = 0; i < str->size; i++) {
			//printf("%c",str->ptr[i]);
			putchar(str->ptr[i]);
		}
		putchar('\n');
		orcus_mstring_delete(str);
	}

	while (M) {
		M->errorcode = errorcode;
		if(M->bytecode)M->programcounter = M->bytecode->numberofinstructions; //push to end to stop execution
		M = M->parentstate;
	}
}