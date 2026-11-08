#include "orcus_lib_string.hpp"
#include "../orcus.hpp"
#include "../orcus_error.hpp"

#include <malloc.h>
#include <string.h>


void orcus_lib_string_sub(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 3))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable var_end;
	orcus_popstack(M, &var_end);
	if (var_end.type != ORCUS_TYPE_INT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_end, ORCUS_TYPE_INT, 3);
		orcus_release_var(&var_end);
		return;
	}
	orcus_variable var_start;
	orcus_popstack(M, &var_start);
	if (var_start.type != ORCUS_TYPE_INT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_start, ORCUS_TYPE_INT, 2);
		orcus_release_var(&var_start);
		return;
	}

	orcus_variable var_string;
	orcus_popstack(M,&var_string);
	if (var_string.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_string, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&var_string);
		return;
	}

	orcus_string* os=orcus_mstring_substring((orcus_string*)var_string.data.p, var_start.data.i, var_end.data.i);
	orcus_variable out;
	
	out.type = ORCUS_TYPE_STRING;
	out.data.p = os;
	orcus_release_var(&var_string);
	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}




void orcus_lib_string_reverse(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable var_string;
	orcus_popstack(M,&var_string);
	if (var_string.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_string, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&var_string);
		return;
	}
	orcus_string* str = (orcus_string*)var_string.data.p;

	orcus_string* os = (orcus_string*)malloc(sizeof(orcus_string));
	if (!os) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	os->ptr = (char*)malloc(sizeof(char)*str->size);
	if (!os->ptr) {
		free(os);
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}

	for (orcus_int c = 0; c < str->size; c++) {
		os->ptr[str->size - c - 1] = str->ptr[c];
	}
	os->size = str->size;

	orcus_variable out;
	
	out.type = ORCUS_TYPE_STRING;
	out.data.p = os;
	orcus_release_var(&var_string);
	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



void orcus_lib_string_find(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2,3))return;
	if (!args_out) {
		if(args_in>2)orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable startat;
	if (args_in > 2) {
		orcus_popstack(M,&startat);
		if (startat.type != ORCUS_TYPE_INT) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &startat, ORCUS_TYPE_INT, 3);
			orcus_release_var(&startat);
			return;
		}
	}
	else {
		startat.type = ORCUS_TYPE_NIL;
	}

	orcus_variable searchforvar;
	orcus_popstack(M,&searchforvar);
	if (searchforvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &searchforvar, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&searchforvar);
		return;
	}

	orcus_variable strvar;
	orcus_popstack(M,&strvar);
	if (strvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &strvar, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&strvar);
		return;
	}

	orcus_string* str = (orcus_string*)strvar.data.p;
	orcus_string* search = (orcus_string*)searchforvar.data.p;

	orcus_int located_at = -1;

	for (orcus_int c = (startat.type ? startat.data.i : 0); c < str->size-search->size+1; c++) {
		for (orcus_int i = 0; i < search->size; i++) {
			if (str->ptr[c + i] != search->ptr[i]) {
				goto next;
			}
		}
		located_at = c;
		break;
		next:
		{}
	}

	orcus_variable out;
	

	if (located_at != -1) {
		out.type = ORCUS_TYPE_INT;
		out.data.i = located_at;
	}
	else {
		out.type = ORCUS_TYPE_NIL;
		out.data.i = 0;
	}

	orcus_pushstack(M, &out);
	
	//second one is end char.
	if (args_out > 1) {
		orcus_variable out2;
		

		if (located_at != -1) {
			out2.type = ORCUS_TYPE_INT;
			out2.data.i = located_at+search->size-1;
		}
		else {
			out2.type = ORCUS_TYPE_NIL;
			out2.data.i = 0;
		}
		orcus_pushstack(M, &out2);
	}

	orcus_release_var(&searchforvar);
	orcus_release_var(&strvar);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 2);
}



