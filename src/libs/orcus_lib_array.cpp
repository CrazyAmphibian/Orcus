#include "orcus_lib_array.hpp"
#include"../orcus.hpp"
#include"../orcus_error.hpp"
#include "malloc.h"
#include <stdlib.h>
#include <math.h>

#include <stdio.h>

void orcus_lib_array_flush(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //discards nil values.
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M,args_in,1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}

	orcus_variable arr_var;
	orcus_popstack(M,&arr_var);
	if (arr_var.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &arr_var, ORCUS_TYPE_ARRAY, 1);
		orcus_release_var(&arr_var);
		return;
	}


	orcus_array* arr = (orcus_array*)arr_var.data.p;


	orcus_array* newarr=orcus_newarray();
	if (!newarr) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}

	orcus_int len = 0;
	orcus_int len_neg = -1;
	
	//iterate through the array and push each non-nil variable into a new table
	if (arr->values) {
#ifdef ORCUS_64BIT
		//this can't be the best way to do it. i mean... just look at this piece of shit.
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1>>1; i1++) { //bitshift right once because we are ignoring negative values, and those start with 1
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
								orcus_variable var = st5[i6];
								if (var.type) {
									orcus_setarray(newarr, st5+i6, len);
									len++;
									st5[i6].type=ORCUS_TYPE_NIL; //set to nil so that the var isn't freed later
								}
							}
						}
					}
				}
			}
		}
		for (int i1 = ORCUS_SIZE_SUBARRAY_1-1; i1 > (ORCUS_SIZE_SUBARRAY_1-1) >> 1; i1--) { //now do the negative values. these count backwards
			orcus_variable***** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = ORCUS_SIZE_SUBARRAY_2; i2 >=0 ; i2--) {
				orcus_variable**** const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = ORCUS_SIZE_SUBARRAY_3; i3 >= 0; i3--) {
					orcus_variable*** const st3 = st2[i3];
					if (!st3)continue;
					for (int i4 = ORCUS_SIZE_SUBARRAY_4; i4 >= 0; i4--) {
						orcus_variable** const st4 = st3[i4];
						if (!st4)continue;
						for (int i5 = ORCUS_SIZE_SUBARRAY_5; i5 >= 0; i5--) {
							orcus_variable* const st5 = st4[i5];
							if (!st5)continue;
							for (int i6 = ORCUS_SIZE_SUBARRAY_6; i6 >= 0; i6--) {
								orcus_variable var = st5[i6];
								if (var.type) {
									orcus_setarray(newarr, st5+i6, len_neg);
									len_neg--;
									st5[i6].type=ORCUS_TYPE_NIL; //set to nil so that the var isn't freed later
								}
							}
						}
					}
				}
			}
		}
#else
		//it's less shit here but still not great.
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1>>1; i1++) { //bitshift right once because we are ignoring negative values, and those start with 1
			orcus_variable** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable* const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					orcus_variable var = st2[i3];
					if (var.type) {
						orcus_setarray(newarr, st2+i3, len);
						len++;
						st2[i3].type = ORCUS_TYPE_NIL; //set to nil so that the var isn't freed later
					}
				}
			}
		}
		for (int i1 = ORCUS_SIZE_SUBARRAY_1 - 1; i1 > (ORCUS_SIZE_SUBARRAY_1 - 1) >> 1; i1--) { //now do the negative values. these count backwards
			orcus_variable** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = ORCUS_SIZE_SUBARRAY_2; i2 >= 0; i2--) {
				orcus_variable* const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = ORCUS_SIZE_SUBARRAY_3; i3 >= 0; i3--) {
					orcus_variable var = st2[i3];
					if (var.type) {
						orcus_setarray(newarr, st2 + i3, len);
						len_neg--;
						st2[i3].type = ORCUS_TYPE_NIL; //set to nil so that the var isn't freed later
					}
				}
			}
		}
#endif
	}

	orcus_array intermediate = *newarr; //swap the new array data with the old array
	newarr->values = arr->values;
	arr->values = intermediate.values;
	orcus_destroyarray(newarr); //then free the array with the old data in it.

	orcus_release_var(&arr_var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
}


