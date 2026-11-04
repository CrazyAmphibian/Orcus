#include "orcus_lib_table.hpp"
#include"../orcus.hpp"
#include"../orcus_error.hpp"

#include <malloc.h>
#include <cstring>

void orcus_lib_table_copy(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //basically the same as array.copy
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable tab_var;
	orcus_popstack(M,&tab_var);
	if (tab_var.type != ORCUS_TYPE_TABLE) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &tab_var, ORCUS_TYPE_TABLE, 1);
		orcus_release_var(&tab_var);
		return;
	}

	orcus_variable new_tab_var;

	orcus_table* newtab = orcus_newtable();
	orcus_table* oldtab = (orcus_table*)tab_var.data.p;

	for (uint8_t t = 0; t < ORCUS_NUMBER_OF_TYPES; t++){
		orcus_subtable st_n = newtab->data[t];
		orcus_subtable st_o = oldtab->data[t];

		st_n.values=(orcus_variable*)malloc(sizeof(orcus_variable) * st_o.size);
		if (!st_n.values) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		
		st_n.keys = (orcus_variable*)malloc(sizeof(orcus_variable) * st_o.size);
		if (!st_n.keys) {
			free(st_n.values);
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}

		st_n.size = st_o.size;

		for (orcus_int i = 0; i < st_o.size; i++) {
			orcus_increment_variable_refrence_count(st_o.keys + i);
			orcus_increment_variable_refrence_count(st_o.values + i);
			st_n.keys[i] = st_o.keys[i];
			st_n.values[i] = st_o.values[i];
		}

		newtab->data[t] = st_n;
	}
	newtab->references = 1;

	new_tab_var.type = ORCUS_TYPE_TABLE;
	new_tab_var.data.p = newtab;
	orcus_pushstack(M, &new_tab_var);

	orcus_release_var(&tab_var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

void orcus_lib_table_flush(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //basically the same as array.flush
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;

	orcus_variable tab_var;
	orcus_popstack(M, &tab_var);
	if (tab_var.type != ORCUS_TYPE_TABLE) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &tab_var, ORCUS_TYPE_TABLE, 1);
		orcus_release_var(&tab_var);
		return;
	}
	orcus_table* tab = (orcus_table*)tab_var.data.p;
	for (uint8_t t = 0; t < ORCUS_NUMBER_OF_TYPES; t++) {
		orcus_subtable st = tab->data[t];
		if (!st.size)continue;
		orcus_int subtracted_size = 0;
		for (orcus_int i = 0; i < st.size; i++) { //for each value
			if (!st.values[i].type) { //that is nil
				for (orcus_int i2 = (st.size- subtracted_size - 1); i2 > i; i2--) { //look through each value, start high
					if (st.values[i2].type) { //then go lower to find a non-nil value
						st.values[i] = st.values[i2];
						orcus_decrement_variable_refrence_count(st.keys+i);  //if we do, place the value at our position, and free our key
						orcus_release_var(st.keys+i);
						st.keys[i] = st.keys[i2];
						break;
					}
				}
				subtracted_size++; //either way increment the counter.
			}
		}
		st.size -= subtracted_size;
		void* nptr=realloc(st.keys, sizeof(orcus_variable) * st.size);
		if (!nptr && st.size) { //we're decreasing the size (or keeping the same) so this should theoretically never fail. but still. be safe.
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			orcus_release_var(&tab_var);
			return;
		}
		st.keys = (orcus_variable*)nptr;
		nptr = realloc(st.values, sizeof(orcus_variable) * st.size);
		if (!nptr && st.size) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			orcus_release_var(&tab_var);
			return;
		}
		st.values = (orcus_variable*)nptr;
		tab->data[t] = st;
	}

	orcus_release_var(&tab_var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
}


void orcus_lib_table_getkeys(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //array of all keys
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_array* arr = orcus_newarray();
	if (!arr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable tab_var;
	orcus_popstack(M, &tab_var);
	if (tab_var.type != ORCUS_TYPE_TABLE) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &tab_var, ORCUS_TYPE_TABLE, 1);
		orcus_release_var(&tab_var);
		return;
	}
	orcus_int counter = 0;
	orcus_table* tab = (orcus_table*)tab_var.data.p;
	for (uint8_t t = 0; t < ORCUS_NUMBER_OF_TYPES; t++) {
		orcus_subtable st = tab->data[t];
		if (!st.size)continue;
		for (orcus_int i = 0; i < st.size; i++) { //for each value
			if (!orcus_setarray(arr, st.keys + i, counter)) {
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				orcus_destroyarray(arr);
				orcus_release_var(&tab_var);
				return;
			}
			counter++;
		}
	}

	orcus_release_var(&tab_var);

	orcus_variable out;
	out.type = ORCUS_TYPE_ARRAY;
	out.data.p = arr;
	orcus_pushstack(M, &out);
	if (args_out > 1) {
		out.type = ORCUS_TYPE_INT;
		out.data.i = counter;
		orcus_pushstack(M, &out);
	}
	

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,2);
}