void orcus_lib_string_replace(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 3, 4))return;
	if (!args_out) {
		if(args_in>3)orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}


	orcus_variable max_replacments;
	if (args_in > 3) {
		orcus_popstack(M,&max_replacments);
		if (max_replacments.type != ORCUS_TYPE_INT) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &max_replacments, ORCUS_TYPE_INT, 4);
			orcus_release_var(&max_replacments);
			return;
		}
	}
	else {
		max_replacments.type = ORCUS_TYPE_NIL;
	}

	orcus_variable replace_var;
	orcus_popstack(M,& replace_var);
	if (replace_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &replace_var, ORCUS_TYPE_STRING, 3);
		orcus_release_var(&replace_var);
		return;
	}

	orcus_variable search_var;
	orcus_popstack(M, &search_var);
	if (search_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &search_var, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&search_var);
		return;
	}

	orcus_variable str_var;
	orcus_popstack(M, &str_var);
	if (str_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &str_var, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&str_var);
		return;
	}

	orcus_string* outstr = (orcus_string*)malloc(sizeof(orcus_string));
	if (!outstr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	outstr->size = 0;
	outstr->references = 0;
	outstr->ptr = nullptr;
	
	orcus_string* str = (orcus_string*)str_var.data.p;
	orcus_string* search = (orcus_string*)search_var.data.p;
	orcus_string* replace = (orcus_string*)replace_var.data.p;

	orcus_int replacments = 0;
	orcus_int substr_start = 0;

	bool exitloop = str->size == 0;
	for (orcus_int c = 0; c <= str->size;) {
		if (exitloop)break;
		for (orcus_int i = 0; i < search->size; i++) {
			if ((c + i >= str->size) || (max_replacments.type && (replacments>=max_replacments.data.i)) ) {
				if (!orcus_mstring_addchars(outstr, str->ptr + substr_start, str->size - substr_start)) {
					orcus_mstring_delete(outstr);
					orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
					return;
				}
				exitloop = true;
				goto nomatch;
			}
			else if (str->ptr[c + i] != search->ptr[i]) {
				goto nomatch;
			}
		}
		//matched
		if (!search->size) {
			exitloop = true;
			c = str->size;
		}
	match:
		{
			if (!orcus_mstring_addchars(outstr, str->ptr + substr_start, c - substr_start)) {
				orcus_mstring_delete(outstr);
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			if (!orcus_mstrings_append(outstr,replace)) {
				orcus_mstring_delete(outstr);
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			replacments++;
			c += search->size;
			substr_start = c;
		}
		continue;
	nomatch:
		c++;
	}

	orcus_release_var(&str_var);
	orcus_release_var(&search_var);
	orcus_release_var(&replace_var);

	orcus_variable out;
	
	out.type = ORCUS_TYPE_STRING;
	out.data.p = outstr;

	orcus_pushstack(M,&out);

	if (args_out > 1) {
		orcus_variable out2;
		
		out2.type = ORCUS_TYPE_INT;
		out2.data.i = replacments;
		orcus_pushstack(M, &out2);
	}

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 2);
}

void orcus_lib_string_count(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable search_var;
	orcus_popstack(M, &search_var);
	if (search_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &search_var, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&search_var);
		return;
	}

	orcus_variable str_var;
	orcus_popstack(M, &str_var);
	if (str_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &str_var, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&str_var);
		return;
	}

	orcus_string* str = (orcus_string*)str_var.data.p;
	orcus_string* search = (orcus_string*)search_var.data.p;

	orcus_int replacments = 0;

	for (orcus_int c = 0; c < str->size;) {
		if (!search->size)break;
		if (c > str->size - search->size)goto next;
		for (orcus_int i = 0; i < search->size; i++) {
			if (str->ptr[c + i] != search->ptr[i]) {
				goto next;
			}
		}
		//string found
		replacments++;
		c += search->size;
		continue;
	next:
		c++;
	}

	orcus_release_var(&str_var);
	orcus_release_var(&search_var);

	
	search_var.type = ORCUS_TYPE_INT;
	search_var.data.i = replacments;

	orcus_pushstack(M, &search_var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

void orcus_lib_string_toarray(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //converts the bytes to an array of ints.
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable str_var;
	orcus_popstack(M, &str_var);
	if (str_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &str_var, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&str_var);
		return;
	}
	orcus_string* str = (orcus_string*)str_var.data.p;

	orcus_array* arr = orcus_newarray();
	if (!arr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}

	for (orcus_int i = 0; i < str->size; i++) {
		orcus_variable* v=(orcus_variable*)malloc(sizeof(orcus_variable));
		if (!v)continue;
		v->type = ORCUS_TYPE_INT;
		v->data.i = (unsigned char)str->ptr[i];
		orcus_setarray(arr, v, i);
	}

	orcus_release_var(&str_var); //re use
	
	str_var.type = ORCUS_TYPE_ARRAY;
	str_var.data.p = arr;
	orcus_pushstack(M, &str_var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

void orcus_lib_string_fromarray(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //converts an array of ints into a string
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable str_var;
	orcus_popstack(M,&str_var);
	if (str_var.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &str_var, ORCUS_TYPE_ARRAY, 1);
		orcus_release_var(&str_var);
		return;
	}
	orcus_array* arr = (orcus_array*)str_var.data.p;

	orcus_int len=orcus_array_len(arr)+1;

	orcus_string* st = (orcus_string*)malloc(sizeof(orcus_string));
	if (!st) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	memset(st, 0, sizeof(orcus_string));
	st->ptr=(char*)malloc(len * sizeof(char));
	if (!st->ptr) {
		free(st);
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	st->size = len;

	for (orcus_int i = 0; i < len; i++) {
		orcus_variable v;
		orcus_getarray(arr, i,&v);
		if (v.type == ORCUS_TYPE_INT) {
			st->ptr[i] = (unsigned char)(v.data.i & 0xFF);
		}
		else {
			st->ptr[i] = '\0';
		}
	}

	orcus_release_var(&str_var);
	
	str_var.type = ORCUS_TYPE_STRING;
	str_var.data.p = st;
	orcus_pushstack(M, &str_var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_string_separate(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //seperates a string into an array of smaller strings.
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}


	orcus_variable sep_var;
	orcus_popstack(M, &sep_var);
	if (sep_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &sep_var, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&sep_var);
		return;
	}

	orcus_variable str_var;
	orcus_popstack(M, &str_var);
	if (str_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &str_var, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&str_var);
		return;
	}


	orcus_array* arr = orcus_newarray();
	if (!arr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}

	orcus_string* str = (orcus_string*)str_var.data.p;
	orcus_string* sep = (orcus_string*)sep_var.data.p;

	orcus_int arr_elems = 0;
	orcus_int substr_start = 0;

	bool exitloop = str->size==0;
	for (orcus_int c = 0; c <= str->size;) {
		if (exitloop)break;
		for (orcus_int i = 0; i < sep->size; i++) {
			if (c + i >= str->size) {
				exitloop = true;
				goto match;
			}
			else if (str->ptr[c + i] != sep->ptr[i]) {
				goto nomatch;
			}
		}
		//matched
		if (!sep->size) {
			exitloop = true;
			c = str->size;
		}
		match:
		{
			orcus_string* addstr = orcus_cstring_to_mstring(str->ptr+ substr_start, c - substr_start);
			if (!addstr) {
				orcus_destroyarray(arr);
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			orcus_variable v;
			v.data.p = addstr;
			v.type = ORCUS_TYPE_STRING;
			orcus_setarray(arr, &v, arr_elems);
			arr_elems++;
			c+= sep->size;
			substr_start = c;
		}
		continue;
		nomatch:
		c++;
	}


	orcus_release_var(&sep_var);
	orcus_release_var(&str_var);
	
	
	sep_var.type = ORCUS_TYPE_ARRAY;
	sep_var.data.p = arr;
	orcus_pushstack(M, &sep_var);


	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_string_upper(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable var_string;
	orcus_popstack(M,&var_string);
	if (var_string.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_string, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&var_string);
		return;
	}
	orcus_string* str = (orcus_string*)var_string.data.p;

	orcus_string* os = (orcus_string*)malloc(sizeof(orcus_string));
	if (!os) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	os->ptr = (char*)malloc(sizeof(char) * str->size);
	if (!os->ptr) {
		free(os);
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}

	for (orcus_int c = 0; c < str->size; c++) {
		char cc = str->ptr[c];
		if (cc >= 'a' && cc <= 'z') {
			os->ptr[c] = cc-0x20;
		}
		else {
			os->ptr[c] = cc;
		}
		
	}
	os->size = str->size;
	os->constant = false;
	os->references = 0;

	orcus_release_var(&var_string);
	
	var_string.type = ORCUS_TYPE_STRING;
	var_string.data.p = os;

	orcus_pushstack(M, &var_string);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_string_lower(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable var_string;
	orcus_popstack(M, &var_string);
	if (var_string.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_string, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&var_string);
		return;
	}
	orcus_string* str = (orcus_string*)var_string.data.p;

	orcus_string* os = (orcus_string*)malloc(sizeof(orcus_string));
	if (!os) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	os->ptr = (char*)malloc(sizeof(char) * str->size);
	if (!os->ptr) {
		free(os);
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}

	for (orcus_int c = 0; c < str->size; c++) {
		char cc = str->ptr[c];
		if (cc >= 'A' && cc <= 'Z') {
			os->ptr[c] = cc + 0x20;
		}
		else {
			os->ptr[c] = cc;
		}

	}
	os->size = str->size;
	os->constant = false;
	os->references = 0;

	orcus_release_var(&var_string);
	
	var_string.type = ORCUS_TYPE_STRING;
	var_string.data.p = os;

	orcus_pushstack(M, &var_string);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

enum stringformat_argpositons {
	ARG_WIDTH = 1,
	ARG_PERCISION = 2,
	ARG_PADDINGISZERO = 3,
	ARG_LEFTALIGN = 4,
	ARG_FORCEPLUS = 5,
	ARG_ALTERNATENUM = 6
};

char* getprintfstring(orcus_int* args, bool* args_def, const char* type) {
	char* str=(char*)malloc(sizeof(char) * 256);
	if (!str)return nullptr;
	short ptr = 1;
	str[0] = '%';

	if (args_def[ARG_FORCEPLUS]) {
		switch (args[ARG_FORCEPLUS]) {
		case 1:
			str[ptr] = '+';
			break;
		case 2:
			str[ptr] = ' ';
			break;
		}
		ptr++;
	}
	if (args_def[ARG_LEFTALIGN]) {
		str[ptr] = '-';
		ptr++;
	}
	if (args_def[ARG_ALTERNATENUM]) {
		str[ptr] = '#';
		ptr++;
	}
	if (args_def[ARG_PADDINGISZERO]) {
		str[ptr] = '0';
		ptr++;
	}

	if (args_def[ARG_WIDTH]) {
		str[ptr] = '*';
		ptr++;
	}

	if (args_def[ARG_PERCISION]) {
		str[ptr] = '.';
		str[ptr+1] = '*';
		ptr+=2;
	}

	memcpy(str+ptr,type,strlen(type));
	ptr+=(short)strlen(type);

	str[ptr] = '\0';
	return str;
}

void printf_wrapper_takes_float(char* buffer, size_t buffer_size,orcus_string* str_out,double value,const char* type, orcus_int* args, bool* args_def) {
	orcus_int l = 0;
	orcus_int p = 0;
	if (args_def[ARG_WIDTH])l = args[ARG_WIDTH];
	if (args_def[ARG_PERCISION])p = args[ARG_PERCISION];

	char* printf_fstr = getprintfstring(args, args_def, type);
	if (!printf_fstr)return;

	int chars_added = 0;
	if (args_def[ARG_WIDTH]) {
		if (args_def[ARG_PERCISION]) {
			chars_added = snprintf(buffer, buffer_size, printf_fstr, l, p, value);
		}
		else {
			chars_added = snprintf(buffer, buffer_size, printf_fstr, l, value);
		}
	}
	else {
		if (args_def[ARG_PERCISION]) {
			chars_added = snprintf(buffer, buffer_size, printf_fstr, p, value);
		}
		else {
			chars_added = snprintf(buffer, buffer_size, printf_fstr, value);
		}
	}
	free(printf_fstr);
	orcus_mstring_addchars(str_out, buffer, chars_added);
}

void printf_wrapper_takes_int(char* buffer, size_t buffer_size, orcus_string* str_out, orcus_int value, const char* type, orcus_int* args, bool* args_def) {
	orcus_int l = 0;
	orcus_int p = 0;
	if (args_def[ARG_WIDTH])l = args[ARG_WIDTH];
	if (args_def[ARG_PERCISION])p = args[ARG_PERCISION];

	char* printf_fstr = getprintfstring(args, args_def, type);
	if (!printf_fstr)return;

	int chars_added = 0;
	if (args_def[ARG_WIDTH]) {
		if (args_def[ARG_PERCISION]) {
			chars_added=snprintf(buffer, buffer_size, printf_fstr, l, p, value);
		}
		else {
			chars_added = snprintf(buffer, buffer_size, printf_fstr, l, value);
		}
	}
	else {
		if (args_def[ARG_PERCISION]) {
			chars_added = snprintf(buffer, buffer_size, printf_fstr, p, value);
		}
		else {
			chars_added = snprintf(buffer, buffer_size, printf_fstr, value);
		}
	}
	free(printf_fstr);
	orcus_mstring_addchars(str_out, buffer, chars_added);
}

union orcus_rawdata_snowflake_printf { //because printf promotes floats to doubles for some fucking reason. why?
	double f;
	orcus_int i;
	orcus_uint u;
	void* p;
};


orcus_int orcus_readformat(orcus_string* str, orcus_int offset, orcus_string* str_out, orcus_variable* v_arr, orcus_int* num_vars) {
	orcus_int add_off = 0;

	unsigned char nargs = 0;
	bool args_def[256] = { false }; //if the arg is defined.
	orcus_int args[256] = {0}; // the value of that arg.
	char mode = 0;

	const size_t BUFFER_SIZE = 1024;
	char printfbuffer[BUFFER_SIZE] = {'\0'};

	const char* type=nullptr;
	orcus_rawdata_snowflake_printf value;
	value.i = 0;

	while (true) {
		if (add_off + offset >= str->size)break;
		char c = str->ptr[add_off + offset];
		add_off++;
		switch (c) {
		case '%':
			orcus_mstring_addchars(str_out,(char*)"%",1);
			return add_off;
		case 'I':
		case 'i': // integer
			if (*num_vars) {
				(*num_vars)--;
				value.i = orcus_checkint(v_arr + *num_vars);
			}
			printf_wrapper_takes_int(printfbuffer, BUFFER_SIZE, str_out, value.i, "zi", args, args_def);
			return add_off;
		case 'U':
		case 'u': // unsigned integer
			if (*num_vars) {
				(*num_vars)--;
				value.i = orcus_checkint(v_arr + *num_vars);
			}
			printf_wrapper_takes_int(printfbuffer, BUFFER_SIZE, str_out, value.i, "zu", args, args_def);
			return add_off;
		case 'O':
		case 'o': // unsigned octal
			if (*num_vars) {
				(*num_vars)--;
				value.i = orcus_checkint(v_arr + *num_vars);
			}
			printf_wrapper_takes_int(printfbuffer, BUFFER_SIZE, str_out, value.i, "zo", args, args_def);
			return add_off;
		case 'x': // hex int
			if (*num_vars) {
				(*num_vars)--;
				value.i = orcus_checkint(v_arr + *num_vars);
			}
			printf_wrapper_takes_int(printfbuffer, BUFFER_SIZE, str_out, value.i, "zx", args, args_def);
			return add_off;
		case 'X': // hex int (capital)
			if (*num_vars) {
				(*num_vars)--;
				value.i = orcus_checkint(v_arr + *num_vars);
			}
			printf_wrapper_takes_int(printfbuffer, BUFFER_SIZE, str_out, value.i, "zX", args, args_def);
			return add_off;
		case 'c':
		case 'C':
			if (*num_vars) {
				(*num_vars)--;
				value.i = orcus_checkint(v_arr + *num_vars);
			}
			printf_wrapper_takes_int(printfbuffer, BUFFER_SIZE, str_out, value.i, "c", args, args_def);
			return add_off;
return add_off;
		case 'f':
			if (*num_vars) {
				(*num_vars)--;
				value.f = orcus_checkfloat(v_arr + *num_vars);
			}
			printf_wrapper_takes_float(printfbuffer, BUFFER_SIZE, str_out, value.f, "f", args, args_def);
			return add_off;
		case 'F':
			if (*num_vars) {
				(*num_vars)--;
				value.f = orcus_checkfloat(v_arr + *num_vars);

			}
			printf_wrapper_takes_float(printfbuffer, BUFFER_SIZE, str_out, value.f, "F", args, args_def);
			return add_off;
		case 'e':
			if (*num_vars) {
				(*num_vars)--;
				value.f = orcus_checkfloat(v_arr + *num_vars);

			}
			printf_wrapper_takes_float(printfbuffer, BUFFER_SIZE, str_out, value.f, "e", args, args_def);
			return add_off;
		case 'E':
			if (*num_vars) {
				(*num_vars)--;
				value.f = orcus_checkfloat(v_arr + *num_vars);

			}
			printf_wrapper_takes_float(printfbuffer, BUFFER_SIZE, str_out, value.f, "E", args, args_def);
			return add_off;
		case 'g':
			if (*num_vars) {
				(*num_vars)--;
				value.f = orcus_checkfloat(v_arr + *num_vars);

			}
			printf_wrapper_takes_float(printfbuffer, BUFFER_SIZE, str_out, value.f, "g", args, args_def);
			return add_off;
		case 'G':
			if (*num_vars) {
				(*num_vars)--;
				value.f = orcus_checkfloat(v_arr + *num_vars);

			}
			printf_wrapper_takes_float(printfbuffer, BUFFER_SIZE, str_out, value.f, "G", args, args_def);
			return add_off;
		case 'a':
			if (*num_vars) {
				(*num_vars)--;
				value.f = orcus_checkfloat(v_arr + *num_vars);

			}
			printf_wrapper_takes_float(printfbuffer, BUFFER_SIZE, str_out, value.f, "a", args, args_def);
			return add_off;
		case 'A':
			if (*num_vars) {
				(*num_vars)--;
				value.f = orcus_checkfloat(v_arr + *num_vars);

			}
			printf_wrapper_takes_float(printfbuffer, BUFFER_SIZE, str_out, value.f, "A", args, args_def);
			return add_off;
		case 'p':
		case 'P':
			if (*num_vars) {
				(*num_vars)--;
				value.p = orcus_checkpointer(v_arr + *num_vars);
			}
			printf_wrapper_takes_int(printfbuffer, BUFFER_SIZE, str_out, (orcus_int)value.p, "p", args, args_def);
			return add_off;
		case 'S':
		case 's': // string
		{
			orcus_string* v = nullptr;
			orcus_int addedlen = 0;
			if (*num_vars) {
				(*num_vars)--;
				v = orcus_tostring(v_arr + *num_vars);
			}
			if (v) {
				addedlen = v->size;
				if (args_def[ARG_WIDTH] && addedlen < args[ARG_WIDTH]) {
					orcus_mstring_addchars(v, "", 0); //in case the string provided in the args is constant, v will be constant, too, and this makes it not constant, and prevents trying to reallocate a const char*.
					char* npt = (char*)realloc(v->ptr, sizeof(char) * args[ARG_WIDTH]);
					if (!npt)return 0;
					v->ptr = npt;
					if (!args_def[ARG_LEFTALIGN]) {
						memmove(v->ptr + args[ARG_WIDTH]- addedlen, v->ptr,addedlen);
						memset(v->ptr, ' ', args[ARG_WIDTH] - addedlen);
					}
					else {
						memset(v->ptr + addedlen, ' ', args[ARG_WIDTH] - addedlen);
					}
					v->size = args[ARG_WIDTH];
					orcus_mstrings_append(str_out, v);
					orcus_mstring_delete(v);
				}
				else {
					orcus_mstrings_append(str_out, v);
					orcus_mstring_delete(v);
				}
			}
			else {
				if (args_def[ARG_WIDTH] && args[ARG_WIDTH]){
					char* p=(char*)malloc(args[ARG_WIDTH] * sizeof(char));
					if (p) {
						memset(p, ' ', sizeof(char));
						orcus_mstring_addchars(str_out, p, args[ARG_WIDTH]);
					}
				}
			}
			return add_off;
		}
		case '-':
			if (!mode) {
				args_def[ARG_LEFTALIGN] = true;
				args[ARG_LEFTALIGN] = 1;
			}
			break;
		case '#':
			if (!mode) {
				args_def[ARG_ALTERNATENUM] = true;
				args[ARG_ALTERNATENUM] = 1;
			}
			break;
		case '+':
			if (!mode) {
				args_def[ARG_FORCEPLUS] = true;
				args[ARG_FORCEPLUS] = 1;
			}
			break;
		case ' ':
			if (!mode && !args[ARG_FORCEPLUS]) {
				args_def[ARG_FORCEPLUS] = true;
				args[ARG_FORCEPLUS] = 2;
			}
			break;
		case '.': // X.Y notation. swap to another mode.
			mode = ARG_PERCISION;
			//add_off++;
			break;
		case '0':
			if (!mode) {
				args_def[ARG_PADDINGISZERO]=true;
				args[ARG_PADDINGISZERO] = 1;
			}
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (!mode) { mode = ARG_WIDTH; }
			args_def[mode] = true;
			args[mode] *= 10;
			args[mode] += (c-'0');
			//add_off++;
			break;
		default:
			return add_off;
		}
	}

	return add_off;
}



void orcus_lib_string_format(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //c-style printf formatting.
	if (args_in < 1) {
		orcus_raise_error_nonpointer(M, ORCUS_ERROR_NOT_ENOUGH_ARGS, args_in, 1);
		return;
	};
	if (!args_out) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0);
		return;
	}

	orcus_int var_s = args_in - 1;
	orcus_variable* var_t = (orcus_variable*)malloc(sizeof(orcus_variable)*args_in-1);
	if (!var_t) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}

	for (orcus_int i = 1; i < args_in; i++) {
		orcus_popstack(M, var_t+i - 1);
	}

	orcus_variable strvar;
	orcus_popstack(M,&strvar);
	if (strvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &strvar, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&strvar);
		return;
	}
	orcus_string* s = (orcus_string*)strvar.data.p;

	orcus_int pc = 0;

	orcus_string* outstr = (orcus_string*)malloc(sizeof(orcus_string));
	if (!outstr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	outstr->constant = false;
	outstr->ptr = nullptr;
	outstr->size = 0;
	outstr->references = 0;
		
	
	/*
	ARG1 = 1
	ARG2 = 2
	*/

	while (pc<s->size) {
		char c = s->ptr[pc];
		if (c == '%') {
			pc++;
			if (pc < s->size) {
				pc+=orcus_readformat(s,pc, outstr,var_t,&var_s);
			}
		}
		else {
			orcus_mstring_addchars(outstr, s->ptr+pc, 1);
			pc++;
		}
	}

	for (orcus_int i = 1; i < args_in; i++) {
		orcus_release_var(var_t + i - 1);
	}
	free(var_t);

	orcus_release_var(&strvar);
	
	strvar.type = ORCUS_TYPE_STRING;
	strvar.data.p = outstr;
	orcus_pushstack(M, &strvar);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



/*
	patterns!

	selectors:
	%n - number
	%l - letter
	%w - lowercase letters
	%u - uppercase letters
	%a - alphanumberic
	%s - symbols
	%  - whitespace
	%. - wildcard
	%% - % literal

	specifiers:
	%* - 0 to infinity matches, greedy
	%~ - 0 to infinity matches
	%- - 1 to infinity matches
	%+ - 1 to infinity matches, greedy
	%? - 0 or 1 matches
	%! - 1 match (implicit behavior made explicit)
	%/ - 0 matches, next char
	%\ - 0 matches, greedy

*/


enum ORCUS_PAT_MATCH {
	//count flags
	MATCH_SINGLE = (1<<0), //we only want a single char, and stop as soon as we find what we want.
	MATCH_AT_LEAST_ONE = (1<<1), //we can match many chars, and keep going until the rule stops being true.
	//behavior flags
	MATCH_OPTIONAL = (1<<2), //if we aren't required, just set passed to true. still iterates through chars to advance filters
	MATCH_GREEDY = (1<<3), //if flag, do not allow traversal through first_good_char
	MATCH_INVERTED = (1<<4), //we do NOT want this char.
};

struct ORCUS_PATTERN{
	bool allowed_chars[256] = { false };
	int match_type = MATCH_SINGLE;
	orcus_int first_good_char = 0; //succesive checks will start at this +1 if not MATCH_GREEDY
	orcus_int last_good_char = 0; //and end at this.
	bool passed = false;
};

void orcus_initialize_pattern(ORCUS_PATTERN* P) {
	if (!P)return;
	memset(P, 0, sizeof(ORCUS_PATTERN));
	P->match_type = MATCH_SINGLE;
}


inline void orcus_init_pattern_filter_numbers(ORCUS_PATTERN* P){
	if (!P)return;
	int i = '0';
	while (i <= '9') {
		P->allowed_chars[i] = true;
		i++;
	}
}
inline void orcus_init_pattern_filter_letter_upper(ORCUS_PATTERN* P) {
	if (!P)return;
	int i = 'A';
	while (i <= 'Z') {
		P->allowed_chars[i] = true;
		i++;
	}
}

inline void orcus_init_pattern_filter_letter_lower(ORCUS_PATTERN* P) {
	if (!P)return;
	int i = 'a';
	while (i <= 'z') {
		P->allowed_chars[i] = true;
		i++;
	}
}

inline void orcus_init_pattern_filter_wildcard(ORCUS_PATTERN* P) {
	if (!P)return;
	int i = '\x00';
	while (i <= (unsigned char)'\xFF') {
		P->allowed_chars[i] = true;
		i++;
	}
}

inline void orcus_init_pattern_filter_symbols(ORCUS_PATTERN* P) {
	if (!P)return;
	int i = '\x21';
	while (i <= '\x2F') {
		P->allowed_chars[i] = true;
		i++;
	}
	i = '\x3A';
	while (i <= '\x40') {
		P->allowed_chars[i] = true;
		i++;
	}
	i = '\x5B';
	while (i <= '\x60') {
		P->allowed_chars[i] = true;
		i++;
	}
	i = '\x7B';
	while (i <= '\x7E') {
		P->allowed_chars[i] = true;
		i++;
	}
}

inline void orcus_init_pattern_filter_whitespace(ORCUS_PATTERN* P) {
	if (!P)return;
	P->allowed_chars[' '] = true;
	P->allowed_chars['\t'] = true;
	P->allowed_chars['\n'] = true;
	P->allowed_chars['\r'] = true;
}

inline void orcus_init_pattern_filter_hexadecimal(ORCUS_PATTERN* P) {
	if (!P)return;
	P->allowed_chars['a'] = true;
	P->allowed_chars['b'] = true;
	P->allowed_chars['c'] = true;
	P->allowed_chars['d'] = true;
	P->allowed_chars['e'] = true;
	P->allowed_chars['f'] = true;
	P->allowed_chars['A'] = true;
	P->allowed_chars['B'] = true;
	P->allowed_chars['C'] = true;
	P->allowed_chars['D'] = true;
	P->allowed_chars['E'] = true;
	P->allowed_chars['F'] = true;
	P->allowed_chars['0'] = true;
	P->allowed_chars['1'] = true;
	P->allowed_chars['2'] = true;
	P->allowed_chars['3'] = true;
	P->allowed_chars['4'] = true;
	P->allowed_chars['5'] = true;
	P->allowed_chars['6'] = true;
	P->allowed_chars['7'] = true;
	P->allowed_chars['8'] = true;
	P->allowed_chars['9'] = true;
}

inline void orcus_init_mattern_invert_filter_chars(ORCUS_PATTERN* P) {
	if (!P)return;
	int i = '\x00';
	while (i <= (unsigned char)'\xFF') {
		P->allowed_chars[i] = !P->allowed_chars[i];
		i++;
	}
}

ORCUS_PATTERN* orcus_patternize_string(orcus_string* str,orcus_int* num_out) {
	orcus_int n = 0;
	ORCUS_PATTERN* out = nullptr;


	for (orcus_int c = 0; c < str->size; c++) {
		char cc = str->ptr[c];
		//printf("%i %c\n",c,cc);
		ORCUS_PATTERN* nptr = (ORCUS_PATTERN*)realloc(out, sizeof(ORCUS_PATTERN) * (n + 1));
		if (!nptr)return nullptr;
		out = nptr;
		ORCUS_PATTERN* p = out+n;
		n++;
		orcus_initialize_pattern(p);
		
		//look for the first part of the pattern, the character set.
		if (cc == '%' && (c + 1) < str->size) {
			char nc = str->ptr[c + 1];
			c++;
			switch (nc) {
			case 'n':
				orcus_init_pattern_filter_numbers(p);
				break;
			case 'l':
				orcus_init_pattern_filter_letter_upper(p);
				orcus_init_pattern_filter_letter_lower(p);
				break;
			case 'u':
				orcus_init_pattern_filter_letter_upper(p);
				break;
			case 'w':
				orcus_init_pattern_filter_letter_lower(p);
				break;
			case 'a':
				orcus_init_pattern_filter_numbers(p);
				orcus_init_pattern_filter_letter_upper(p);
				orcus_init_pattern_filter_letter_lower(p);
				break;
			case 's':
				orcus_init_pattern_filter_symbols(p);
				break;
			case ' ':
				orcus_init_pattern_filter_whitespace(p);
				break;
			case '.':
				orcus_init_pattern_filter_wildcard(p);
				break;
			case 'x':
				orcus_init_pattern_filter_hexadecimal(p);
				break;
			case '%': //explicitly define this as intented behavior. if you're not escaping %, then you're just asking for trouble later down the line.
			default:
				p->allowed_chars[nc] = true;
			}
		}
		else {
			p->allowed_chars[cc] = true;
		}

		p->match_type = MATCH_SINGLE;

		//next, look for the second half, the selector.
		if (c + 1 >= str->size)continue;
		
		cc = str->ptr[c+1];
		if (cc == '%' && (c + 2) < str->size) {
			char nc = str->ptr[c + 2];
			c+=2;
			switch (nc) {
			case '+':
				p->match_type = MATCH_AT_LEAST_ONE | MATCH_GREEDY;
				break;
			case '-':
				p->match_type = MATCH_AT_LEAST_ONE;
				break;
			case '*':
				p->match_type = MATCH_AT_LEAST_ONE | MATCH_OPTIONAL | MATCH_GREEDY;
				break;
			case '~':
				p->match_type = MATCH_AT_LEAST_ONE | MATCH_OPTIONAL;
				break;
			case '?':
				p->match_type = MATCH_SINGLE | MATCH_OPTIONAL;
				break;
			case '!':
				p->match_type = MATCH_SINGLE;
				break;
			case '/':
				orcus_init_mattern_invert_filter_chars(p);
				p->match_type = MATCH_AT_LEAST_ONE | MATCH_OPTIONAL | MATCH_GREEDY;
				break;
			case '\\':
				orcus_init_mattern_invert_filter_chars(p);
				p->match_type = MATCH_AT_LEAST_ONE | MATCH_OPTIONAL;
				break;
			case '#':
				orcus_init_mattern_invert_filter_chars(p);
				p->match_type = MATCH_AT_LEAST_ONE | MATCH_GREEDY;
				break;
			case '@':
				orcus_init_mattern_invert_filter_chars(p);
				p->match_type = MATCH_AT_LEAST_ONE;
				break;
			case '|':
				orcus_init_mattern_invert_filter_chars(p);
				p->match_type = MATCH_SINGLE | MATCH_OPTIONAL;
				break;
			case '=':
				orcus_init_mattern_invert_filter_chars(p);
				p->match_type = MATCH_SINGLE;
				break;
			case '%': //see above.
			default:
				c -= 2; //invalid selector? we drop the character advance, so we can treat it like a normal character sequence.
			}
		}


	}

	if (num_out)*num_out = n;
	return out;
}


bool orcus_evaluate_patterns(orcus_string* str, ORCUS_PATTERN* patterns, orcus_int num_patterns, orcus_int str_start, orcus_int* pattern_str_start, orcus_int* pattern_str_end) {

#ifdef ORCUS_DEBUG

	for (orcus_int i = 0; i < num_patterns; i++) {

		ORCUS_PATTERN P = patterns[i];
		printf("pattern %zi: ",i);

		if (P.match_type & MATCH_SINGLE) {
			printf("MATCH_SINGLE ");
		}
		if (P.match_type & MATCH_AT_LEAST_ONE) {
			printf("MATCH_AT_LEAST_ONE ");
		}
		if (P.match_type & MATCH_GREEDY) {
			printf("MATCH_GREEDY ");
		}
		if (P.match_type & MATCH_OPTIONAL) {
			printf("MATCH_OPTIONAL ");
		}
		if (P.match_type & MATCH_INVERTED) {
			printf("MATCH_INVERTED ");
		}

		for (int n = 0; n <= 0xFF; n++) {
			if (P.allowed_chars[n])putchar(n);
		}
		putchar('\n');
	}

#endif



	*pattern_str_start = 0;
	*pattern_str_end = 0;
	if (str_start >= str->size)return false;
	str_start--;
	if (!num_patterns)return true;

	orcus_int advanced_from = 0;
	orcus_int current_pattern = 0;
	advance:
	str_start++;
	if (str_start >= str->size)return false;
	*pattern_str_start = str_start;
	advanced_from = 0;
	current_pattern = 0;
	for (orcus_int n = 0; n < num_patterns; n++) {
		patterns[n].passed = false;
		patterns[n].first_good_char = 0;
		patterns[n].last_good_char = 0;
	}
	for (orcus_int c = str_start; c < str->size; c++) {
		unsigned char cc = str->ptr[c];
		
		
		ORCUS_PATTERN* p = patterns+current_pattern;
		if (!current_pattern && !p->passed)*pattern_str_start = c;
		if (p->allowed_chars[cc]) {
			if (p->match_type & MATCH_SINGLE) {
				current_pattern++;
			}
			else {
				if (!p->passed) {
					p->first_good_char = c;
				}
				p->last_good_char = c;
			}
			p->passed = true;
		}
		else {
			if (p->passed) {
				if (!(p->match_type & MATCH_GREEDY)) {
					advanced_from = c;
					c = p->first_good_char + 1;
				}
				else {
					c--;
				}
				current_pattern++;
			}else if (p->match_type & MATCH_OPTIONAL) {
				current_pattern++;
				c--;
			}

			if(c>=advanced_from && !(p->match_type & MATCH_OPTIONAL) && !p->passed ) {
				goto advance;
			}
		}	

		*pattern_str_end = c;

		if (current_pattern == num_patterns) {
			bool anypass = false;
			orcus_int passed = 0;
			orcus_int req_pas = 0;
			for (orcus_int n = 0; n < num_patterns; n++) {
				if ((patterns[n].match_type & MATCH_INVERTED) ? (!patterns[n].passed) : patterns[n].passed) {
					if (!(patterns[n].match_type & MATCH_OPTIONAL)) {
						passed++;
					}
					anypass = true;
				}
				if (!(patterns[n].match_type & MATCH_OPTIONAL)) {
					req_pas++;
				}
			}
			//printf("%i %i %i\n", anypass, passed, req_pas);
			if (anypass && passed >= req_pas) {
				break;
			}
			else {
				goto advance;
			}
		}
		if (c+1 == str->size && p->match_type&MATCH_AT_LEAST_ONE) {
			current_pattern++;
			if (current_pattern == num_patterns)break;
			c = p->first_good_char;
		}

	}


	bool anypass = false;
	orcus_int passed = 0;
	orcus_int req_pas = 0;
	for (orcus_int n = 0; n < num_patterns; n++) {
		if ((patterns[n].match_type & MATCH_INVERTED) ? (!patterns[n].passed) : patterns[n].passed) {
			if (!(patterns[n].match_type & MATCH_OPTIONAL)) {
				passed++;
			}
			anypass = true;
		}
		if (!(patterns[n].match_type & MATCH_OPTIONAL)) {
			req_pas++;
		}
	}
	if (anypass && passed >= req_pas) {
		return true;
	}
	else {
		goto advance;
	}

}



void orcus_lib_string_p_find(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if(ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2, 3))return;
	if (!args_out) {
		if(args_in>2)orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable startat;
	if (args_in > 2) {
		orcus_popstack(M, &startat);
		if (startat.type != ORCUS_TYPE_INT) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &startat, ORCUS_TYPE_INT, 3);
			orcus_release_var(&startat);
			return;
		}
	}
	else {
		startat.type = ORCUS_TYPE_NIL;
	}

	orcus_variable searchforvar;
	orcus_popstack(M, &searchforvar);
	if (searchforvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &searchforvar, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&searchforvar);
		return;
	}

	orcus_variable strvar;
	orcus_popstack(M, &strvar);
	if (strvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &strvar, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&strvar);
		return;
	}


	orcus_string* str = (orcus_string*)strvar.data.p;
	orcus_string* pat_str = (orcus_string*)searchforvar.data.p;

	orcus_int num_pats = 0;
	ORCUS_PATTERN* P = orcus_patternize_string(pat_str,&num_pats);


	orcus_int start = 0;
	orcus_int end = 0;
	bool found=orcus_evaluate_patterns(str,P,num_pats, startat.type ? startat.data.i : 0,&start,&end);

	free(P);
	
	orcus_release_var(&searchforvar);
	orcus_release_var(&strvar);
	
	


	if (found) {
		searchforvar.type = ORCUS_TYPE_INT;
		searchforvar.data.i = start;
		strvar.type = ORCUS_TYPE_INT;
		strvar.data.i = end;
	}
	else {
		searchforvar.type = ORCUS_TYPE_NIL;
		searchforvar.data.i = 0;
		strvar.type = ORCUS_TYPE_NIL;
		strvar.data.i = 0;
	}

	orcus_pushstack(M, &searchforvar);
	if (args_out > 1) {
		orcus_pushstack(M, &strvar);
	}

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 2);
}