void orcus_lib_array_copy(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //creates a copy of all values. not recursive. arrays are references so this will make a new one.
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (args_out < 1) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable arr_var;
	orcus_popstack(M,&arr_var);
	if (arr_var.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &arr_var, ORCUS_TYPE_ARRAY, 1);
		orcus_release_var(&arr_var);
		return;
	}

	orcus_array* arr1 = (orcus_array*)arr_var.data.p;
	orcus_array* arr2 = orcus_newarray();
	if (!arr2) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		orcus_release_var(&arr_var);
		return;
	}



	if (arr1->values) {
#ifdef ORCUS_64BIT
		arr2->values = (orcus_variable******)calloc(ORCUS_SIZE_SUBARRAY_1, sizeof(void*));
		if (!arr2->values) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable***** const st1_1 = arr1->values[i1];
			if (!st1_1)continue;
			orcus_variable***** st1_2 = (orcus_variable*****)calloc(ORCUS_SIZE_SUBARRAY_2,sizeof(orcus_variable****));
			if (!st1_2) {
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			arr2->values[i1] = st1_2;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable**** const st2_1 = st1_1[i2];
				if (!st2_1)continue;
				orcus_variable**** st2_2 = (orcus_variable****)calloc(ORCUS_SIZE_SUBARRAY_3, sizeof(orcus_variable***));
				if (!st2_2) {
					orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
					return;
				}
				st1_2[i2] = st2_2;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_4; i3++) {
					orcus_variable*** const st3_1 = st2_1[i3];
					if (!st3_1)continue;
					orcus_variable*** st3_2 = (orcus_variable***)calloc(ORCUS_SIZE_SUBARRAY_4, sizeof(orcus_variable**));
					if (!st3_2) {
						orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
						return;
					}
					st2_2[i3] = st3_2;
					for (int i4 = 0; i4 < ORCUS_SIZE_SUBARRAY_4; i4++) {
						orcus_variable** const st4_1 = st3_1[i4];
						if (!st4_1)continue;
						orcus_variable** st4_2 = (orcus_variable**)calloc(ORCUS_SIZE_SUBARRAY_5, sizeof(orcus_variable*));
						if (!st4_2) {
							orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
							return;
						}
						st3_2[i4] = st4_2;
						for (int i5 = 0; i5 < ORCUS_SIZE_SUBARRAY_5; i5++) {
							orcus_variable* const st5_1 = st4_1[i5];
							if (!st5_1)continue;
							orcus_variable* st5_2 = (orcus_variable*)calloc(ORCUS_SIZE_SUBARRAY_6, sizeof(orcus_variable));
							if (!st5_2) {
								orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
								return;
							}
							st4_2[i5] = st5_2;
							for (int i6 = 0; i6 < ORCUS_SIZE_SUBARRAY_6; i6++) {
								if (st5_1[i6].type)st5_2[i6]= st5_1[i6];
							}
						}
					}
				}
			}
		}
#else
		arr2->values = (orcus_variable***)calloc(ORCUS_SIZE_SUBARRAY_1, sizeof(void*));
		if (!arr2->values) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
			orcus_variable** const st1_1 = arr1->values[i1];
			if (!st1_1)continue;
			orcus_variable** st1_2 = (orcus_variable**)calloc(ORCUS_SIZE_SUBARRAY_2, sizeof(orcus_variable*));
			if (!st1_2) {
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			arr2->values[i1] = st1_2;
			for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
				orcus_variable* const st2_1 = st1_1[i2];
				if (!st2_1)continue;
				orcus_variable* st2_2 = (orcus_variable*)calloc(ORCUS_SIZE_SUBARRAY_3, sizeof(orcus_variable));
				if (!st2_2) {
					orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
					return;
				}
				st1_2[i2] = st2_2;
				for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
					if (st2_1[i3].type)st2_2[i3]=st2_1[i3];
				}
			}
		}
#endif
	}


	orcus_release_var(&arr_var);

	arr_var.type = ORCUS_TYPE_ARRAY;
	arr_var.data.p = arr2;
	orcus_pushstack(M, &arr_var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}



