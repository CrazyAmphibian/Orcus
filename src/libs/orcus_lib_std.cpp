#include"../orcus.hpp"
#include"../orcus_error.hpp"
#include"../orcus_bytecode.hpp"

#include<malloc.h>
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include <limits.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#include <time.h>
#endif

//throws stuff into stdout. adds a newline at the end, seperates with a tab. designed to be variadic.
void orcus_lib_std_print(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {

	orcus_variable* vartable = (orcus_variable*)malloc(sizeof(orcus_variable) * args_in);
	if (vartable == nullptr && args_in) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}

	for (orcus_int a = args_in-1; a >= 0; a--) {
		orcus_popstack(M, vartable+a);
	}

	for (orcus_int a = 0; a < args_in; a++) {

		orcus_string* mstrv = orcus_tostring(vartable+a);
		orcus_release_var(vartable+a);
		if (mstrv) {
			fwrite(mstrv->ptr, 1, mstrv->size, stdout);
			putchar('\t');
			fflush(stdout);

			orcus_mstring_delete(mstrv);
		}
		
		
	}
	free(vartable);
	putchar('\n');
	fflush(stdout);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
}


//traverses a list-like variable entry-by-entry. useful for serializing tables, most likley. arg 1 is the thing, and arg 2 is the function.
void orcus_lib_std_iterate(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if(ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2))return;

	orcus_variable function;
	orcus_popstack(M,&function);
	orcus_variable listlike;
	orcus_popstack(M,&listlike);


	if (function.type != ORCUS_TYPE_CFUNC && function.type != ORCUS_TYPE_FUNCTION) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_FUNCTION_VARIABLEPROVIDED, &function, 2);
		return;
	}
	if (listlike.type != ORCUS_TYPE_TABLE && listlike.type != ORCUS_TYPE_ARRAY) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_STORAGETYPE_VARIABLEPROVIDED, &listlike, 1);
		return;
	}

	orcus_state* SubM = orcus_get_child_state(M);
	if (!SubM) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	orcus_function* previous = SubM->bytecode;

	orcus_increment_variable_refrence_count(&function);
	orcus_increment_variable_refrence_count(&listlike);
	
	if (function.type == ORCUS_TYPE_FUNCTION) {
		SubM->bytecode = ((orcus_function*)function.data.p);
	}

	if (listlike.type == ORCUS_TYPE_ARRAY) {
		orcus_array* arr = (orcus_array*)listlike.data.p;
		orcus_int srefs = arr->references;


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
									orcus_variable var = st5[i6];
									const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3, i4, i5, i6);
#else
			for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
				orcus_variable** const st1 = arr->values[i1];
				if (!st1)continue;
				for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
					orcus_variable* const st2 = st1[i2];
					if (!st2)continue;
					for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
						orcus_variable var = st2[i3];
						const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3);