void orcus_lib_string_p_extract(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //returns all instances of a match in an array.
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable matchvar;
	orcus_popstack(M, &matchvar);
	if (matchvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &matchvar, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&matchvar);
		return;
	}

	orcus_variable strvar;
	orcus_popstack(M, &strvar);
	if (strvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &strvar, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&strvar);
		return;
	}

	orcus_string* str = (orcus_string*)strvar.data.p;
	orcus_string* match = (orcus_string*)matchvar.data.p;

	orcus_int num_pats = 0;
	ORCUS_PATTERN* P = orcus_patternize_string(match, &num_pats);

	orcus_array* arr=orcus_newarray();


	orcus_int count = 0;
	orcus_int start = 0;
	orcus_int end = 0;
	while (orcus_evaluate_patterns(str, P, num_pats, end, &start, &end)) {
		orcus_string* ss=orcus_mstring_substring(str,start,end);
		if (!ss) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			orcus_release_var(&strvar);
			orcus_release_var(&matchvar);
			free(P);
			return;
		}
		orcus_variable inter;
		inter.type = ORCUS_TYPE_STRING;
		inter.data.p = ss;
		orcus_setarray(arr, &inter, count);
		count++;
		end++;
	}

	orcus_release_var(&strvar);
	orcus_release_var(&matchvar);
	free(P);
	
	matchvar.type = ORCUS_TYPE_ARRAY;
	matchvar.data.p = arr;
	orcus_pushstack(M,&matchvar);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