void orcus_lib_array_insert(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //add at index, shifts values forward.
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2,3)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}

	orcus_variable len_var;
	if (args_in > 2) {
		orcus_popstack(M,&len_var);
		if (len_var.type != ORCUS_TYPE_INT) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE, &len_var, ORCUS_TYPE_INT, 3);
			orcus_release_var(&len_var);
			return;
		}
	}
	else {
		len_var.type = ORCUS_TYPE_NIL;
	}
	orcus_variable var_to_ins;
	orcus_popstack(M,&var_to_ins);

	orcus_variable arr_var;
	orcus_popstack(M, &arr_var);
	if (arr_var.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &arr_var, ORCUS_TYPE_ARRAY, 1);
		orcus_release_var(&arr_var);
		return;
	}

	orcus_array* arr = (orcus_array*)arr_var.data.p;
	orcus_int cur_len=orcus_array_len(arr)+1;
	orcus_int placed_at = -1;
	if (len_var.type) { //if len, shift values ahead to make space.
		orcus_int target = len_var.data.i;
		for (orcus_int i = cur_len; i >= target; i--) {
			orcus_variable v;
			orcus_getarray(arr, i, &v);
			orcus_setarray(arr, &v, i + 1);
		}
		orcus_setarray(arr, &var_to_ins, target);
		placed_at = target;
	}
	else { //if len is not specified, add to the end.
		orcus_setarray(arr, &var_to_ins, cur_len);
		placed_at = cur_len;
	}

	orcus_release_var(&arr_var);

	if (args_out) {
		arr_var.data.i = placed_at;
		arr_var.type = ORCUS_TYPE_INT;
		orcus_pushstack(M, &arr_var);
	}

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}

void orcus_lib_array_remove(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //gets rid of a value and shifts ones after down.
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}

	orcus_variable len_var;
	orcus_popstack(M, &len_var);
	if (len_var.type != ORCUS_TYPE_INT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE, &len_var, ORCUS_TYPE_INT, 2);
		orcus_release_var(&len_var);
		return;
	}

	orcus_variable arr_var;
	orcus_popstack(M, &arr_var);
	if (arr_var.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &arr_var, ORCUS_TYPE_ARRAY, 1);
		orcus_release_var(&arr_var);
		return;
	}

	orcus_int target = len_var.data.i;
	orcus_array* arr = (orcus_array*)arr_var.data.p;
	orcus_int cur_len = orcus_array_len(arr);


	orcus_variable v;
	orcus_getarray(arr, target, &v);
	orcus_increment_variable_refrence_count(&v);//protect variable from freeing
	orcus_variable inter;
	for (orcus_int i = target; i <= cur_len; i++) {
		orcus_getarray(arr, i + 1, &inter);
		orcus_setarray(arr, &inter, i);
	}
	orcus_decrement_variable_refrence_count(&v);
	if (args_out) {
		orcus_pushstack(M, &v);
	}
	else {
		orcus_release_var(&v);
	}


	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}


void orcus_lib_array_swap(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //duh.
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 3)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}

	orcus_variable pos_var1;
	orcus_popstack(M, &pos_var1);
	if (pos_var1.type != ORCUS_TYPE_INT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE, &pos_var1, ORCUS_TYPE_INT, 3);
		return;
	}

	orcus_variable pos_var2;
	orcus_popstack(M, &pos_var2);
	if (pos_var2.type != ORCUS_TYPE_INT) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE, &pos_var2, ORCUS_TYPE_INT, 2);
		return;
	}

	orcus_variable arr_var;
	orcus_popstack(M, &arr_var);
	if (arr_var.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &arr_var, ORCUS_TYPE_ARRAY, 1);
		return;
	}

	orcus_array* ar = (orcus_array*)arr_var.data.p;
	orcus_int p1 = pos_var1.data.i;
	orcus_int p2 = pos_var2.data.i;

	orcus_variable tv;
	orcus_getarray(ar, p1,&tv);
	orcus_variable tv2;
	orcus_getarray(ar, p2, &tv2);
	orcus_increment_variable_refrence_count(&tv);
	orcus_increment_variable_refrence_count(&tv2); //protect freeing
	orcus_setarray(ar, &tv2, p1);
	orcus_setarray(ar, &tv,p2);
	orcus_decrement_variable_refrence_count(&tv);
	orcus_decrement_variable_refrence_count(&tv2);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
}