#endif
						if (var.type) {
							orcus_variable idxvar;
							idxvar.data.i = index;
							idxvar.type = ORCUS_TYPE_INT;

							if (function.type == ORCUS_TYPE_CFUNC) {
								orcus_pushstack(SubM, &idxvar);
								orcus_pushstack(SubM, &var);
								orcus_pushstack(SubM, &listlike);
								((orcus_cfunc)function.data.p)(SubM, 3, 1);

								orcus_variable o;
								orcus_pullstack(SubM,&o);
								if (orcus_checkbool(&o) || SubM->errorcode) { //soft break from all loops
#ifdef ORCUS_64BIT
									i1 = i2 = i3 = i4 = i5 = i6 = INT_MAX-1;
#else
									i1 = i2 = i3 = INT_MAX-1;
#endif
								}
								orcus_release_var(&o);
								orcus_clearstate(SubM);
							}
							else { //M functions get args in the reverse order. confusing, but it works.
								orcus_pushstack(SubM, &listlike);
								orcus_pushstack(SubM, &var);
								orcus_pushstack(SubM, &idxvar);
								while (orcus_stepstate(SubM));

								orcus_variable o;
								orcus_pullstack(SubM, &o);
								if (orcus_checkbool(&o) || SubM->errorcode) { //soft break from all loops
#ifdef ORCUS_64BIT
									i1 = i2 = i3 = i4 = i5 = i6 = INT_MAX-1;
#else
									i1 = i2 = i3 = INT_MAX-1;
#endif
								}
								orcus_release_var(&o);
								orcus_clearstate(SubM);
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

		//arr->references = srefs;


	}
	else if (listlike.type == ORCUS_TYPE_TABLE) {
		orcus_table* tab = (orcus_table*)listlike.data.p;
		orcus_int srefs = tab->references;

		for (uint8_t t = 0; t < ORCUS_NUMBER_OF_TYPES; t++) {
			orcus_subtable subt = tab->data[t];
			for (orcus_int i = 0; i < subt.size; i++) {
				orcus_variable k = subt.keys[i];
				orcus_variable v = subt.values[i];
				
				if (function.type == ORCUS_TYPE_CFUNC) {
					orcus_pushstack(SubM, &k);
					orcus_pushstack(SubM, &v);
					orcus_pushstack(SubM, &listlike);
					((orcus_cfunc)function.data.p)(SubM, 3, 1);
					orcus_variable o;
					orcus_pullstack(SubM, &o);
					if (orcus_checkbool(&o)) {
						t = ORCUS_NUMBER_OF_TYPES; //soft break from both loops.
						i = subt.size;
					}
					orcus_release_var(&o);
					
				}
				else {
					orcus_pushstack(SubM, &listlike);
					orcus_pushstack(SubM, &v);
					orcus_pushstack(SubM, &k);
					
					while (orcus_stepstate(SubM));
					SubM->programcounter = 0; //reset position to start so we can run it again if it's a M func.

					orcus_variable o;
					orcus_pullstack(SubM, &o);
					if (orcus_checkbool(&o)) {
						t = ORCUS_NUMBER_OF_TYPES;
						i = subt.size;
					}
					orcus_release_var(&o);
				}
				if (SubM->errorcode) {
					t = ORCUS_NUMBER_OF_TYPES;
					i = subt.size;
				};
				orcus_clearstate(SubM);
			}
		}
		//tab->references = srefs;


	}

	SubM->bytecode= previous;
	orcus_clearstate(SubM);

	orcus_decrement_variable_refrence_count(&function);
	orcus_decrement_variable_refrence_count(&listlike);

	orcus_release_var(&function);
	orcus_release_var(&listlike);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
}


//executes a function whose enviroment is a supplied table. variadic, as it will supply additional args to the function
/*
TODO:
get return args
*/
void orcus_lib_std_restricted_call(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (args_in < 2) {
		orcus_raise_error_nonpointer(M,ORCUS_ERROR_NOT_ENOUGH_ARGS, args_in,2);
		return;
	}
	orcus_variable* argt = (orcus_variable*)malloc(sizeof(orcus_variable) * (args_in - 2));
	if (!argt && args_in>2) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}
	for (orcus_int a = args_in; a > 2; a--) {
		orcus_popstack(M, argt+ args_in - a);
	}

	orcus_variable tab;
	orcus_popstack(M,&tab);
	orcus_variable func;
	orcus_popstack(M,&func);

	if (tab.type != ORCUS_TYPE_TABLE) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &tab, ORCUS_TYPE_TABLE, 2);
		orcus_release_var(&tab);
		free(argt);
		return;
	}
	if (func.type != ORCUS_TYPE_CFUNC && func.type != ORCUS_TYPE_FUNCTION) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_FUNCTION_VARIABLEPROVIDED, &func, 1);
		orcus_release_var(&func);
		free(argt);
		return;
	}


	orcus_state* iso_M = orcus_get_child_state(M);//orcus_newstate();
	if (!iso_M) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		free(argt);
		return;
	}
	
	iso_M->errorcode = 0;
	//remove variables that link the enviroment
	orcus_table* oldenv = iso_M->enviroment;
	orcus_function* oldfunc = iso_M->bytecode;
	orcus_state* oldmaster = iso_M->masterstate;
	orcus_state* oldparent = iso_M->parentstate;
	iso_M->masterstate=iso_M;
	iso_M->parentstate = nullptr;

	iso_M->enviroment = (orcus_table*)tab.data.p;
	for (orcus_int i = 0; i < args_in - 2; i++) {
		orcus_pushstack(iso_M, argt+i);
	}
	if (func.type == ORCUS_TYPE_FUNCTION) {
		iso_M->bytecode = (orcus_function*)func.data.p;
	}

	orcus_variable out;
	out.type = ORCUS_TYPE_BOOL;
	out.data.i = 1;
	orcus_int args_sent_out = 0;
	if (func.type == ORCUS_TYPE_CFUNC) {
		args_sent_out -= iso_M->sizeofstack;
		((orcus_cfunc)func.data.p)(iso_M, args_in-2, args_out<0?-1:(args_out ? args_out-1 : 0) );
		args_sent_out += iso_M->sizeofstack;
		if (args_sent_out < 0)args_sent_out = 0;
	}
	else {
		while (orcus_stepstate(iso_M));
	}
	if (iso_M->errorcode)out.data.i = 0;
	iso_M->bytecode = oldfunc;

	if (args_out) {
		orcus_pushstack(M, &out);
	}

	if (out.data.i) {
		if (args_out > 0) {
			for (orcus_int i = 1; i < args_out; i++) {
				orcus_pullstack(iso_M, &out);
				orcus_pushstack(M, &out);
			}
		}
		else if(args_out<0){ //variadic mode
			while (iso_M->sizeofstack) {
				orcus_pullstack(iso_M, &out);
				orcus_pushstack(M, &out);
			}
		}
	}
	else {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
	}


	free(argt);

	//reset variables to original state.
	iso_M->enviroment=oldenv;
	iso_M->bytecode = oldfunc;
	iso_M->masterstate = oldmaster;
	iso_M->parentstate = oldparent;

	orcus_clearstate(iso_M);

	orcus_release_var(&func);
	orcus_release_var(&tab);

	//ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M,args_out, 1);
}