void orcus_lib_string_p_replace(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 3,4))return;
	if (!args_out) {
		if(args_in>3)orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable max_replacments;
	if (args_in > 3) {
		orcus_popstack(M, &max_replacments);
		if (max_replacments.type != ORCUS_TYPE_INT) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &max_replacments, ORCUS_TYPE_INT, 4);
			orcus_release_var(&max_replacments);
			return;
		}
	}
	else {
		max_replacments.type = ORCUS_TYPE_NIL;
	}

	orcus_variable repvar;
	orcus_popstack(M, &repvar);
	if (repvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &repvar, ORCUS_TYPE_STRING, 3);
		orcus_release_var(&repvar);
		return;
	}

	orcus_variable matchvar;
	orcus_popstack(M, &matchvar);
	if (matchvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &matchvar, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&matchvar);
		return;
	}

	orcus_variable strvar;
	orcus_popstack(M, &strvar);
	if (strvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &strvar, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&strvar);
		return;
	}

	orcus_string* str = (orcus_string*)strvar.data.p;
	orcus_string* match = (orcus_string*)matchvar.data.p;
	orcus_string* rep = (orcus_string*)repvar.data.p;

	orcus_int num_pats = 0;
	ORCUS_PATTERN* P = orcus_patternize_string(match, &num_pats);
	

	orcus_string* outstr = orcus_cstring_to_mstring((char*)"", 0);
	

	orcus_int count = 0;

	orcus_int start = 0;
	orcus_int end = 0;
	orcus_int lastendmatch = 0;
	while (orcus_evaluate_patterns(str, P, num_pats, end, &start, &end)) {
		if (max_replacments.type && count >= max_replacments.data.i)break;

		//printf("%i=%i\n", start, end);

		orcus_string* app = orcus_mstring_substring(str, lastendmatch, start - 1);
		if (!app) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			free(P);
			return;
		}
		orcus_mstrings_append(outstr, app);
		orcus_mstrings_append(outstr, rep);
	
		
		end++;
		lastendmatch = end;
		count++;
	}
	orcus_mstrings_append(outstr, orcus_mstring_substring(str,lastendmatch,str->size-1));

	orcus_release_var(&matchvar);
	orcus_release_var(&repvar);
	orcus_release_var(&strvar);
	free(P);
	
	matchvar.type = ORCUS_TYPE_STRING;
	matchvar.data.p = outstr;
	orcus_pushstack(M, &matchvar);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_string_p_count(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable matchvar;
	orcus_popstack(M, &matchvar);
	if (matchvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &matchvar, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&matchvar);
		return;
	}

	orcus_variable strvar;
	orcus_popstack(M, &strvar);
	if (strvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &strvar, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&strvar);
		return;
	}

	orcus_string* str = (orcus_string*)strvar.data.p;
	orcus_string* match = (orcus_string*)matchvar.data.p;

	orcus_int num_pats = 0;
	ORCUS_PATTERN* P = orcus_patternize_string(match, &num_pats);


	orcus_int count = 0;

	orcus_int start = 0;
	orcus_int end = 0;
	while (orcus_evaluate_patterns(str, P, num_pats, end, &start, &end)) {
		//printf("%i=%i\n", start, end);

		end++;
		count++;
	}

	orcus_release_var(&strvar);
	orcus_release_var(&matchvar);
	free(P);

	matchvar.type = ORCUS_TYPE_INT;
	matchvar.data.i = count;
	orcus_pushstack(M, &matchvar);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_string_escape_orcus(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable instr;
	orcus_popstack(M,&instr);
	if (instr.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &instr, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&instr);
		return;
	}

	orcus_string* os = (orcus_string*)malloc(sizeof(orcus_string));
	if (!os) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	os->size = 0;
	os->ptr = nullptr;
	os->constant = false;
	os->references = 0;

	orcus_string* is = (orcus_string*)instr.data.p;
	orcus_int pos = 0;
	while (pos < is->size) {
		char c = is->ptr[pos];
		switch (c) {
		case '\"':
			orcus_mstring_addchars(os, (char*)"\\\"", 2);
			break;
		case '\\':
			orcus_mstring_addchars(os, (char*)"\\\\", 2);
			break;
		default:
			orcus_mstring_addchars(os, is->ptr + pos, 1);
		}
		
		pos++;
	}

	orcus_release_var(&instr);
	
	instr.type = ORCUS_TYPE_STRING;
	instr.data.p = os;
	orcus_pushstack(M, &instr);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_string_escape_url(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable instr;
	orcus_popstack(M, &instr);
	if (instr.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &instr, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&instr);
		return;
	}

	orcus_string* os = (orcus_string*)malloc(sizeof(orcus_string));
	if (!os) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	os->size = 0;
	os->ptr = nullptr;
	os->constant = false;
	os->references = 0;

	orcus_string* is = (orcus_string*)instr.data.p;
	orcus_int pos = 0;
	while (pos < is->size) {
		char c = is->ptr[pos];


		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c=='-' || c == '_' || c == '~' || c == '.') {
			orcus_mstring_addchars(os, is->ptr + pos, 1);
		}
		else { //need to percent encode these.
			orcus_mstring_addchars(os, (char*)"%" , 1);
			char sc = (c & 0xF0)>>4;
			if (sc<0xA) {
				char n = sc+0x30;
				orcus_mstring_addchars(os, &n, 1);
			}
			else {
				char n = sc + 0x41-0x0A;
				orcus_mstring_addchars(os, &n, 1);
			}
			c &= 0xF;
			if (c < 0xA) {
				char n = c + 0x30;
				orcus_mstring_addchars(os, &n, 1);
			}
			else {
				char n = c + 0x41 - 0x0A;
				orcus_mstring_addchars(os, &n, 1);
			}

		}

		pos++;
	}

	orcus_release_var(&instr);
	
	instr.type = ORCUS_TYPE_STRING;
	instr.data.p = os;
	orcus_pushstack(M, &instr);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_string_escape_c(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable instr;
	orcus_popstack(M, &instr);
	if (instr.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &instr, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&instr);
		return;
	}

	orcus_string* os = (orcus_string*)malloc(sizeof(orcus_string));
	if (!os) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	os->size = 0;
	os->ptr = nullptr;
	os->constant = false;
	os->references = 0;

	orcus_string* is = (orcus_string*)instr.data.p;
	orcus_int pos = 0;
	while (pos < is->size) {
		char c = is->ptr[pos];
		switch (c) {
		case '\a':
			orcus_mstring_addchars(os, (char*)"\\a", 2);
			break;
		case '\b':
			orcus_mstring_addchars(os, (char*)"\\b", 2);
			break;
		case '\x1B': //ASCII escape character
			orcus_mstring_addchars(os, (char*)"\\x1B", 4);
			break;
		case '\f':
			orcus_mstring_addchars(os, (char*)"\\f", 2);
			break;
		case '\n':
			orcus_mstring_addchars(os, (char*)"\\n", 2);
			break;
		case '\r':
			orcus_mstring_addchars(os, (char*)"\\r", 2);
			break;
		case '\t':
			orcus_mstring_addchars(os, (char*)"\\t", 2);
			break;
		case '\v':
			orcus_mstring_addchars(os, (char*)"\\v", 2);
			break;
		case '\\':
			orcus_mstring_addchars(os, (char*)"\\\\", 2);
			break;
		case '\'':
			orcus_mstring_addchars(os, (char*)"\\'", 2);
			break;
		case '\"':
			orcus_mstring_addchars(os, (char*)"\\\"", 2);
			break;
		case '\?':
			orcus_mstring_addchars(os, (char*)"\\?", 2);
			break;
		case '\0':
			orcus_mstring_addchars(os, (char*)"\\0", 2);
			break;
		default:
			orcus_mstring_addchars(os, is->ptr + pos, 1);
		}

		pos++;
	}

	orcus_release_var(&instr);
	
	instr.type = ORCUS_TYPE_STRING;
	instr.data.p = os;
	orcus_pushstack(M, &instr);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