orcus_function* SORTING_ORCUS_FUNCTION = nullptr;
orcus_state* SORTING_ORCUS_STATE = nullptr;

int orcus_sort_use_orcus_function(const void* a, const void* b) {
	orcus_variable var_a = *(orcus_variable*)a;
	orcus_variable var_b = *(orcus_variable*)b;

	orcus_state* M= orcus_get_child_state(SORTING_ORCUS_STATE);
	if (!M)return 0;
	//M->bytecode.instructions = SORTING_ORCUS_FUNCTION->instructions;
	//M->bytecode.numberofinstructions = SORTING_ORCUS_FUNCTION->numberofinstructions;
	orcus_function* oldf = M->bytecode;
	M->bytecode = SORTING_ORCUS_FUNCTION;
	orcus_pushstack(M,&var_b);
	orcus_pushstack(M,&var_a);
	while (orcus_stepstate(M));
	orcus_variable var_o;
	orcus_popstack(M,&var_o);
	M->bytecode = oldf;
	//orcus_destroystate(M);
	orcus_clearstate(M);
	int out = (int)orcus_checkint(&var_o);
	orcus_release_var(&var_o);
	return out;
}


int orcus_sort_function_greater_to_lesser(const void* a, const void* b) {
	orcus_variable var_a = *(orcus_variable*)a;
	orcus_variable var_b = *(orcus_variable*)b;
	if (var_a.type != ORCUS_TYPE_INT && var_a.type != ORCUS_TYPE_FLOAT) {
		return 0;
	}
	if (var_b.type != ORCUS_TYPE_INT && var_b.type != ORCUS_TYPE_FLOAT) {
		return 0;
	}

	if (var_a.type == ORCUS_TYPE_INT) {
		if (var_b.type == ORCUS_TYPE_INT) {
			return (var_a.data.i < var_b.data.i) - (var_a.data.i > var_b.data.i);
		}
		else {
			return (var_a.data.i < var_b.data.f) - (var_a.data.i > var_b.data.f);
		}
	}
	else {
		if (var_b.type == ORCUS_TYPE_INT) {
			return (var_a.data.f < var_b.data.i) - (var_a.data.f > var_b.data.i);
		}
		else {
			return (var_a.data.f < var_b.data.f) - (var_a.data.f > var_b.data.f);
		}
	}
	return 0;
}

int orcus_sort_function_lesser_to_greater(const void* a, const void* b) {
	orcus_variable var_a = *(orcus_variable*)a;
	orcus_variable var_b = *(orcus_variable*)b;

	if (var_a.type != ORCUS_TYPE_INT && var_a.type != ORCUS_TYPE_FLOAT) {
		return 0;
	}
	if (var_b.type != ORCUS_TYPE_INT && var_b.type != ORCUS_TYPE_FLOAT) {
		return 0;
	}

	if (var_a.type == ORCUS_TYPE_INT) {
		if (var_b.type == ORCUS_TYPE_INT) {
			return (var_a.data.i > var_b.data.i) - (var_a.data.i < var_b.data.i);
		}
		else {
			return (var_a.data.i > var_b.data.f) - (var_a.data.i < var_b.data.f);
		}
	}
	else {
		if (var_b.type == ORCUS_TYPE_INT) {
			return (var_a.data.f > var_b.data.i) - (var_a.data.f < var_b.data.i);
		}
		else {
			return (var_a.data.f > var_b.data.f) - (var_a.data.f < var_b.data.f);
		}
	}
	return 0;
}