inline bool check_pointer_dumped(void* ptr, orcus_uint* num_pointers_covered, void*** pointers_covered) {
	for (orcus_uint i = 0; i < (*num_pointers_covered); i++) {
		if ((*pointers_covered)[i] == ptr) {
			return true;
		}
	}
	return false;
}

inline bool add_dump_pointer(void* ptr, orcus_uint* num_pointers_covered, void*** pointers_covered) {
	void* nptr = realloc(*pointers_covered, sizeof(void*) * ((*num_pointers_covered) + 1));
	if (!nptr)return false;
	*pointers_covered = (void**)nptr;

	(*pointers_covered)[*num_pointers_covered] = ptr;

	(*num_pointers_covered)++;
	return true;
}


orcus_string* orcus_stringify(orcus_rawdata data, uint8_t type,orcus_uint* num_pointers_covered,void*** pointers_covered) {
	orcus_string* str = nullptr;// orcus_cstring_to_mstring((char*)"", 0);


	orcus_string* temp = nullptr;
	orcus_string* temp2 = nullptr;
	char tout[256];
	for (int i = 0; i < 256; i++) {
		tout[i] = '\0';
	}
	int tint;

	switch (type)
	{
	case ORCUS_TYPE_NIL:
		orcus_cstring_to_mstring((char*)"nil", 3);
		break;
	case ORCUS_TYPE_INT:
		tint = snprintf(tout, sizeof(tout), "%zi", data.i);
		str = orcus_cstring_to_mstring(tout, strlen(tout));
		break;
	case ORCUS_TYPE_FLOAT:
#ifdef ORCUS_64BIT
		tint = snprintf(tout, sizeof(tout), "%.60g", data.f);
#else
		tint = snprintf(tout, sizeof(tout), "%.30g", data.f);
#endif
		str = orcus_cstring_to_mstring(tout, strlen(tout));
		break;
	case ORCUS_TYPE_BOOL:
		if (data.i) {
			str = orcus_cstring_to_mstring((char*)"true", 4);
		}
		else {
			str = orcus_cstring_to_mstring((char*)"false", 5);
		}
		break;
	case ORCUS_TYPE_TABLE:
		if (check_pointer_dumped(data.p, num_pointers_covered, pointers_covered)) {
			str = (orcus_string*)malloc(sizeof(orcus_string));
			if (str)memset(str, 0, sizeof(orcus_string));
		}
		else {
			add_dump_pointer(data.p, num_pointers_covered, pointers_covered);
			str = orcus_cstring_to_mstring((char*)"{", 1);
			{
				orcus_table* t = (orcus_table*)data.p;
				for (uint8_t i = 0; i < ORCUS_NUMBER_OF_TYPES; i++) {
					orcus_subtable st = t->data[i];
					for (orcus_int n = 0; n < st.size; n++) {
						orcus_string* key = orcus_stringify(st.keys[n].data, i, num_pointers_covered, pointers_covered);
						orcus_variable v = st.values[n];
						orcus_string* value = orcus_stringify(v.data, v.type, num_pointers_covered, pointers_covered);

						if (!key || !value) {
							if(key)orcus_mstring_delete(key);
							if(value)orcus_mstring_delete(value);
							continue;
						}

						orcus_mstrings_append(str, key);
						orcus_mstring_delete(key);

						orcus_mstring_addchars(str, (char*)"=", 1);

						orcus_mstrings_append(str, value);
						orcus_mstring_delete(value);

						orcus_mstring_addchars(str, (char*)",");

					}
				}
			}
			orcus_mstring_addchars(str, (char*)"}");
		}
		break;
	case ORCUS_TYPE_STRING:
		str = (orcus_string*)malloc(sizeof(orcus_string));
		if (str) {
			memset(str, 0, sizeof(orcus_string));
			orcus_string* cstr = (orcus_string*)data.p;
			orcus_int size_total_str = cstr->size;
			for (orcus_int i = 0; i < cstr->size; i++) {
				switch (cstr->ptr[i])
				{
				case '\\':
				case '\"':
					size_total_str++;
				}
			}
			str->ptr=(char*)malloc(sizeof(char)* (size_total_str+2));
			if (!str->ptr) {
				return nullptr;
			}
			str->size = size_total_str+2;

			str->ptr[0] = '\"';
			str->ptr[size_total_str+1] = '\"';

			if (str->ptr) {
				orcus_int traversed = 0;
				for (orcus_int i = 0; i < cstr->size; i++) { //can't use memcpy, we need to escape \ and ".
					switch (cstr->ptr[i])
					{
					case '\"':
					case '\\':
						str->ptr[traversed+1] = '\\';
						str->ptr[traversed+2] = cstr->ptr[i];
						traversed += 2;
						break;
					default:
						str->ptr[traversed+1] = cstr->ptr[i];
						traversed++;
						break;
					}
				}
			}
			else {
				free(str);
				str = nullptr;
			}


			


		}
		break;
	case ORCUS_TYPE_ARRAY:
		if (check_pointer_dumped(data.p, num_pointers_covered, pointers_covered)) {
			str = (orcus_string*)malloc(sizeof(orcus_string));
			if (str)memset(str, 0, sizeof(orcus_string));
		}
		else {
			add_dump_pointer(data.p, num_pointers_covered, pointers_covered);
			str = orcus_cstring_to_mstring((char*)"[", 1);
			{
				orcus_array* arr = (orcus_array*)data.p;

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
								if (var.type) {
									temp = orcus_stringify(var.data, var.type, num_pointers_covered, pointers_covered);
									if (!temp)continue;

									orcus_string* temp2 = orcus_stringify({ index }, ORCUS_TYPE_INT, num_pointers_covered, pointers_covered);
									orcus_mstrings_append(str, temp2);
									orcus_mstring_delete(temp2);

									orcus_mstring_addchars(str, (char*)"=", 1);

									orcus_mstrings_append(str, temp);
									orcus_mstring_delete(temp);

									orcus_mstring_addchars(str, (char*)",");
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
			}
			orcus_mstring_addchars(str, (char*)"]");
		}
		break;
	default:
		return nullptr;
		break;
	}
	return str;
}








//takes a variable and generates a string which represents that variable.
void orcus_lib_std_dump(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if(ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable vartodump;
	orcus_popstack(M,&vartodump);

	orcus_uint nptrs=0;
	void** ptrs = nullptr;
	orcus_string* dmp_str = orcus_stringify(vartodump.data, vartodump.type,&nptrs,&ptrs);
	if (!dmp_str) {
		dmp_str = (orcus_string*)malloc(sizeof(orcus_string));
		if (!dmp_str) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		dmp_str->ptr = nullptr;
		dmp_str->size = 0;
	}
	free(ptrs);

	orcus_release_var(&vartodump);
	vartodump.type = ORCUS_TYPE_STRING;
	vartodump.data.p = dmp_str;
	orcus_pushstack(M, &vartodump);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_std_compile(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if(ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable codestr;
	orcus_popstack(M,&codestr);
	if (codestr.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &codestr,ORCUS_TYPE_STRING, 1);
		return;
	}


	orcus_variable out;
	orcus_compile_mstring((orcus_string*)codestr.data.p, &out);
	orcus_release_var(&codestr);
	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, 1, 1);
}


void orcus_lib_std_type(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if(ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable var;
	orcus_popstack(M,&var);
	orcus_release_var(&var);
	var.data.i = var.type;
	var.type = ORCUS_TYPE_INT;
	orcus_pushstack(M, &var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}




void orcus_lib_std_tostring(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //pretty easy, actually. we already have a function.
	if(ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable i;
	orcus_popstack(M,&i);
	orcus_string* l = orcus_tostring(&i);
	orcus_release_var(&i); //we can just re-use the variable struct. saves time, probly
	i.type = ORCUS_TYPE_STRING;
	i.data.p = l;
	orcus_pushstack(M, &i);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_std_tonumber(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //bit more complicated.
	if(ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable i;
	orcus_popstack(M,&i);
	orcus_variable o;

	switch (i.type) {
	case ORCUS_TYPE_INT:
	case ORCUS_TYPE_FLOAT: //already numbers. easy.
		o.type = i.type;
		o.data = i.data;
		break;
	case ORCUS_TYPE_BOOL:
		o.type = ORCUS_TYPE_INT;
		o.data.i = i.data.i ? 1 : 0;
		break;
	case ORCUS_TYPE_STRING:
		{
		orcus_string* s = (orcus_string*)i.data.p;
		if (!s->size) {
			o.type = ORCUS_TYPE_NIL;
			o.data.i = 0;
			break;
		}
		char* c = orcus_mstring_to_cstring(s);
		char* e;
		orcus_int n = strtoll(c, &e, 0);
		if (*e == '\0') {
			o.type = ORCUS_TYPE_INT;
			o.data.i = n;
			free(c);
			break;
		}
		orcus_float f = strtod(c, &e);
		if (*e == '\0') {
			o.type = ORCUS_TYPE_FLOAT;
			o.data.f = f;
			free(c);
			break;
		}
		o.type = ORCUS_TYPE_NIL;
		o.data.i = 0;
		free(c);
		}
		break;
	default:
		o.type = ORCUS_TYPE_NIL;
		o.data.i = 0;
	}

	orcus_release_var(&i);
	orcus_pushstack(M, &o);


	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



void orcus_lib_std_dynamic_library_load(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //dangerous, hell yeah!
	if(ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;

	orcus_variable i;
	orcus_popstack(M,&i);

	if (i.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &i, ORCUS_TYPE_STRING, 1);
		return;
	}

	char* c=orcus_mstring_to_cstring((orcus_string*)i.data.p);

	orcus_release_var(&i);

	orcus_variable o;
	o.type = ORCUS_TYPE_BOOL;
	

#ifdef _WIN32
	HMODULE lib = LoadLibraryA(c);
	if (!lib) {
		o.data.i = 0;
	}
	else {
		FreeLibrary(lib);
		o.data.i = 1;
	}
#else
	void* lib = dlopen(c, RTLD_NOW);
	if (!lib) {
		o.data.i = 0;
	}
	else {
		dlclose(lib);
		o.data.i = 1;
	}
#endif

	free(c);
	

	if(args_out)orcus_pushstack(M, &o);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_std_toint(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if(ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_lib_std_tonumber(M, 1, 1); //we already have to number code. might as well use it.

	orcus_variable i;
	orcus_popstack(M,&i);

	switch (i.type) { //because we know it'll only be a float, int, or nil, we can only check for float for extra easy code.
	case ORCUS_TYPE_FLOAT:
		i.type = ORCUS_TYPE_INT;
		i.data.i = (orcus_int)i.data.f;
		orcus_pushstack(M, &i);
		break;
	default:
		orcus_pushstack(M, &i);
		break;
	}
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_std_tofloat(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //basically the same thing as the above.
	if(ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_lib_std_tonumber(M, 1, 1);

	orcus_variable i;
	orcus_popstack(M, &i);

	switch (i.type) {
	case ORCUS_TYPE_INT:
		i.type = ORCUS_TYPE_FLOAT;
		i.data.f = (orcus_float)i.data.i;
		orcus_pushstack(M, &i);
		break;
	default:
		orcus_pushstack(M, &i);
		break;
	}
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


enum deepcopy_returntypes :int {
	DEEPCOPY_SUCCES = 0,
	DEEPCOPY_MEMORY_ALLOCATION_ERROR = 1,
	DEEPCOPY_UNCOPYABLE_TYPE = 2,
};

inline bool var_can_be_deepcopied(const orcus_variable* var) {
	switch (var->type) {
	case ORCUS_TYPE_NIL:
	case ORCUS_TYPE_INT:
	case ORCUS_TYPE_FLOAT:
	case ORCUS_TYPE_BOOL:
	case ORCUS_TYPE_CFUNC:
	case ORCUS_TYPE_STRING:
	case ORCUS_TYPE_FUNCTION:
	case ORCUS_TYPE_ARRAY:
	case ORCUS_TYPE_TABLE:
		return true;
	default:
		return false;
	}
}

inline bool type_is_pointer(const orcus_variable* var) {
	switch (var->type) {
	case ORCUS_TYPE_NIL:
	case ORCUS_TYPE_INT:
	case ORCUS_TYPE_FLOAT:
	case ORCUS_TYPE_BOOL:
	case ORCUS_TYPE_CFUNC:
		return false;
	default:
		return true;
	};

}

inline void* get_converted_pointer(const void* in,orcus_uint* num_pointers_converted, void*** pointer_conversions_in, void*** pointer_conversions_out) {
	//printf("checking for self from %zu pointers\n",*num_pointers_converted);
	for (orcus_uint i = 0; i < (*num_pointers_converted); i++) {
		//printf("comparing %p to %p\n", (*pointer_conversions_in)[i],in);
		if ( (*pointer_conversions_in)[i] == in) {
			//printf("output convert pointer\n");
			return (*pointer_conversions_out)[i];
		}
	}
	return nullptr;
}

inline bool add_converted_pointer(void* originalp,void* newp, orcus_uint* num_pointers_converted, void*** pointer_conversions_in, void*** pointer_conversions_out) {
	void* nptr=realloc(*pointer_conversions_in, sizeof(void*) * ((*num_pointers_converted) + 1));
	if (!nptr)return false;
	*pointer_conversions_in = (void**)nptr;
	nptr = realloc(*pointer_conversions_out, sizeof(void*) * ((*num_pointers_converted) + 1) );
	if (!nptr)return false;
	*pointer_conversions_out = (void**)nptr;

	//printf("adding pointer %p replacded by %p\n",originalp,newp);

	(*pointer_conversions_in)[*num_pointers_converted] = originalp;
	(*pointer_conversions_out)[*num_pointers_converted] = newp;

	(*num_pointers_converted)++;
	return true;
}

//pointer to an array of void*s
int orcus_variable_deepcopy(orcus_variable* var_in, orcus_variable* var_out,orcus_uint* num_pointers_converted,void*** pointer_conversions_in,void*** pointer_conversions_out) {
	var_out->type = var_in->type;
	//printf("called!\n");
	if (type_is_pointer(var_in)) {
		void* cp = get_converted_pointer(var_in->data.p, num_pointers_converted, pointer_conversions_in, pointer_conversions_out);
		if (cp) {
			var_out->data.p = cp;
			//orcus_increment_variable_refrence_count(var_out);
			return DEEPCOPY_SUCCES;
		}
	}

	switch (var_in->type) {
		case ORCUS_TYPE_NIL:
		case ORCUS_TYPE_INT:
		case ORCUS_TYPE_FLOAT:
		case ORCUS_TYPE_BOOL:
		case ORCUS_TYPE_CFUNC:
			var_out->data = var_in->data;
			return DEEPCOPY_SUCCES;
		case ORCUS_TYPE_TABLE:
			{
			orcus_table* newtab=orcus_newtable();
			orcus_table* intab = (orcus_table*)var_in->data.p;
			if (!newtab) {
				return DEEPCOPY_MEMORY_ALLOCATION_ERROR;
			}
			add_converted_pointer(intab, newtab, num_pointers_converted, pointer_conversions_in, pointer_conversions_out);

			newtab->enviromental = false;
			newtab->references = 0;
			for (uint8_t t = 0; t < ORCUS_NUMBER_OF_TYPES;t++) {
				orcus_subtable st = intab->data[t];
				orcus_variable key;
				orcus_variable value;
				for (orcus_int i = 0; i < st.size; i++) {
					key=st.keys[i];
					value=st.values[i];
					if (var_can_be_deepcopied(&key) && var_can_be_deepcopied(&value) ) {
						orcus_variable newkey;
						orcus_variable newvalue;
						if (orcus_variable_deepcopy(&key, &newkey, num_pointers_converted, pointer_conversions_in, pointer_conversions_out) != DEEPCOPY_SUCCES) {
							continue;
						}
						if(orcus_variable_deepcopy(&value,&newvalue,num_pointers_converted,pointer_conversions_in,pointer_conversions_out) != DEEPCOPY_SUCCES) {
							orcus_release_var(&newkey);
							continue;
						}
						orcus_increment_variable_refrence_count(&newkey);
						orcus_increment_variable_refrence_count(&newvalue);
						orcus_setkey(newtab, &newkey, &newvalue);
					}
				}
			}
			var_out->data.p = newtab;

			}
			return DEEPCOPY_SUCCES;
		case ORCUS_TYPE_STRING:
			{
			orcus_string* nptr = (orcus_string*)malloc(sizeof(orcus_string));
			orcus_string* instr = (orcus_string*)var_in->data.p;
			if (!nptr) {
				return DEEPCOPY_MEMORY_ALLOCATION_ERROR;
			}
			memset(nptr, 0, sizeof(orcus_string));
			nptr->ptr=(char*)malloc(instr->size);
			if (!nptr->ptr) {
				free(nptr);
				return DEEPCOPY_MEMORY_ALLOCATION_ERROR;
			}
			add_converted_pointer(instr, nptr, num_pointers_converted, pointer_conversions_in, pointer_conversions_out);
			memcpy(nptr->ptr, instr->ptr, instr->size);
			nptr->size = instr->size;
			nptr->references = 0;
			var_out->data.p = nptr;
			}
			return DEEPCOPY_SUCCES;
		case ORCUS_TYPE_ARRAY:
			{
				orcus_array* newarr = orcus_newarray();
				newarr->references = 0;
				orcus_array* inarr = (orcus_array*)var_in->data.p;
				if (!newarr) {
					return DEEPCOPY_MEMORY_ALLOCATION_ERROR;
				}
				
				add_converted_pointer(inarr, newarr, num_pointers_converted, pointer_conversions_in, pointer_conversions_out);

				if (inarr->values) {
#ifdef ORCUS_64BIT
					for (int i1 = 0; i1 < ORCUS_SIZE_SUBARRAY_1; i1++) {
						orcus_variable***** const st1 = inarr->values[i1];
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
						orcus_variable** const st1 = inarr->values[i1];
						if (!st1)continue;
						for (int i2 = 0; i2 < ORCUS_SIZE_SUBARRAY_2; i2++) {
							orcus_variable* const st2 = st1[i2];
							if (!st2)continue;
							for (int i3 = 0; i3 < ORCUS_SIZE_SUBARRAY_3; i3++) {
								orcus_variable const var = st2[i3];
								const orcus_int index = orcus_reconstruct_array_index(i1, i2, i3);
#endif
								if (var.type && var_can_be_deepcopied(&var)) {
									orcus_variable newvar;
									if (orcus_variable_deepcopy((orcus_variable*)&var, &newvar, num_pointers_converted, pointer_conversions_in, pointer_conversions_out) != DEEPCOPY_SUCCES) {
										continue;
									}
									orcus_increment_variable_refrence_count(&newvar);
									orcus_setarray(newarr, &newvar, index);
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

				var_out->data.p = newarr;
			}
			
			return DEEPCOPY_SUCCES;
		case ORCUS_TYPE_FUNCTION:
			{
			orcus_function* nptr = (orcus_function*)malloc(sizeof(orcus_function));
			orcus_function* infun = (orcus_function*)var_in->data.p;
			if (!nptr) {
				return DEEPCOPY_MEMORY_ALLOCATION_ERROR;
			}
			memset(nptr, 0, sizeof(orcus_function));
			orcus_clone_function(infun, nptr);
			nptr->references = 0;
			if (nptr->numberofinstructions != infun->numberofinstructions) {
				free(nptr);
				return DEEPCOPY_MEMORY_ALLOCATION_ERROR;
			}

			add_converted_pointer(infun, nptr, num_pointers_converted, pointer_conversions_in, pointer_conversions_out);
			}
			return DEEPCOPY_SUCCES;
		case ORCUS_TYPE_FILE:
		case ORCUS_TYPE_THREAD:
		default:
			return DEEPCOPY_UNCOPYABLE_TYPE;
	}

}



void orcus_lib_std_deepcopy(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable in;
	orcus_variable out;
	orcus_popstack(M, &in);
	
	orcus_uint nc = 0;
	void** cpi = nullptr;
	void** cpo = nullptr;
	int code=orcus_variable_deepcopy(&in, &out, &nc, &cpi, &cpo);
	free(cpi);
	free(cpo);
	orcus_release_var(&in);

	if (code != DEEPCOPY_SUCCES) {
		out.data.i = 0;
		out.type = ORCUS_TYPE_NIL;
	}
	orcus_increment_variable_refrence_count(&out);
	orcus_pushstack(M, &out);


	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

void orcus_lib_std_error(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0,1))return;


	orcus_variable in;
	in.type = ORCUS_TYPE_NIL;
	if(args_in)orcus_popstack(M, &in);
	orcus_string* str=orcus_tostring(&in);
	if (str) {
		char* cstr = orcus_mstring_to_cstring(str);
		if (cstr) {
			orcus_raise_error(M, ORCUS_ERROR_CUSTOM_STRING, (const orcus_int*)cstr);
			free(cstr);
		}
		orcus_mstring_delete(str);
	}
	orcus_release_var(&in);
	

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,0);
}

void orcus_lib_std_addlibs(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	orcus_variable in;
	orcus_table* tab;
	in.type = ORCUS_TYPE_NIL;
	if(args_in)orcus_popstack(M, &in);
	if (in.type) {
		

		if (in.type != ORCUS_TYPE_TABLE) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_TABLE, 1);
			orcus_release_var(&in);
			return;
		}
		tab = (orcus_table*)in.data.p;
	}
	else if(args_out){
		tab=orcus_newtable();
		if (!tab) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
	}
	else {
		return;
	}
	orcus_populate_table_with_libs(tab);
	if (args_out) {
		in.type = ORCUS_TYPE_TABLE;
		in.data.p = tab;
		orcus_pushstack(M, &in);
	}
	
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

void orcus_lib_std_sleep(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	orcus_variable in;
	orcus_popstack(M, &in);

	if (in.type == ORCUS_TYPE_INT) {
		in.data.f = (orcus_float)in.data.i;
	}
	else if (in.type == ORCUS_TYPE_FLOAT) {
		;
	}
	else {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_NUMBER_VARIABLEPROVIDED, &in, 1);
		orcus_release_var(&in);
		return;
	}

	if(in.data.f < 0)in.data.f = 0.0;
#ifdef _WIN32
	DWORD time = (DWORD)(in.data.f*1000.0);
	if (time == INFINITE)time--; //thanks, windows
	Sleep(time);
#else
	timespec ts;
	ts.tv_sec = (time_t)in.data.f;
	in.data.f -= (orcus_float)ts.tv_sec;
	in.data.f *= 1000000000.0; //convert to nanoseconds
	ts.tv_nsec = (long)in.data.f;
	if (ts.tv_nsec > 999999999)ts.tv_nsec = 999999999; //floating point error might cause an overshoot.
	nanosleep(&ts,NULL);
#endif

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 0);
}