void orcus_lib_string_escape_html(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable instr;
	orcus_popstack(M, &instr);
	if (instr.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &instr, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&instr);
		return;
	}

	orcus_string* os = (orcus_string*)malloc(sizeof(orcus_string));
	if (!os) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	os->size = 0;
	os->ptr = nullptr;
	os->constant = false;
	os->references = 0;

	orcus_string* is = (orcus_string*)instr.data.p;
	orcus_int pos = 0;
	while (pos < is->size) {
		char c = is->ptr[pos];
		switch (c) {
		case '<':
			orcus_mstring_addchars(os, (char*)"&lt;", 4);
			break;
		case '>':
			orcus_mstring_addchars(os, (char*)"&gt;", 4);
			break;
		case '&':
			orcus_mstring_addchars(os, (char*)"&amp;", 5);
			break;
		case '\"':
			orcus_mstring_addchars(os, (char*)"&quot;", 6);
			break;
		case '\'':
			orcus_mstring_addchars(os, (char*)"&#39;", 6);
			break;
		default:
			orcus_mstring_addchars(os, is->ptr + pos, 1);
		}

		pos++;
	}

	orcus_release_var(&instr);
	
	instr.type = ORCUS_TYPE_STRING;
	instr.data.p = os;
	orcus_pushstack(M, &instr);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