int orcus_sort_function_greater_to_lesser_absolute(const void* a, const void* b) {
	orcus_variable var_a = *(orcus_variable*)a;
	orcus_variable var_b = *(orcus_variable*)b;

	if (var_a.type != ORCUS_TYPE_INT && var_a.type != ORCUS_TYPE_FLOAT) {
		return 0;
	}
	if (var_b.type != ORCUS_TYPE_INT && var_b.type != ORCUS_TYPE_FLOAT) {
		return 0;
	}

	if (var_a.type == ORCUS_TYPE_INT) {
		if (var_b.type == ORCUS_TYPE_INT) {
			return (abs(var_a.data.i) < abs(var_b.data.i)) - (abs(var_a.data.i) > abs(var_b.data.i));
		}
		else {
			return (abs(var_a.data.i) < fabs(var_b.data.f)) - (abs(var_a.data.i) > fabs(var_b.data.f));
		}
	}
	else {
		if (var_b.type == ORCUS_TYPE_INT) {
			return (fabs(var_a.data.f) < abs(var_b.data.i)) - (fabs(var_a.data.f) > abs(var_b.data.i));
		}
		else {
			return (fabs(var_a.data.f) < fabs(var_b.data.f)) - (fabs(var_a.data.f) > fabs(var_b.data.f));
		}
	}
	return 0;
}




int orcus_sort_function_lesser_to_greater_absolute(const void* a, const void* b) {
	orcus_variable var_a = *(orcus_variable*)a;
	orcus_variable var_b = *(orcus_variable*)b;

	if (var_a.type != ORCUS_TYPE_INT && var_a.type != ORCUS_TYPE_FLOAT) {
		return 0;
	}
	if (var_b.type != ORCUS_TYPE_INT && var_b.type != ORCUS_TYPE_FLOAT) {
		return 0;
	}

	if (var_a.type == ORCUS_TYPE_INT) {
		if (var_b.type == ORCUS_TYPE_INT) {
			return (abs(var_a.data.i) > abs(var_b.data.i)) - (abs(var_a.data.i) < abs(var_b.data.i));
		}
		else {
			return (abs(var_a.data.i) > fabs(var_b.data.f)) - (abs(var_a.data.i) < fabs(var_b.data.f));
		}
	}
	else {
		if (var_b.type == ORCUS_TYPE_INT) {
			return (fabs(var_a.data.f) > abs(var_b.data.i)) - (fabs(var_a.data.f) < abs(var_b.data.i));
		}
		else {
			return (fabs(var_a.data.f) > fabs(var_b.data.f)) - (fabs(var_a.data.f) < fabs(var_b.data.f));
		}
	}
	return 0;
}


int orcus_sort_function_alphabet_az(const void* a, const void* b) {
	orcus_variable var_a = *(orcus_variable*)a;
	orcus_variable var_b = *(orcus_variable*)b;

	if (var_a.type != ORCUS_TYPE_STRING) {
		return 0;
	}
	if (var_b.type != ORCUS_TYPE_STRING) {
		return 0;
	}

	orcus_string* str_a = (orcus_string*)var_a.data.p;
	orcus_string* str_b = (orcus_string*)var_b.data.p;

	orcus_int s_a = str_a->size;
	orcus_int s_b = str_b->size;

	orcus_int size = s_a > s_b ? s_a : s_b;

	const unsigned char* string_a = (unsigned char*)str_a->ptr;
	const unsigned char* string_b = (unsigned char*)str_b->ptr;

	for (orcus_int c = 0; c < size; c++) {
		short a = c >= s_a ? -1 : string_a[c];
		short b = c >= s_b ? -1 : string_b[c];
		if (a > b) {
			return 1;
		}
		else if (b > a) {
			return -1;
		}
	}

	return 0;
}

int orcus_sort_function_alphabet_za(const void* a, const void* b) {
	orcus_variable var_a = *(orcus_variable*)a;
	orcus_variable var_b = *(orcus_variable*)b;

	if (var_a.type != ORCUS_TYPE_STRING) {
		return 0;
	}
	if (var_b.type != ORCUS_TYPE_STRING) {
		return 0;
	}

	orcus_string* str_a = (orcus_string*)var_a.data.p;
	orcus_string* str_b = (orcus_string*)var_b.data.p;

	orcus_int s_a = str_a->size;
	orcus_int s_b = str_b->size;

	orcus_int size = s_a > s_b ? s_a : s_b;

	const unsigned char* string_a = (unsigned char*)str_a->ptr;
	const unsigned char* string_b = (unsigned char*)str_b->ptr;

	for (orcus_int c = 0; c < size; c++) {
		short a = c >= s_a ? -1 : string_a[c];
		short b = c >= s_b ? -1 : string_b[c];
		if (a > b) {
			return -1;
		}
		else if (b > a) {
			return 1;
		}
	}

	return 0;
}