void orcus_lib_table_getvalues(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //array of all values
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_array* arr = orcus_newarray();
	if (!arr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable tab_var;
	orcus_popstack(M, &tab_var);
	if (tab_var.type != ORCUS_TYPE_TABLE) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &tab_var, ORCUS_TYPE_TABLE, 1);
		orcus_release_var(&tab_var);
		return;
	}
	orcus_int counter = 0;
	orcus_table* tab = (orcus_table*)tab_var.data.p;
	for (uint8_t t = 0; t < ORCUS_NUMBER_OF_TYPES; t++) {
		orcus_subtable st = tab->data[t];
		if (!st.size)continue;
		for (orcus_int i = 0; i < st.size; i++) { //for each value
			if (!orcus_setarray(arr, st.values + i, counter)) {
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				orcus_destroyarray(arr);
				orcus_release_var(&tab_var);
				return;
			}
			counter++;
		}
	}

	orcus_release_var(&tab_var);

	orcus_variable out;
	out.type = ORCUS_TYPE_ARRAY;
	out.data.p = arr;
	orcus_pushstack(M, &out);
	if (args_out > 1) {
		out.type = ORCUS_TYPE_INT;
		out.data.i = counter;
		orcus_pushstack(M, &out);
	}


	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 2);
}


void orcus_lib_table_haskey(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //if has a key
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable key_var;
	orcus_popstack(M, &key_var);

	orcus_variable tab_var;
	orcus_popstack(M, &tab_var);

	orcus_variable out;

	orcus_table* tab = (orcus_table*)tab_var.data.p;
	for (uint8_t t = 0; t < ORCUS_NUMBER_OF_TYPES; t++) {
		orcus_subtable st = tab->data[t];
		if (!st.size)continue;
		for (orcus_int i = 0; i < st.size; i++) {
			if (orcus_vars_equal(st.keys + i, &key_var)) {
				out.type = ORCUS_TYPE_BOOL;
				out.data.i = 1;
				orcus_pushstack(M, &out);
				if (args_out > 1) {
					orcus_pushstack(M, st.values+i);
				}
				return;
			}
		}
	}

	out.type = ORCUS_TYPE_BOOL;
	out.data.i = 0;
	orcus_pushstack(M, &out);
	if (args_out > 1) {
		out.type = ORCUS_TYPE_NIL;
		orcus_pushstack(M, &out);
	}

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 2);
}


void orcus_lib_table_hasvalue(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //if has a value
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable key_var;
	orcus_popstack(M, &key_var);

	orcus_variable tab_var;
	orcus_popstack(M, &tab_var);

	orcus_variable out;

	orcus_table* tab = (orcus_table*)tab_var.data.p;
	for (uint8_t t = 0; t < ORCUS_NUMBER_OF_TYPES; t++) {
		orcus_subtable st = tab->data[t];
		if (!st.size)continue;
		for (orcus_int i = 0; i < st.size; i++) {
			if (orcus_vars_equal(st.values + i, &key_var)) {
				out.type = ORCUS_TYPE_BOOL;
				out.data.i = 1;
				orcus_pushstack(M, &out);
				if (args_out > 1) {
					orcus_pushstack(M, st.keys + i);
				}
				return;
			}
		}
	}

	out.type = ORCUS_TYPE_BOOL;
	out.data.i = 0;
	orcus_pushstack(M, &out);
	if (args_out > 1) {
		out.type = ORCUS_TYPE_NIL;
		orcus_pushstack(M, &out);
	}

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 2);
}

void orcus_lib_table_keyswithvalue(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //all keys with a value
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_array* arr = orcus_newarray();
	if (!arr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable key_var;
	orcus_popstack(M, &key_var);

	orcus_variable tab_var;
	orcus_popstack(M, &tab_var);

	orcus_variable out;
	orcus_int elements = 0;
	orcus_table* tab = (orcus_table*)tab_var.data.p;
	for (uint8_t t = 0; t < ORCUS_NUMBER_OF_TYPES; t++) {
		orcus_subtable st = tab->data[t];
		if (!st.size)continue;
		for (orcus_int i = 0; i < st.size; i++) {
			if (orcus_vars_equal(st.values + i, &key_var)) {
				orcus_setarray(arr, st.keys + i, elements);
				elements++;
			}
		}
	}

	out.type = ORCUS_TYPE_ARRAY;
	out.data.p = arr;
	orcus_pushstack(M, &out);
	if (args_out > 1) {
		out.type = ORCUS_TYPE_INT;
		out.data.i = elements;
		orcus_pushstack(M, &out);
	}

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 2);
}