void orcus_lib_string_copy_string(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}
	orcus_variable in;
	orcus_popstack(M, &in);
	if (in.type != ORCUS_TYPE_STRING){
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&in);
		return;
	}
	orcus_string* instr = (orcus_string*)in.data.p;
	orcus_string* nstr=(orcus_string*)malloc(sizeof(orcus_string));
	if (!nstr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	if (instr->size) {
		char* sp = (char*)malloc(instr->size);
		if (!sp) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			free(nstr);
			return;
		}
		nstr->ptr = sp;
		memcpy(sp, instr->ptr, instr->size);
	}
	else {
		nstr->ptr = nullptr;
	}
	nstr->constant = false;
	nstr->references = 0;
	nstr->size = instr->size;

	orcus_release_var(&in);
	in.data.p = nstr;
	orcus_pushstack(M, &in);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

//iterates all matches in a function and lets you do things to them
void orcus_lib_string_match_function(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 3))return;
	orcus_variable var_str;
	orcus_variable var_match;
	orcus_variable var_func;

	orcus_popstack(M, &var_func);
	if (var_func.type != ORCUS_TYPE_FUNCTION) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_func, ORCUS_TYPE_FUNCTION, 3);
		orcus_release_var(&var_func);
		return;
	}

	orcus_popstack(M, &var_match);
	if (var_match.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_match, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&var_match);
		orcus_release_var(&var_func);
		return;
	}

	orcus_popstack(M, &var_str);
	if (var_str.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_str, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&var_str);
		orcus_release_var(&var_match);
		orcus_release_var(&var_func);
		return;
	}

	if (!args_out) {
		orcus_release_var(&var_str);
		orcus_release_var(&var_match);
		orcus_release_var(&var_func);
		return;
	}
	orcus_state* SubM = orcus_get_child_state(M);
	orcus_string* out_str=(orcus_string*)malloc(sizeof(orcus_string));
	if (!out_str || !SubM) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		orcus_release_var(&var_str);
		orcus_release_var(&var_match);
		orcus_release_var(&var_func);
		return;
	}
	memset(out_str, 0, sizeof(orcus_string));

	orcus_function* previous = SubM->bytecode;
	SubM->bytecode = ((orcus_function*)var_func.data.p);
	orcus_variable out;

	orcus_string* base_str = (orcus_string*)var_str.data.p;
	orcus_string* find_str = (orcus_string*)var_match.data.p;
	orcus_int i = 0;
	while (true) {
		next_check:
		if (i> base_str->size-find_str->size || !find_str->size) {
			orcus_mstring_addchars(out_str, base_str->ptr + i, base_str->size-i);
			break;
		}

		for (orcus_int n = 0; n < find_str->size; n++) {
			if (base_str->ptr[i + n] != find_str->ptr[n]) {
				orcus_mstring_addchars(out_str, base_str->ptr + i, n + 1);
				i++;
				goto next_check;
			}
		}
		SubM->programcounter = 0;

		orcus_variable push;
		push.type = ORCUS_TYPE_INT;
		push.data.i = i + find_str->size - 1;
		orcus_pushstack(SubM, &push);
		push.type = ORCUS_TYPE_INT;
		push.data.i = i;
		orcus_pushstack(SubM, &push);
		push.type = ORCUS_TYPE_STRING;
		push.data.p = orcus_mstring_substring(base_str, i, i + find_str->size - 1);
		orcus_pushstack(SubM, &push);
		push.type = ORCUS_TYPE_STRING;
		base_str->references++; //add a refrence to prevent freeing in use.
		push.data.p = base_str;
		orcus_pushstack(SubM, &push);
		while (orcus_stepstate(SubM));
		if (SubM->errorcode) {
			base_str->references--;
			orcus_mstring_delete(out_str);
			orcus_release_var(&var_str);
			orcus_release_var(&var_match);
			orcus_release_var(&var_func);
			return;
		}
		SubM->programcounter = 0;
		orcus_popstack(SubM, &push);
		while (SubM->sizeofstack)orcus_discard_top_of_stack(SubM);
		base_str->references--;
		if (push.type == ORCUS_TYPE_STRING) {
			orcus_mstrings_append(out_str, (orcus_string*)push.data.p);
		}
		orcus_release_var(&push);

		i += find_str->size;
	}

	SubM->bytecode = previous;
	orcus_clearstate(SubM);

	orcus_release_var(&var_str);
	orcus_release_var(&var_match);
	orcus_release_var(&var_func);

	out.type = ORCUS_TYPE_STRING;
	out.data.p = out_str;
	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}