void orcus_lib_array_sort(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2);

	orcus_variable var_func;
	orcus_popstack(M,&var_func);
	if (var_func.type != ORCUS_TYPE_INT && var_func.type != ORCUS_TYPE_CFUNC) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_func, ORCUS_TYPE_FUNCTION, 2);
		orcus_release_var(&var_func);
		return;
	}

	orcus_variable var_array;
	orcus_popstack(M,&var_array);
	if (var_array.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_array, ORCUS_TYPE_ARRAY, 1);
		orcus_release_var(&var_array);
		return;
	}

	orcus_array* arr = (orcus_array*)var_array.data.p;
	orcus_int arr_size=orcus_array_len(arr)+1;
	orcus_variable* tlist=(orcus_variable*)malloc(sizeof(orcus_variable)*arr_size);
	if (!tlist) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	for (orcus_int i = 0; i < arr_size; i++) { //this could probably be optimized with a memcpy. oh well.
		orcus_getarray(arr, i, tlist+i);
	}

	if (var_func.type == ORCUS_TYPE_INT) {
		switch(var_func.data.i) {
			case orcus_sort_greater_to_lesser:
				qsort(tlist, arr_size, sizeof(orcus_variable), orcus_sort_function_greater_to_lesser);
				break;
			case orcus_sort_lesser_to_greater:
				qsort(tlist, arr_size, sizeof(orcus_variable), orcus_sort_function_lesser_to_greater);
				break;
			case orcus_sort_greater_to_lesser_absolute:
				qsort(tlist, arr_size, sizeof(orcus_variable), orcus_sort_function_greater_to_lesser_absolute);
				break;
			case orcus_sort_lesser_to_greater_absolute:
				qsort(tlist, arr_size, sizeof(orcus_variable), orcus_sort_function_lesser_to_greater_absolute);
				break;
			case orcus_sort_alphabet_az:
				qsort(tlist, arr_size, sizeof(orcus_variable), orcus_sort_function_alphabet_az);
				break;
			case orcus_sort_alphabet_za:
				qsort(tlist, arr_size, sizeof(orcus_variable), orcus_sort_function_alphabet_za);
				break;
		}
	}
	else {
		SORTING_ORCUS_FUNCTION = (orcus_function*)var_func.data.p;
		SORTING_ORCUS_STATE = M;
		qsort(tlist, arr_size, sizeof(orcus_variable), orcus_sort_use_orcus_function); //surely this will work.
	}

	for (orcus_int i = 0; i < arr_size; i++) { //see above.
		orcus_cleararrayindex(arr, i);
		orcus_setarray(arr, tlist+i,i);
	}
	free(tlist);
	
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 0);
}



void orcus_lib_array_concat(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	orcus_variable var_str;
	orcus_popstack(M,&var_str);
	if (var_str.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_str, ORCUS_TYPE_STRING, 2);
		return;
	}

	orcus_variable var_array;
	orcus_popstack(M,&var_array);
	if (var_array.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &var_array, ORCUS_TYPE_ARRAY, 1);
		return;
	}
	if (!args_out) { 
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_string* out_str=orcus_cstring_to_mstring((char*)"",0);

	orcus_int limit=orcus_array_len((orcus_array*)var_array.data.p)+1;
	for (orcus_int i = 0; i < limit; i++) {
		orcus_variable var;
		orcus_getarray((orcus_array*)var_array.data.p, i, &var);
		if (var.type) {
			orcus_string* s = orcus_tostring(&var);
			if (s) {
				orcus_mstrings_append(out_str, s);
				orcus_mstring_delete(s);
			}
		}
		

		if(i!=limit-1)orcus_mstrings_append(out_str, (orcus_string*)var_str.data.p);
	}


	orcus_release_var(&var_array);
	orcus_release_var(&var_str);
	var_str.type = ORCUS_TYPE_STRING;
	var_str.data.p = out_str;

	orcus_pushstack(M, &var_str);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}