//iterates all matches in a function and lets you do things to them
void orcus_lib_string_p_match_function(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 3))return;
	orcus_variable var_str;
	orcus_variable var_match;
	orcus_variable var_func;

	orcus_popstack(M, &var_func);
	if (var_func.type != ORCUS_TYPE_FUNCTION) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_func, ORCUS_TYPE_FUNCTION, 3);
		orcus_release_var(&var_func);
		return;
	}

	orcus_popstack(M, &var_match);
	if (var_match.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_match, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&var_match);
		orcus_release_var(&var_func);
		return;
	}

	orcus_popstack(M, &var_str);
	if (var_str.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_str, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&var_str);
		orcus_release_var(&var_match);
		orcus_release_var(&var_func);
		return;
	}

	if (!args_out) {
		orcus_release_var(&var_str);
		orcus_release_var(&var_match);
		orcus_release_var(&var_func);
		return;
	}
	orcus_state* SubM = orcus_get_child_state(M);
	orcus_string* out_str = (orcus_string*)malloc(sizeof(orcus_string));
	if (!out_str || !SubM) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		orcus_release_var(&var_str);
		orcus_release_var(&var_match);
		orcus_release_var(&var_func);
		return;
	}
	memset(out_str, 0, sizeof(orcus_string));

	orcus_function* previous = SubM->bytecode;
	SubM->bytecode = ((orcus_function*)var_func.data.p);
	orcus_variable out;

	orcus_string* base_str = (orcus_string*)var_str.data.p;
	orcus_string* find_str = (orcus_string*)var_match.data.p;

	//orcus_int i = 0;

	orcus_int num_pats = 0;
	ORCUS_PATTERN* P = orcus_patternize_string(find_str, &num_pats);
	orcus_int start = 0;
	orcus_int end = 0;
	orcus_int i = 0;
	while (true) {
	next_check:
		i = end;
		if (start > base_str->size - find_str->size || !find_str->size) {
			orcus_mstring_addchars(out_str, base_str->ptr + i, base_str->size - i);
			break;
		}
		
		if (!orcus_evaluate_patterns(base_str, P, num_pats, end, &start, &end)) {
			orcus_mstring_addchars(out_str,base_str->ptr+i,1);
			i++;
			start = end = i;
			goto next_check;
		}
		orcus_mstring_addchars(out_str, base_str->ptr + i, start-i);


		SubM->programcounter = 0;

		orcus_variable push;
		push.type = ORCUS_TYPE_INT;
		push.data.i = end;
		orcus_pushstack(SubM, &push);
		push.type = ORCUS_TYPE_INT;
		push.data.i = start;
		orcus_pushstack(SubM, &push);
		push.type = ORCUS_TYPE_STRING;
		push.data.p = orcus_mstring_substring(base_str, start, end);
		orcus_pushstack(SubM, &push);
		push.type = ORCUS_TYPE_STRING;
		base_str->references++; //add a refrence to prevent freeing in use.
		push.data.p = base_str;
		orcus_pushstack(SubM, &push);
		while (orcus_stepstate(SubM));
		if (SubM->errorcode) {
			base_str->references--;
			orcus_mstring_delete(out_str);
			orcus_release_var(&var_str);
			orcus_release_var(&var_match);
			orcus_release_var(&var_func);
			free(P);
			return;
		}
		SubM->programcounter = 0;
		orcus_popstack(SubM, &push);
		while (SubM->sizeofstack)orcus_discard_top_of_stack(SubM);
		base_str->references--;
		if (push.type == ORCUS_TYPE_STRING) {
			orcus_mstrings_append(out_str, (orcus_string*)push.data.p);
		}
		orcus_release_var(&push);

		end++;
		start = end;
	}
	free(P);

	SubM->bytecode = previous;
	orcus_clearstate(SubM);

	orcus_release_var(&var_str);
	orcus_release_var(&var_match);
	orcus_release_var(&var_func);

	out.type = ORCUS_TYPE_STRING;
	out.data.p = out_str;
	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

//makes a string of input string string times
void orcus_lib_string_repeat_chars(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2))return;

	orcus_variable var_count;
	orcus_variable var_str;

	orcus_popstack(M, &var_count);
	if (var_count.type != ORCUS_TYPE_INT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_count, ORCUS_TYPE_INT, 2);
		orcus_release_var(&var_count);
		return;
	}

	orcus_popstack(M, &var_str);
	if (var_str.type != ORCUS_TYPE_STRING && var_str.type != ORCUS_TYPE_INT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_str, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&var_str);
		return;
	}

	if (!args_out) {
		orcus_release_var(&var_str);
		return;
	}

	orcus_int sizeneed = 1;
	if (var_str.type == ORCUS_TYPE_STRING) sizeneed = ((orcus_string*)var_str.data.p)->size;
	if (var_count.data.i < 0)var_count.data.i = 0;


	orcus_variable out;
	if (!sizeneed || !var_count.data.i) { //easy, empty string
		orcus_release_var(&var_str);
		orcus_string* outstr=orcus_cstring_const_to_mstring(nullptr, 0);
		if (!outstr) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		out.data.p = outstr;
		out.type = ORCUS_TYPE_STRING;
	}
	else {

		char* buffer = (char*)malloc(sizeof(char) * var_count.data.i * sizeneed);
		if (!buffer) {
			orcus_release_var(&var_str);
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}

		orcus_string* outstr = orcus_cstring_promote_to_mstring(buffer, var_count.data.i * sizeneed);
		if (!outstr) {
			free(buffer);
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}


		if (var_str.type == ORCUS_TYPE_STRING) {
			while (var_count.data.i) {
				var_count.data.i--;
				memcpy(buffer+ (sizeneed* var_count.data.i), ((orcus_string*)var_str.data.p)->ptr, sizeneed);
			}
		}
		else {
			memset(buffer, var_str.data.i & 0xFF, var_count.data.i * sizeneed);
		}
		orcus_release_var(&var_str);
		
		out.type = ORCUS_TYPE_STRING;
		out.data.p = outstr;

	}

	orcus_pushstack(M, &out);

	
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_string_set_chars(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 3))return;

	orcus_variable var_position;
	orcus_variable var_replace;
	orcus_variable var_str;

	orcus_popstack(M, &var_position);
	if (var_position.type != ORCUS_TYPE_INT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_position, ORCUS_TYPE_INT, 3);
		orcus_release_var(&var_position);
		return;
	}

	orcus_popstack(M, &var_replace);
	if (var_replace.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_replace, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&var_replace);
		return;
	}

	orcus_popstack(M, &var_str);
	if (var_str.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_str, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&var_replace);
		orcus_release_var(&var_str);
		return;
	}

	if (!args_out) {
		orcus_release_var(&var_replace);
		orcus_release_var(&var_str);
		return;
	}

	orcus_string* instr = ((orcus_string*)var_str.data.p);
	orcus_string* repstr = ((orcus_string*)var_replace.data.p);

	orcus_int shift = -var_position.data.i;
	if (shift < 0)shift = 0;

	if (var_position.data.i < 0)var_position.data.i = 0;

	orcus_int additional = repstr->size - instr->size + var_position.data.i;
	if (additional < 0)additional = 0;
	additional += shift;

	if (!(instr->size + additional)) { //empty string
		orcus_release_var(&var_str);
		orcus_release_var(&var_replace);
		orcus_string* outstr = orcus_cstring_const_to_mstring(nullptr, 0);
		if (!outstr) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		orcus_variable out;
		out.data.p = outstr;
		out.type = ORCUS_TYPE_STRING;
		orcus_pushstack(M, &out);
		return;
	}

	char* buffer=(char*)malloc(instr->size + additional);

	if (!buffer) {
		orcus_release_var(&var_replace);
		orcus_release_var(&var_str);
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}

	orcus_string* outstr = orcus_cstring_promote_to_mstring(buffer, instr->size + additional);
	if (!outstr) {
		free(buffer);
		orcus_release_var(&var_replace);
		orcus_release_var(&var_str);
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	memset(buffer, 0, instr->size + additional);
	memcpy(buffer+ shift, instr->ptr, instr->size);
	memcpy(buffer + var_position.data.i, repstr->ptr, repstr->size);

	orcus_release_var(&var_replace);
	orcus_release_var(&var_str);

	orcus_variable out;
	out.type = ORCUS_TYPE_STRING;
	out.data.p = outstr;
	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}