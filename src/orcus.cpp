#include "orcus.hpp"
#include "orcus_bytecode.hpp"
#include "orcus_compiler.hpp"

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <pthread.h>
#endif

//orcus libs
#include "libs/orcus_lib_std.hpp"
#include "libs/orcus_lib_math.hpp"
#include "libs/orcus_lib_array.hpp"
#include "libs/orcus_lib_string.hpp"
#include "libs/orcus_lib_os.hpp"
#include "libs/orcus_lib_io.hpp"
#include "libs/orcus_lib_thread.hpp"
#include "libs/orcus_lib_table.hpp"
#include "libs/orcus_lib_debug.hpp"

uint8_t ORCUS_NUMBER_OF_TYPES = 11; //VERY IMPORTANT that this is kept at the proper number.
uint16_t register_max = 0xf;






orcus_string* orcus_cstring_to_mstring(const char* const ORCUS_CPP_restrict str , const orcus_int size) {
	orcus_string* nstr=(orcus_string*)malloc(sizeof(orcus_string));
	if (!nstr) return nullptr;
	char* nad = (char*)malloc(sizeof(char) * size);
	if (!nad) return nullptr;
	memcpy(nad, str, size * sizeof(char));

	memset(nstr, 0, sizeof(orcus_string));

	nstr->size = size;
	nstr->ptr = nad;
	return nstr;
}

orcus_string* orcus_cstring_const_to_mstring(const char* const ORCUS_CPP_restrict str, const orcus_int size) {
	orcus_string* nstr = (orcus_string*)malloc(sizeof(orcus_string));
	if (!nstr) return nullptr;

	nstr->size = size;
	nstr->ptr = (char*)str;
	nstr->constant = true;
	nstr->references = 0;
	return nstr;
}

//similar to orcus_cstring_to_mstring, but reuses the char* provided instead of copying the data
orcus_string* orcus_cstring_promote_to_mstring(char* const ORCUS_CPP_restrict str, const orcus_int size) {
	orcus_string* nstr = (orcus_string*)malloc(sizeof(orcus_string));
	if (!nstr) return nullptr;
	nstr->size = size;
	nstr->constant = false;
	nstr->references = 0;
	nstr->ptr = str;
	return nstr;
}

char* orcus_mstring_to_cstring(const orcus_string* const ORCUS_CPP_restrict str) {
	//orcus_int sz = strlen(str->ptr); //use this mecause null terminator
	//printf("%i/%i %s",str->size,sz,str->ptr);
	//if(sz > str->size)sz = str->size;
	char* out = (char*)malloc(sizeof(char) * (str->size+1)); 
	if (!out)return nullptr;

	memcpy(out, str->ptr, str->size*sizeof(char));
	out[str->size] = '\0';
	return out;
}


orcus_string* orcus_copystring(const orcus_string* const ORCUS_CPP_restrict str) {
	if (str->constant) {
		orcus_string* nstr = (orcus_string*)malloc(sizeof(orcus_string));
		if (nstr == nullptr) return nullptr;
		nstr->size = str->size;
		nstr->constant = true;
		nstr->ptr = str->ptr;
		nstr->references = 0;
		return nstr;
	}
	else {
		orcus_string* nstr = (orcus_string*)malloc(sizeof(orcus_string));
		if (nstr == nullptr) return nullptr;
		nstr->ptr = (char*)malloc(sizeof(char) * str->size);
		if (nstr->ptr == nullptr) return nullptr;
		nstr->size = str->size;
		nstr->constant = false;
		nstr->references = 0;
		memcpy(nstr->ptr, str->ptr, str->size * sizeof(char));
		return nstr;
	}
}

orcus_string* orcus_mstrings_concat(const orcus_string* const str1, const orcus_string* const str2) {
	orcus_string* nstr=(orcus_string*)malloc(sizeof(orcus_string));
	if (nstr == nullptr) return nullptr;

	nstr->ptr=(char*)malloc(sizeof(char) * (str1->size + str2->size));
	if (nstr->ptr == nullptr) {
		free(nstr);
		return nullptr;
	}

	memcpy(nstr->ptr,str1->ptr,str1->size*sizeof(char));
	memcpy(nstr->ptr+str1->size, str2->ptr, str2->size * sizeof(char));
	nstr->constant = false;
	nstr->size = str1->size + str2->size;
	nstr->references = 0;
	return nstr;
}

//like concat, but does not return a new string. adds appstr to the end of basestr.
bool orcus_mstrings_append(orcus_string* const basestr, const orcus_string* const appstr) {
	if (basestr->constant) {
		char* nptr = (char*)malloc( sizeof(char) * (basestr->size + appstr->size));
		if (!nptr)return false;
		if(basestr->size)memcpy(nptr,basestr->ptr, basestr->size);
		basestr->ptr = nptr;
		if(appstr->size)memcpy(basestr->ptr + basestr->size, appstr->ptr, appstr->size);
		basestr->size += appstr->size;
		basestr->constant = false;
	}
	else {
		char* nptr = (char*)realloc(basestr->ptr, sizeof(char) * (basestr->size + appstr->size));
		if (!nptr)return false;
		basestr->ptr = nptr;
		if(appstr->size)memcpy(basestr->ptr + basestr->size, appstr->ptr, appstr->size);
		basestr->size += appstr->size;
	}
	return true;
}

bool orcus_mstring_addchars(orcus_string* const ORCUS_CPP_restrict str, const char* const chars, orcus_int len) {
	if (str->constant) {
		char* nptr = (char*)malloc(sizeof(char) * (str->size + len));
		if (!nptr)return false;
		memcpy(nptr,str->ptr, str->size);
		str->ptr = nptr;
		memcpy(str->ptr + str->size, chars, len);
		str->size += len;
		str->constant = false;
	}
	else {
		char* nptr = (char*)realloc(str->ptr, sizeof(char) * (str->size + len));
		if (!nptr)return false;
		str->ptr = nptr;
		for (orcus_int i=0;i<len;i++){
			str->ptr[str->size + i]=chars[i];
		}
		//memcpy(str->ptr + str->size, chars, len);
		str->size += len;
	}
	return true;
}

void orcus_mstring_delete(orcus_string* const ORCUS_CPP_restrict str) {
	if (!str->constant) {
		free(str->ptr);
	}
	free(str);
}

orcus_string* orcus_mstring_substring(orcus_string* str, orcus_int start, orcus_int end) {
	orcus_string* nstr = (orcus_string*)malloc(sizeof(orcus_string));
	if (nstr == nullptr) return nullptr;

	if (start > str->size || end < 0) { //no characters? just return an empty string.
		nstr->ptr = (char*)"";
		nstr->constant = true;
		nstr->size = 0;

		return nstr;
	}

	start = start < 0 ? 0 : start;
	end = end > str->size-1 ? str->size-1 : end; //clip to the bounds of the string.

	nstr->size = 1l+end - start;
	nstr->ptr=(char*)malloc(sizeof(char)*nstr->size);
	nstr->references = 0;
	nstr->constant = false;
	if (!nstr->ptr) {
		nstr->size = 0;
		return nstr;
	}
	memcpy(nstr->ptr, str->ptr + start,nstr->size*sizeof(char));

	return nstr;
}


orcus_table* orcus_newtable() {

	orcus_table* newt = (orcus_table*)malloc(sizeof(orcus_table));
	if (newt == nullptr) return nullptr;
	newt->data=(orcus_subtable*)malloc(sizeof(orcus_subtable) * ORCUS_NUMBER_OF_TYPES);
	if (!newt->data)return nullptr;
	memset(newt->data, 0, sizeof(orcus_subtable)*ORCUS_NUMBER_OF_TYPES);
	/* use this for later if non-zerod values are needed.
	for (uint8_t i = 0; i < ORCUS_NUMBER_OF_TYPES; i++) {
		memset(newt->data+i, 0, sizeof(orcus_subtable));
	}
	*/
	newt->enviromental = false;
	newt->references = 0;

	return newt;
}

void orcus_destroytable(orcus_table* const table) { //not ideal, but it works. kind of.
	for (uint8_t i = 0; i < ORCUS_NUMBER_OF_TYPES; i++) {
		orcus_subtable st = table->data[i];
		for (orcus_int i2 = 0; i2 < st.size; i2++) {
			orcus_decrement_variable_refrence_count(st.keys + i2);
			orcus_decrement_variable_refrence_count(st.values + i2);
			orcus_release_var(st.keys + i2);
			orcus_release_var(st.values + i2);
		}
		free(st.keys);
		free(st.values);
	}
	free(table->data);
	free(table);
}

void orcus_cleartable(const orcus_table* const table) {
	for (uint8_t i = 0; i < ORCUS_NUMBER_OF_TYPES; i++) {
		orcus_subtable st = table->data[i];
		for (orcus_int i2 = 0; i2 < st.size; i2++) {
			orcus_decrement_variable_refrence_count(st.keys + i2);
			orcus_decrement_variable_refrence_count(st.values + i2);
			orcus_release_var(st.keys + i2);
			orcus_release_var(st.values + i2);
		}
		table->data[i].size = 0;
	}
}


orcus_int orcus_tablehaskey(const orcus_table* const table, const orcus_variable* const key) {
	const orcus_subtable subt = table->data[key->type];
	orcus_int i = subt.size;
	if (orcus_type_is_complex(key->type)) {
		while (i) {
			i--;
			if (orcus_vars_equal(subt.keys + i, key))return i;
		}
	}
	else {
		while (i) {
			i--;
			if (key->data.i == subt.keys[i].data.i)return i;
		}
	}
	return -1;
}

bool orcus_getkey(const orcus_table* const table, orcus_variable* const key, orcus_variable* out) {
	const orcus_subtable subt=table->data[key->type];
	orcus_int i = subt.size;
	if (orcus_type_is_complex(key->type)) {
		while (i) {
			i--;
			if (orcus_vars_equal(subt.keys + i, key)) {
				*out = subt.values[i];
				return true;
			}
		}
	}
	else {
		while (i) {
			i--;
			if (key->data.i == subt.keys[i].data.i) {
				*out = subt.values[i];
				return true;
			}
		}
	}
	
	//orcus_release_var(key);
	out->type = ORCUS_TYPE_NIL;
	out->data.i = 0;
	return false;
}

orcus_int orcus_setkey(orcus_table* const table, orcus_variable* const key, const orcus_variable* const value) {
	orcus_subtable subt = table->data[key->type];
	orcus_int i= subt.size;
	if (orcus_type_is_complex(key->type)) {
		while (i) {
			i--;
			if (orcus_vars_equal(subt.keys + i, key)) {
				orcus_increment_variable_refrence_count(value);
				orcus_increment_variable_refrence_count(key);
				orcus_decrement_variable_refrence_count(subt.values + i);
				orcus_decrement_variable_refrence_count(subt.keys + i);
				orcus_release_var(subt.values + i);
				orcus_release_var(subt.keys + i);
				subt.values[i] = *value;
				subt.keys[i] = *key;
				return i;
			}
		}
	}
	else {
		while (i) {
			i--;
			if (key->data.i == subt.keys[i].data.i) {
				orcus_increment_variable_refrence_count(value);
				orcus_increment_variable_refrence_count(key);
				orcus_decrement_variable_refrence_count(subt.values + i);
				orcus_decrement_variable_refrence_count(subt.keys + i);
				orcus_release_var(subt.values + i);
				orcus_release_var(subt.keys + i);
				subt.values[i] = *value;
				subt.keys[i] = *key;
				return i;
			}
		}
	}

	void* nptr=realloc(subt.keys,sizeof(orcus_variable)*(subt.size+1) );
	if (nptr == nullptr) return -1;
	subt.keys = (orcus_variable*)nptr;
	nptr = realloc(subt.values, sizeof(orcus_variable) * (subt.size + 1));
	if (nptr == nullptr) return -1;
	subt.values = (orcus_variable*)nptr;

	orcus_increment_variable_refrence_count(key);
	orcus_increment_variable_refrence_count(value);
	subt.keys[subt.size] = *key;
	subt.values[subt.size] = *value;

	subt.size++;
	table->data[key->type] = subt;
	return subt.size-1;
}

bool orcus_tables_equal(const orcus_table* const table1, const orcus_table* const table2) { //returns true if every single value of the tables match. no, it's not recursive, because fuck that (also because refs).
	for (uint8_t i = 0; i < ORCUS_NUMBER_OF_TYPES; i++) {
		orcus_subtable subt1=table1->data[i];
		orcus_subtable subt2=table2->data[i];

		if (subt1.size != subt2.size) {
			return false;
		}

		for (orcus_int i1 = 0; i1 < subt1.size; i1++) { //not ideal. O(n^2)... this is what i get for not ordering anything.
			for (orcus_int i2 = 0; i2 < subt2.size; i2++) {
				if(orcus_vars_equal(subt1.keys+i1,subt2.keys+i2)) {
					goto found;
				}
			}
			return false;
			found:;
		}

	}
	return true;
}


bool orcus_table_get_cstring_keyvalue(const orcus_table* const table, const char* const key, orcus_variable* out) {
	const orcus_subtable* const subt = table->data+ORCUS_TYPE_STRING;
	for (orcus_int i = 0; i < subt->size; i++) {
		if(orcus_mstring_equal_cstring((orcus_string*)subt->keys[i].data.p,key)){
			*out = subt->values[i];
			return true;
		}
	}
	out->type = ORCUS_TYPE_NIL;
	out->data.i = 0;
	return false;
}


orcus_int orcus_table_set_cstring_keyvalue(orcus_table* const table, const char* const key, const orcus_variable* const value) {
	orcus_subtable subt = table->data[ORCUS_TYPE_STRING];
	for (orcus_int i = 0; i < subt.size; i++) {
		if (orcus_mstring_equal_cstring((orcus_string*)subt.keys[i].data.p,key)) {		
			orcus_increment_variable_refrence_count(value);
			orcus_decrement_variable_refrence_count(subt.values + i);
			orcus_release_var(subt.values+i);
			subt.values[i] = *value;
			return i;
		}
	}

	void* nptr = realloc(subt.keys, sizeof(orcus_variable) * (subt.size + 1));
	if (nptr == nullptr) return -1;
	subt.keys = (orcus_variable*)nptr;
	nptr = realloc(subt.values, sizeof(orcus_variable) * (subt.size + 1));
	if (nptr == nullptr) return -1;
	subt.values = (orcus_variable*)nptr;

	orcus_variable kv;
	kv.type = ORCUS_TYPE_STRING;

	orcus_string* nstr= orcus_cstring_const_to_mstring(key,strlen(key));
	if (!nstr)return -1;

	nstr->references = 1;
	orcus_increment_variable_refrence_count(value);
	kv.data.p = nstr;
	subt.keys[subt.size] = kv;
	subt.values[subt.size] = *value;

	subt.size++;
	table->data[ORCUS_TYPE_STRING] = subt;
	return subt.size - 1;
}

orcus_int orcus_table_has_cstring_key(const orcus_table* const table, const char* const key) {
	const orcus_subtable subt = table->data[ORCUS_TYPE_STRING];
	for (orcus_int i = 0; i < subt.size; i++) {
		if (orcus_mstring_equal_cstring((orcus_string*)subt.keys[i].data.p, key))return i;
	}
	return -1;
}


void orcus_prepare_table_for_state(orcus_table* table,orcus_state* M) {
	orcus_variable v;

	v.type = ORCUS_TYPE_TABLE;
	v.data.p = table;
	orcus_table_set_cstring_keyvalue(table, "_ENV", &v);

	v.data.p = M->masterstate ? M->masterstate->enviroment : table;
	orcus_table_set_cstring_keyvalue(table, "_G", &v);
	table->enviromental = true;
}

orcus_state* orcus_newstate(const orcus_state* const parent) {
	orcus_state* newstate=(orcus_state*)malloc(sizeof(orcus_state));
	if (newstate == nullptr) return nullptr;

	newstate->enviroment = orcus_newtable();
	if (newstate->enviroment == nullptr) {
		free(newstate);
		return nullptr;
	}
	newstate->enviroment->references = 1;

	/*
	orcus_variable* envvarkey = (orcus_variable*)malloc(sizeof(orcus_variable));
	if (!envvarkey)return nullptr;
	envvarkey->type = ORCUS_TYPE_STRING;
	envvarkey->data.p = orcus_cstring_const_to_mstring((char*)"_ENV",4);
	orcus_variable* envvarval = (orcus_variable*)malloc(sizeof(orcus_variable));
	if (!envvarval)return nullptr;
	envvarval->type = ORCUS_TYPE_TABLE;
	envvarval->data.p = newstate->enviroment;
	orcus_setkey(newstate->enviroment, envvarkey, envvarval);
	*/


	if (!parent) {
		newstate->registers = (orcus_variable*)malloc(sizeof(orcus_variable) * (1u+register_max));
		if (newstate->registers == nullptr) {
			free(newstate);
			return nullptr;
		}
		for (orcus_int i = 0; i <= register_max; i++) {
			newstate->registers = nullptr;
		}
		newstate->masterstate = newstate;
		newstate->parentstate =  nullptr;
	}
	else {
		newstate->registers = parent->registers;
		newstate->masterstate = parent->masterstate;
		newstate->parentstate = (orcus_state*)parent;

		parent->enviroment->references+=1;
		parent->masterstate->enviroment->references += 1;
	}
	newstate->childstate = nullptr;

	orcus_prepare_table_for_state(newstate->enviroment, newstate);

	newstate->constants = nullptr;
	newstate->num_constants = 0;

	/*
	orcus_variable* globvarkey = (orcus_variable*)malloc(sizeof(orcus_variable));
	if (!globvarkey)return nullptr;
	globvarkey->type = ORCUS_TYPE_STRING;
	globvarkey->data.p = orcus_cstring_const_to_mstring((char*)"_G", 2);
	orcus_variable* globvarval = (orcus_variable*)malloc(sizeof(orcus_variable));
	if (!globvarval)return nullptr;
	globvarval->type = ORCUS_TYPE_TABLE;
	globvarval->data.p = newstate->masterstate->enviroment;
	orcus_setkey(newstate->enviroment, globvarkey, globvarval);
	*/

	

	newstate->sizeofstack = 0;
	newstate->allocatedstacksize = 0;
	newstate->stack = nullptr;

	newstate->programcounter = 0;
	newstate->errorcode = 0;

	newstate->bytecode = nullptr;

	//newstate->numberofinstructions = 0;
	//newstate->instructions = nullptr;


	return newstate;
}


bool orcus_stepstate(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->programcounter >= M->bytecode->numberofinstructions) return false;

	orcus_opcode opcode = M->bytecode->instructions[M->programcounter];

	//printf("%i - %i %i\n", M->programcounter,iflags,opcode);
	M->programcounter++;
	orcus_bytecode_list[opcode](M);

	/*
	printf("post stack: %i\n", M->sizeofstack);
	for (orcus_int i = 0; i < M->sizeofstack; i++) {
		orcus_variable* v = M->stack[i];
		printf("\t%i [%i]: %i %f %p\n",i,v->type,v->data.i, v->data.f, v->data.p);
	}
	//*/

	return true;
}

void orcus_clearstate(orcus_state* const ORCUS_CPP_restrict M, bool for_deletion) {
	for (orcus_uint i = 0; i < M->sizeofstack; i++) {
		orcus_release_var(M->stack+i);
	}
	M->sizeofstack = 0;
	
	
	if (M->masterstate == M && M->registers) {
		for (orcus_uint i = 0; i < register_max; i++) {
			if (M->registers[i].type) {
				orcus_release_var(M->registers+i);
				M->registers[i].type = ORCUS_TYPE_NIL;
			}
		}
	}

	if (M->enviroment) {
		if (!for_deletion) {
			orcus_cleartable(M->enviroment);
			orcus_prepare_table_for_state(M->enviroment, M);
		}
		else {
			M->enviroment->enviromental = false;
			orcus_cleartable(M->enviroment);
			M->enviroment->references--;
			if(!M->enviroment->references)orcus_destroytable(M->enviroment);
			M->enviroment = nullptr;
		}
	}


	for (orcus_uint i = 0; i < M->num_constants; i++) {
		orcus_decrement_variable_refrence_count(M->constants+i);
		orcus_release_var(M->constants+i);
	}
	M->num_constants = 0;

	M->programcounter = 0;
}

void orcus_destroystate(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->childstate) {
		orcus_destroystate(M->childstate);
	}
	orcus_clearstate(M,true);

	if (M->parentstate) {
		M->parentstate->enviroment->references -= 1;
		M->parentstate->masterstate->enviroment->references -= 1;
	}


	free(M->stack);

	//if (M->enviroment)orcus_destroytable(M->enviroment);
	if (M->bytecode) {
		M->bytecode->references--;
		if (!M->bytecode->references) {
			orcus_clear_function(M->bytecode);
			free(M->bytecode);
		}
	}

	if (M->masterstate == M && M->registers) {
		free(M->registers);
	}

	free(M);
}

void orcus_release_var(orcus_variable* const ORCUS_CPP_restrict var) {
	switch (var->type)
	{
	case ORCUS_TYPE_NIL:
	case ORCUS_TYPE_INT:
	case ORCUS_TYPE_FLOAT:
	case ORCUS_TYPE_BOOL:
	case ORCUS_TYPE_CFUNC:
		return;
	case ORCUS_TYPE_TABLE:
		if (!((orcus_table*)var->data.p)->references) {
			orcus_destroytable((orcus_table*)var->data.p);
		}
		return;
	case ORCUS_TYPE_STRING:
		if (!((orcus_string*)var->data.p)->references) {
			orcus_mstring_delete((orcus_string*)var->data.p);
		}
		return;
	case ORCUS_TYPE_ARRAY:
		if (!((orcus_array*)var->data.p)->references) {
			orcus_destroyarray((orcus_array*)var->data.p);
		}
		return;
	case ORCUS_TYPE_FUNCTION:
		{
			orcus_function* ffunction = (orcus_function*)var->data.p;
			if (!ffunction->references) {
				orcus_clear_function(ffunction);
				free(ffunction);
			}
		}
		return;
	case ORCUS_TYPE_FILE:
		{
			orcus_filewrapper* fw = (orcus_filewrapper*)var->data.p;
			if (!fw->references) {
				if (fw->modeflags)fclose(fw->file);
				free(fw);
			}
		}
		return;
	case ORCUS_TYPE_THREAD:
		{
			orcus_threadholder* t = (orcus_threadholder*)var->data.p;
			if (!t->references) {
				if (!t->finished) { //you stupid son of a bitch why are you like this?
	#if defined(_WIN32) || defined(_WIN64)
					WaitForSingleObject(t->threadobject, INFINITE);
					CloseHandle(t->threadobject);
					t->threadobject = NULL;
	#else
					pthread_join(t->threadobject, NULL);
					t->threadobject = NULL;
	#endif
				}
				if (t->customenv) {
					t->state->enviroment->references--;
					if (!t->state->enviroment->references)orcus_destroytable(t->state->enviroment);
					t->state->enviroment = nullptr;
				}
				//t->state->bytecode.instructions = nullptr;
				orcus_destroystate(t->state);
				free(t);
			}

		}
		return;
	default:
		return;
	}
}

void orcus_popstack(orcus_state* const ORCUS_CPP_restrict M, orcus_variable* out) {
	if (M->sizeofstack==0) {
#ifdef ORCUS_DEBUG
		printf("popstack called when stack was empty.\n");
#endif
		out->type = ORCUS_TYPE_NIL;
		out->data.i = 0;
		return;
	}
	M->sizeofstack--;
	*out=  M->stack[M->sizeofstack];
}

//takes from the bottom instead of the top of stack
void orcus_pullstack(orcus_state* const ORCUS_CPP_restrict M, orcus_variable* out) {
	if (M->sizeofstack == 0) {
#ifdef ORCUS_DEBUG
		printf("pullstack called when stack was empty.\n");
#endif
		out->type = ORCUS_TYPE_NIL;
		out->data.i = 0;
		return;
	}

	*out = M->stack[0];
	M->sizeofstack--;
	memmove(M->stack, M->stack + 1, sizeof(orcus_variable) * M->sizeofstack);
}


bool orcus_pushstack(orcus_state* const ORCUS_CPP_restrict M, orcus_variable* const var) {
	if (!(M->allocatedstacksize > M->sizeofstack)) {
		void* nstackptr = realloc(M->stack, (M->sizeofstack + 1) * sizeof(orcus_variable));
		if (nstackptr == nullptr) return false;
		M->stack = (orcus_variable*)nstackptr;

		M->allocatedstacksize = M->sizeofstack + 1;
	}

	M->stack[M->sizeofstack] = *var;
	M->sizeofstack++;
	return true;
}

orcus_array* orcus_newarray() {
	orcus_array* nar = (orcus_array*)malloc(sizeof(orcus_array));
	if (nar == nullptr)return nullptr;

	nar->references = 0;
	nar->values = nullptr;

	return nar;
}

void orcus_destroyarray(orcus_array* const ORCUS_CPP_restrict arr) {
	if (arr->values) {
#ifdef ORCUS_64BIT
		//this can't be the best way to do it. i mean... just look at this piece of shit.
		for (int i1 = (ORCUS_SIZE_SUBARRAY_1 - 1); i1 >= 0; i1--) {
			orcus_variable***** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = (ORCUS_SIZE_SUBARRAY_2 - 1); i2 >= 0; i2--) {
				orcus_variable**** const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = (ORCUS_SIZE_SUBARRAY_3 - 1); i3 >= 0; i3--) {
					orcus_variable*** const st3 = st2[i3];
					if (!st3)continue;
					for (int i4 = (ORCUS_SIZE_SUBARRAY_4 - 1); i4 >= 0; i4--) {
						orcus_variable** const st4 = st3[i4];
						if (!st4)continue;
						for (int i5 = (ORCUS_SIZE_SUBARRAY_5 - 1); i5 >= 0; i5--) {
							orcus_variable* const st5 = st4[i5];
							if (!st5)continue;
							for (int i6 = (ORCUS_SIZE_SUBARRAY_6 - 1); i6 >= 0; i6--) {
								orcus_variable* var = st5+i6;
								if (var->type) { orcus_decrement_variable_refrence_count(var); orcus_release_var(var); }
							}
							free(st5);
						}
						free(st4);
					}
					free(st3);
				}
				free(st2);
			}
			free(st1);
		}
#else
		//it's less shit here but still not great.
		for (int i1 = (ORCUS_SIZE_SUBARRAY_1 - 1); i1 >= 0; i1--) {
			orcus_variable** const st1 = arr->values[i1];
			if (!st1)continue;
			for (int i2 = (ORCUS_SIZE_SUBARRAY_2 - 1); i2 >= 0; i2--) {
				orcus_variable* const st2 = st1[i2];
				if (!st2)continue;
				for (int i3 = (ORCUS_SIZE_SUBARRAY_3 - 1); i3 >= 0; i3--) {
					orcus_variable* const var = st2+i3;
					if (var->type) { orcus_decrement_variable_refrence_count(var); orcus_release_var(var); }
				}
				free(st2);
			}
			free(st1);
		}
#endif
	}
	free(arr->values);
	free(arr);
}

bool orcus_setarray(orcus_array* const array, const orcus_variable* const var, const orcus_int pos) {
#ifdef ORCUS_64BIT
	if (!array->values) {
		array->values = (orcus_variable******)calloc(ORCUS_SIZE_SUBARRAY_1, sizeof(void*));
		if (!array->values) {
			return false;
		}
	}

	int current_subindex = get_array_index_from_mint_1(pos);
	orcus_variable *****sa1 = array->values[current_subindex];
	if (!sa1) {
		sa1=(orcus_variable*****)calloc(ORCUS_SIZE_SUBARRAY_2, sizeof(void*));
		if (!sa1) {
			return false;
		}
		array->values[current_subindex]=sa1;
	}
	
	current_subindex = get_array_index_from_mint_2(pos);
	orcus_variable**** sa2 = sa1[current_subindex];
	if (!sa2) {
		sa2 = (orcus_variable****)calloc(ORCUS_SIZE_SUBARRAY_3, sizeof(void*));
		if (!sa2) {
			return false;
		}
		sa1[current_subindex] = sa2;
	}

	current_subindex = get_array_index_from_mint_3(pos);
	orcus_variable*** sa3 = sa2[current_subindex];
	if (!sa3) {
		sa3 = (orcus_variable***)calloc(ORCUS_SIZE_SUBARRAY_4, sizeof(void*));
		if (!sa3) {
			return false;
		}
		sa2[current_subindex] = sa3;
	}

	current_subindex = get_array_index_from_mint_4(pos);
	orcus_variable** sa4 = sa3[current_subindex];
	if (!sa4) {
		sa4 = (orcus_variable**)calloc(ORCUS_SIZE_SUBARRAY_5, sizeof(void*));
		if (!sa4) {
			return false;
		}
		sa3[current_subindex] = sa4;
	}

	current_subindex = get_array_index_from_mint_5(pos);
	orcus_variable* sa5 = sa4[current_subindex];
	if (!sa5) {
		sa5 = (orcus_variable*)calloc(ORCUS_SIZE_SUBARRAY_6, sizeof(orcus_variable));
		if (!sa5) {
			return false;
		}
		sa4[current_subindex] = sa5;
	}

	current_subindex = get_array_index_from_mint_6(pos);
	orcus_variable* arrvar = sa5+current_subindex;
	orcus_increment_variable_refrence_count(var);
	if (arrvar->type) {
		orcus_decrement_variable_refrence_count(arrvar);
		orcus_release_var(arrvar);
	}
	sa5[current_subindex] = *var;
#else
	if (!array->values) {
		array->values = (orcus_variable***)calloc(ORCUS_SIZE_SUBARRAY_1, sizeof(void*));
		if (!array->values) {
			return false;
		}
	}

	int current_subindex = get_array_index_from_mint_1(pos);
	orcus_variable** sa1 = array->values[current_subindex];
	if (!sa1) {
		sa1 = (orcus_variable**)calloc(ORCUS_SIZE_SUBARRAY_2, sizeof(void*));
		if (!sa1) {
			return false;
		}
		array->values[current_subindex] = sa1;
	}

	current_subindex = get_array_index_from_mint_2(pos);
	orcus_variable* sa2 = sa1[current_subindex];
	if (!sa2) {
		sa2 = (orcus_variable*)calloc(ORCUS_SIZE_SUBARRAY_3, sizeof(orcus_variable));
		if (!sa2) {
			return false;
		}
		sa1[current_subindex] = sa2;
	}

	current_subindex = get_array_index_from_mint_3(pos);
	orcus_variable* arrvar = sa2+current_subindex;
	orcus_increment_variable_refrence_count(var);
	if (arrvar->type) {
		orcus_decrement_variable_refrence_count(arrvar);
		orcus_release_var(arrvar);
	}
	sa2[current_subindex] = *var;
#endif
	return true;
}

//clears the variable at the index WITHOUT freeing it. you must manually do that!
bool orcus_cleararrayindex(orcus_array* const array, const orcus_int pos) {
	orcus_variable cleared_var;
	cleared_var.data.i = 0;
	cleared_var.type = ORCUS_TYPE_NIL;
#ifdef ORCUS_64BIT
	if (!array->values) {
		array->values = (orcus_variable******)calloc(ORCUS_SIZE_SUBARRAY_1, sizeof(void*));
		if (!array->values) {
			return false;
		}
	}

	int current_subindex = get_array_index_from_mint_1(pos);
	orcus_variable***** sa1 = array->values[current_subindex];
	if (!sa1) {
		sa1 = (orcus_variable*****)calloc(ORCUS_SIZE_SUBARRAY_2, sizeof(void*));
		if (!sa1) {
			return false;
		}
		array->values[current_subindex] = sa1;
	}

	current_subindex = get_array_index_from_mint_2(pos);
	orcus_variable**** sa2 = sa1[current_subindex];
	if (!sa2) {
		sa2 = (orcus_variable****)calloc(ORCUS_SIZE_SUBARRAY_3, sizeof(void*));
		if (!sa2) {
			return false;
		}
		sa1[current_subindex] = sa2;
	}

	current_subindex = get_array_index_from_mint_3(pos);
	orcus_variable*** sa3 = sa2[current_subindex];
	if (!sa3) {
		sa3 = (orcus_variable***)calloc(ORCUS_SIZE_SUBARRAY_4, sizeof(void*));
		if (!sa3) {
			return false;
		}
		sa2[current_subindex] = sa3;
	}

	current_subindex = get_array_index_from_mint_4(pos);
	orcus_variable** sa4 = sa3[current_subindex];
	if (!sa4) {
		sa4 = (orcus_variable**)calloc(ORCUS_SIZE_SUBARRAY_5, sizeof(void*));
		if (!sa4) {
			return false;
		}
		sa3[current_subindex] = sa4;
	}

	current_subindex = get_array_index_from_mint_5(pos);
	orcus_variable* sa5 = sa4[current_subindex];
	if (!sa5) {
		sa5 = (orcus_variable*)calloc(ORCUS_SIZE_SUBARRAY_6, sizeof(orcus_variable));
		if (!sa5) {
			return false;
		}
		sa4[current_subindex] = sa5;
	}

	current_subindex = get_array_index_from_mint_6(pos);
	orcus_variable* arrvar = sa5 + current_subindex;
	if (arrvar->type) {
		orcus_decrement_variable_refrence_count(arrvar);
		orcus_release_var(arrvar);
	}
	sa5[current_subindex] = cleared_var;
#else
	if (!array->values) {
		array->values = (orcus_variable***)calloc(ORCUS_SIZE_SUBARRAY_1, sizeof(void*));
		if (!array->values) {
			return false;
		}
	}

	int current_subindex = get_array_index_from_mint_1(pos);
	orcus_variable** sa1 = array->values[current_subindex];
	if (!sa1) {
		sa1 = (orcus_variable**)calloc(ORCUS_SIZE_SUBARRAY_2, sizeof(void*));
		if (!sa1) {
			return false;
		}
		array->values[current_subindex] = sa1;
	}

	current_subindex = get_array_index_from_mint_2(pos);
	orcus_variable* sa2 = sa1[current_subindex];
	if (!sa2) {
		sa2 = (orcus_variable*)calloc(ORCUS_SIZE_SUBARRAY_3, sizeof(orcus_variable));
		if (!sa2) {
			return false;
		}
		sa1[current_subindex] = sa2;
	}

	current_subindex = get_array_index_from_mint_3(pos);
	orcus_variable* arrvar = sa2 + current_subindex;
	if (arrvar->type) {
		//orcus_decrement_variable_refrence_count(arrvar);
		//orcus_release_var(arrvar);
	}
	sa2[current_subindex] = cleared_var;
#endif
	return true;
}

void orcus_getarray(orcus_array* const array, const orcus_int pos, orcus_variable* out) {
	if (!array->values)goto no_index;
#ifdef ORCUS_64BIT
	{
		int current_subindex = get_array_index_from_mint_1(pos);
		orcus_variable***** sa1 = array->values[current_subindex];
		if (!sa1)goto no_index;
		current_subindex = get_array_index_from_mint_2(pos);
		orcus_variable**** sa2 = sa1[current_subindex];
		if (!sa2)goto no_index;
		current_subindex = get_array_index_from_mint_3(pos);
		orcus_variable*** sa3 = sa2[current_subindex];
		if (!sa3)goto no_index;
		current_subindex = get_array_index_from_mint_4(pos);
		orcus_variable** sa4 = sa3[current_subindex];
		if (!sa4)goto no_index;
		current_subindex = get_array_index_from_mint_5(pos);
		orcus_variable* sa5 = sa4[current_subindex];
		if (!sa5)goto no_index;
		current_subindex = get_array_index_from_mint_6(pos);
		orcus_variable* var = sa5+current_subindex;
		if (var->type) {
			*out = *var;
			return;
		}
	}
#else
	{
		int current_subindex = get_array_index_from_mint_1(pos);
		orcus_variable** sa1 = array->values[current_subindex];
		if (!sa1)goto no_index;
		current_subindex = get_array_index_from_mint_2(pos);
		orcus_variable* sa2 = sa1[current_subindex];
		if (!sa2)goto no_index;
		current_subindex = get_array_index_from_mint_3(pos);
		orcus_variable* var = sa2+current_subindex;
		if (var) { 
			*out = *var;
			return; 
		}
	}
#endif
	no_index:
	out->data.i = 0;
	out->type = ORCUS_TYPE_NIL;
}

orcus_int orcus_array_len(const orcus_array* const ORCUS_CPP_restrict arr) {
	if (!arr->values)return -1;
	orcus_int out = 0;
#ifdef ORCUS_64BIT
	//this can't be the best way to do it. i mean... just look at this piece of shit.
	for (int i1 = (ORCUS_SIZE_SUBARRAY_1 - 1) >> 1; i1>=0; i1--) { //bitshift right once because we are ignoring negative values, and those start with 1
		orcus_variable***** const st1 = arr->values[i1];
		if (!st1)continue;
		for (int i2 = (ORCUS_SIZE_SUBARRAY_2 - 1); i2 >= 0; i2--) {
			orcus_variable**** const st2 = st1[i2];
			if (!st2)continue;
			for (int i3 = (ORCUS_SIZE_SUBARRAY_3 - 1); i3 >= 0; i3--) {
				orcus_variable*** const st3 = st2[i3];
				if (!st3)continue;
				for (int i4 = (ORCUS_SIZE_SUBARRAY_4 - 1); i4 >= 0; i4--) {
					orcus_variable** const st4 = st3[i4];
					if (!st4)continue;
					for (int i5 = (ORCUS_SIZE_SUBARRAY_5 - 1); i5 >= 0; i5--) {
						orcus_variable* const st5 = st4[i5];
						if (!st5)continue;
						for (int i6 = (ORCUS_SIZE_SUBARRAY_6 - 1); i6 >= 0; i6--) {
							const orcus_variable* const var = st5+i6;
							if (var->type)return orcus_reconstruct_array_index(i1,i2,i3,i4,i5,i6);
						}
					}
				}
			}
		}
	}
#else
	//it's less shit here but still not great.
	for (int i1 = (ORCUS_SIZE_SUBARRAY_1 - 1) >> 1; i1 >= 0; i1--) { //bitshift right once because we are ignoring negative values, and those start with 1
		orcus_variable** const st1 = arr->values[i1];
		if (!st1)continue;
		for (int i2 = (ORCUS_SIZE_SUBARRAY_2 - 1); i2 >= 0; i2--) {
			orcus_variable* const st2 = st1[i2];
			if (!st2)continue;
			for (int i3 = (ORCUS_SIZE_SUBARRAY_3 - 1); i3 >= 0; i3--) {
				const orcus_variable* const var = st2+i3;
				if (var->type)return orcus_reconstruct_array_index(i1, i2, i3);
			}
		}
	}
#endif
	return -1;
}




orcus_string* orcus_tostring(const orcus_variable* const ORCUS_CPP_restrict var) {
	orcus_string* tstr=nullptr;

	char tout[256] = { '\0' };
	int tint;

	switch (var->type) {
	case ORCUS_TYPE_NIL:
		tstr = orcus_cstring_const_to_mstring((char*)"nil", 3);
		break;
	case ORCUS_TYPE_INT:
		tint=snprintf( tout,sizeof(tout),"%zi",var->data.i);
		if (tint==-1) {
			return nullptr;
		}
		tstr = orcus_cstring_to_mstring(tout, strlen(tout) );
		break;
	case ORCUS_TYPE_FLOAT:
		tint = snprintf(tout, sizeof(tout), "%#.10g", var->data.f);	
		if (tint==-1) {
			return nullptr;
		}
		tstr = orcus_cstring_to_mstring(tout, strlen(tout));
		break;
	case ORCUS_TYPE_BOOL:
		if (var->data.i) {
			tstr = orcus_cstring_const_to_mstring((char*)"true", 4);
		}
		else {
			tstr = orcus_cstring_const_to_mstring((char*)"false", 5);
		}
		break;
	case ORCUS_TYPE_TABLE:
		tint = snprintf(tout, sizeof(tout), "table %p", var->data.p);
		if (tint==-1) {
			return nullptr;
		}
		tstr = orcus_cstring_to_mstring(tout, strlen(tout));
		break;
	case ORCUS_TYPE_STRING:
		tstr = orcus_copystring((orcus_string*)var->data.p);
		break;
	case ORCUS_TYPE_CFUNC:
		tint = snprintf(tout, sizeof(tout), "c function %p", var->data.p);
		if (tint==-1) {
			return nullptr;
		}
		tstr = orcus_cstring_to_mstring(tout, strlen(tout));
		break;
	case ORCUS_TYPE_FUNCTION:
		tint = snprintf(tout, sizeof(tout), "function %p", var->data.p);
		if (tint==-1) {
			return nullptr;
		}
		tstr = orcus_cstring_to_mstring(tout, strlen(tout));
		break;
	case ORCUS_TYPE_ARRAY:
		tint = snprintf(tout, sizeof(tout), "array %p", var->data.p);
		if (tint==-1) {
			return nullptr;
		}
		tstr = orcus_cstring_to_mstring(tout, strlen(tout));
		break;
	case ORCUS_TYPE_FILE:
		tint = snprintf(tout, sizeof(tout), "file %p", var->data.p);
		if (tint==-1) {
			return nullptr;
		}
		tstr = orcus_cstring_to_mstring(tout, strlen(tout));
		break;
	default:
		tstr = orcus_cstring_const_to_mstring((char*)"unknown", 7);
	}
	
	return tstr;
}

bool orcus_checkbool(const orcus_variable* const ORCUS_CPP_restrict var) {
	switch (var->type) {
	case ORCUS_TYPE_NIL:
		return false;
	case ORCUS_TYPE_BOOL:
		if (var->data.i) {
			return true;
		}
		else {
			return false;
		}
	case ORCUS_TYPE_INT:
		return var->data.i != 0;
	default:
		return true;
	}
}

//type coersion when you really need it
orcus_int orcus_checkint(const orcus_variable* const ORCUS_CPP_restrict var) {
	switch (var->type) {
	case ORCUS_TYPE_NIL:
		return 0;
	case ORCUS_TYPE_BOOL:
		if (var->data.i) {
			return 1;
		}
		else {
			return 0;
		}
	case ORCUS_TYPE_INT:
		return var->data.i;
	case ORCUS_TYPE_FLOAT:
		return (orcus_int)var->data.f;
	default:
		return 0;
	}
}
//see above
orcus_float orcus_checkfloat(const orcus_variable* const ORCUS_CPP_restrict var) {
	switch (var->type) {
	case ORCUS_TYPE_NIL:
		return 0.0;
	case ORCUS_TYPE_BOOL:
		if (var->data.i) {
			return 1.0;
		}
		else {
			return 0.0;
		}
	case ORCUS_TYPE_INT:
		return (orcus_float)var->data.i;
	case ORCUS_TYPE_FLOAT:
		return var->data.f;
	default:
		return 0.0;
	}
}
//ditto
void* orcus_checkpointer(const orcus_variable* const ORCUS_CPP_restrict var) {
	switch (var->type) {
	case ORCUS_TYPE_NIL:
	case ORCUS_TYPE_BOOL:
	case ORCUS_TYPE_INT:
	case ORCUS_TYPE_FLOAT:
		return nullptr;
	default:
		return var->data.p;
	}
}


bool orcus_vars_equal(const orcus_variable* const var1, const orcus_variable* const var2) {
	if (var1->type != var2->type) {
		return false;
	}

	if (var1->type == ORCUS_TYPE_STRING) {
		return var1->data.i==var2->data.i || orcus_mstrings_equal( (orcus_string*)var1->data.p, (orcus_string*)var2->data.p);
	}
	else {
		return var1->data.i == var2->data.i;
	}
}


void orcus_debugdumptable(orcus_table* tab,int level) {
	for (uint8_t t = 0; t < ORCUS_NUMBER_OF_TYPES; t++) {
		orcus_subtable subt = tab->data[t];
		for (orcus_int i = 0; i < subt.size; i++) {
			for (int n = 0; n < level; n++) {
				putchar('\t');
			}
			orcus_rawdata keydat = subt.keys[i].data;
			switch (t) {
				case ORCUS_TYPE_NIL:
					printf("nil_%zi", keydat.i);
					break;
				case ORCUS_TYPE_INT:
					printf("%zi", keydat.i);
					break;
				case ORCUS_TYPE_FLOAT:
					printf("%f", keydat.f);
					break;
				case ORCUS_TYPE_STRING:
					{
					orcus_string* str = (orcus_string*)keydat.p;
					putchar('\"');
					for (orcus_int i2 = 0; i2 < str->size; i2++) {
						putchar(str->ptr[i2]);
						}
					}
					putchar('\"');
					break;
				default:
					printf("0x%p", keydat.p);
			}

			printf(" - ");

			orcus_variable* var = subt.values+i;

			switch (var->type) {
				case ORCUS_TYPE_INT:
					printf("%zi", var->data.i);
					break;
				case ORCUS_TYPE_FLOAT:
					printf("%f", var->data.f);
					break;
				case ORCUS_TYPE_TABLE:
					if (var->data.p != tab) {
						printf("TABLE 0x%p:\n", var->data.p);
						orcus_debugdumptable((orcus_table*)var->data.p, level + 1);
					}
					else {
						printf("TABLE <self> 0x%p", var->data.p);
					}
					break;
				case ORCUS_TYPE_ARRAY:
					printf("ARRAY 0x%p", var->data.p);
					{
						orcus_array* arr = (orcus_array*)var->data.p;
						orcus_int l=orcus_array_len(arr)+1;
						for (orcus_int q = 0; q < l; q++) {
							putchar('\n');
							orcus_variable v;
							orcus_getarray(arr, q,&v);
							for (int n = 0; n < level + 1; n++) {
								putchar('\t');
							}
							switch (v.type)
							{
							case ORCUS_TYPE_NIL:
								printf("%zi - nil", q);
								break;
							case ORCUS_TYPE_INT:
								printf("%zi - %zi", q, v.data.i);
								break;
							case ORCUS_TYPE_FLOAT:
								printf("%zi - %f", q, v.data.f);
								break;
							case ORCUS_TYPE_STRING:
							{
								orcus_string* sp = (orcus_string*)v.data.p;
							
								printf("%zi - ", q);
								putchar('\"');
								for (orcus_int sc = 0; sc < sp->size; sc++) {
									putchar(sp->ptr[sc]);
								}
								putchar('\"');
								
							}
								break;
							default:
								printf("%zi - [%hhu] 0x%p",q,v.type ,v.data.p);
								break;
							}
						}
					}
					break;
				case ORCUS_TYPE_STRING:
					{
						orcus_string* str = (orcus_string*)var->data.p;
						putchar('\"');
						for (orcus_int i2 = 0; i2 < str->size; i2++) {
							putchar(str->ptr[i2]);
						}
						putchar('\"');
					}
					break;
				default:
					printf("[%hhu] 0x%p",var->type, var->data.p);
			}

			
			

			putchar('\n');
		}
	}


}

void orcus_clone_function(orcus_function* in, orcus_function* out) {
	out->numberofinstructions = in->numberofinstructions - 1;// guard value. this will be overwritten later as long as the function executes correctly and these should be the same number.
	out->references = 0;
	if (in->instruction_dbg_lookup) {
		out->instruction_dbg_lookup = (orcus_uint*)malloc(sizeof(orcus_uint) * in->numberofinstructions);
		if (!out->instruction_dbg_lookup)return;
		out->dbg_tokens = (orcus_debug_token*)malloc(sizeof(orcus_debug_token) * in->num_dbg_tokens);
		if (!out->dbg_tokens)return;
		out->num_dbg_tokens = in->num_dbg_tokens;
		for (orcus_uint i = 0; i < in->num_dbg_tokens; i++) {
			out->dbg_tokens[i] = in->dbg_tokens[i];
			out->dbg_tokens[i].chars = (char*)malloc(in->dbg_tokens[i].num_chars);
			if (out->dbg_tokens[i].chars) {
				memcpy(out->dbg_tokens[i].chars, in->dbg_tokens[i].chars, in->dbg_tokens[i].num_chars);
			}
		}
		memcpy(out->instruction_dbg_lookup, in->instruction_dbg_lookup, sizeof(orcus_uint) * in->numberofinstructions);

	}
	else {
		out->instruction_dbg_lookup = nullptr;
		out->num_dbg_tokens = 0;
		out->dbg_tokens = nullptr;
	}
	out->instructions = (orcus_opcode*)malloc(sizeof(orcus_opcode) * in->numberofinstructions);
	if (!out->instructions)return;
	memcpy(out->instructions, in->instructions, sizeof(orcus_opcode) * in->numberofinstructions);
	out->numberofinstructions = in->numberofinstructions;
	out->enviromental = false;
	out->references = 0;
}

//takes a orcus_function* and frees the data associated at the pointer, but does NOT free the pointer itself.
void orcus_clear_function(orcus_function* in) {
	if (in->dbg_tokens) {
		for (orcus_uint i = 0; i < in->num_dbg_tokens; i++) {
			free(in->dbg_tokens[i].chars);
		}
		free(in->dbg_tokens);
	}
	if (in->instruction_dbg_lookup)free(in->instruction_dbg_lookup);
	free(in->instructions);
	memset(in, 0, sizeof(orcus_function));
}

inline const char* m_get_opcode_str(orcus_opcode instruction) {

	if (instruction > ORCUS_OPCODE_MSET) {
		return "????";
	}
	static const char* lookup[0xFFFF] = {
		" NOP", //0

		" ADD", //1
		" SUB", //2
		" MUL", //3
		" DIV", //4
		" POW", //5
		"IDIV", //6
		" MOD", //7

		"BAND", //8
		"BOR ", //9
		"BXOR", //10
		"BNOT", //11
		"BSHL", //12
		"BSHR", //13

		"LAND", //14
		"LOR ", //15
		"LXOR", //16
		"LNOT", //17

		"EQL ", //18
		"NEQ ", //19
		"GRT ", //20
		"LET ", //21
		"GTE ", //22
		"LTE ", //23

		"SENV", //24
		"GENV", //25
		"SET ", //26
		"GET ", //27
		"SREG", //28
		"GREG", //29

		"NINT", //30
		"NFLO", //31
		"NTRU", //32
		"NFAL", //33
		"NNIL", //34
		"NSTR", //35
		"NFUN", //36
		"NTAB", //37
		"NARR", //38

		"JMP ", //39
		"JMPR", //40
		"JIF ", //41
		"JNIF", //42
		"JRIF", //43
		"JRNI", //44

		"CALL", //45
		"EXIT", //46
		"LEN ", //47
		"CNCT", //48
		"CLS ", //49
		"GETL", //50
		"SETL", //51
		"GETG", //52
		"SETG", //53

		"CPYT", //54
		"SWPT", //55
		"CPYX", //56

		"UNM ", //57
		"INC ", //58
		"DEC ", //59

		"SCON", //60
		"GCON", //61

		"SWXY", //62

		"NNSE", //63

		"SETL_KCS", //64
		"GETL_KCS", //65
		"SETG_KCS", //66
		"GETG_KCS", //67
		"SENV_KCS", //68
		"GENV_KCS", //69
		"SET_KCS", //70
		"GET_KCS", //71

		"MSET", //72
	};
	return lookup[instruction];
}

orcus_string* orcus_get_bytecode_debug(orcus_function* F) {
	orcus_string* out= (orcus_string*)malloc(sizeof(orcus_string));
	if (!out)return nullptr;
	out->constant = false;
	out->ptr = nullptr;
	out->size = 0;

	orcus_uint offset = 0;
	while (offset < F->numberofinstructions) {
		orcus_opcode instruction =F->instructions[offset];
		offset++;
		char buffer[0x2FFF];

#ifdef ORCUS_64BIT
	snprintf(buffer, 0x2FFF, "[%016zX - %04hX] %s", offset - 1, instruction, m_get_opcode_str(instruction));
#else
	snprintf(buffer, 0x2FFF, "[%08zX - %04hX] %s", offset - 1, instruction, m_get_opcode_str(instruction));
#endif
		
		orcus_mstring_addchars(out, buffer, strlen(buffer) );

		switch (instruction) {
			case ORCUS_OPCODE_NINT: //static var inputs
				snprintf(buffer, 0x2FFF, " %zi", *(orcus_int*)(F->instructions + offset));
				orcus_mstring_addchars(out, buffer, strlen(buffer));
				offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
				break;
			case ORCUS_OPCODE_NFLO:
				snprintf(buffer, 0x2FFF, " %f", *(orcus_float*)(F->instructions + offset));
				orcus_mstring_addchars(out, buffer, strlen(buffer));
				offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
				break;
			case ORCUS_OPCODE_NSTR:
				{
				orcus_int size = *(orcus_int*)(F->instructions + offset);
				snprintf(buffer, 0x2FFF, " size:%zi ", size );
				orcus_mstring_addchars(out, buffer, strlen(buffer));
				offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
				size = size >= 0x2FFF ? 0x2FFF : size;
				for (orcus_int i = 0; i < size; i++) {
					char c = *(((char*)(F->instructions + offset)) + i);
					buffer[i] = c;
				}
				offset += (size + sizeof(orcus_opcode)-1) / sizeof(orcus_opcode);
				orcus_mstring_addchars(out, (char*)"\"", 1);
				orcus_mstring_addchars(out, buffer, size);
				orcus_mstring_addchars(out, (char*)"\"", 1);
				}
				break;
			case ORCUS_OPCODE_NFUN:
				snprintf(buffer, 0x2FFF, " instructions:%zi ", *(orcus_int*)(F->instructions + offset));
				orcus_mstring_addchars(out, buffer, strlen(buffer));
				offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
				break;
			case ORCUS_OPCODE_CALL:
				snprintf(buffer, 0x2FFF, " in:%zi ", *(orcus_int*)(F->instructions + offset));
				orcus_mstring_addchars(out, buffer, strlen(buffer));
				offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;	
				snprintf(buffer, 0x2FFF, " out:%zi ", *(orcus_int*)(F->instructions + offset));
				orcus_mstring_addchars(out, buffer, strlen(buffer));
				offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
				break;
			case ORCUS_OPCODE_JMP: //gaurentee takes 1, int
			case ORCUS_OPCODE_JMPR:
			case ORCUS_OPCODE_JIF:
			case ORCUS_OPCODE_JNIF:
			case ORCUS_OPCODE_JRIF:
			case ORCUS_OPCODE_JRNI:
				snprintf(buffer, 0x2FFF, " offset:%zi ", *(orcus_int*)(F->instructions + offset));
				orcus_mstring_addchars(out, buffer, strlen(buffer));
				offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
				break;
			case ORCUS_OPCODE_CPYX:
			case ORCUS_OPCODE_SCON:
			case ORCUS_OPCODE_GCON:
				snprintf(buffer, 0x2FFF, " %zi ", *(orcus_int*)(F->instructions + offset));
				orcus_mstring_addchars(out, buffer, strlen(buffer));
				offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
				break;
			case ORCUS_OPCODE_SWXY:
				snprintf(buffer, 0x2FFF, " %zu ", *(orcus_int*)(F->instructions + offset));
				orcus_mstring_addchars(out, buffer, strlen(buffer));
				offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
				snprintf(buffer, 0x2FFF, " %zu ", *(orcus_int*)(F->instructions + offset));
				orcus_mstring_addchars(out, buffer, strlen(buffer));
				offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
				break;
			case ORCUS_OPCODE_SETL_KCS:
			case ORCUS_OPCODE_GETL_KCS:
			case ORCUS_OPCODE_SETG_KCS:
			case ORCUS_OPCODE_GETG_KCS:
			case ORCUS_OPCODE_SENV_KCS:
			case ORCUS_OPCODE_GENV_KCS:
			case ORCUS_OPCODE_SET_KCS:
			case ORCUS_OPCODE_GET_KCS:
				{
				orcus_int size = *(orcus_int*)(F->instructions + offset);
				snprintf(buffer, 0x2FFF, " size:%zi ", size);
				orcus_mstring_addchars(out, buffer, strlen(buffer));
				offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
				size = size >= 0x2FFF ? 0x2FFF : size;
				for (orcus_int i = 0; i < size; i++) {
					char c = *(((char*)(F->instructions + offset)) + i);
					buffer[i] = c;
				}
				offset += (size + sizeof(orcus_opcode) - 1) / sizeof(orcus_opcode);
				orcus_mstring_addchars(out, (char*)"\"", 1);
				orcus_mstring_addchars(out, buffer, size);
				orcus_mstring_addchars(out, (char*)"\"", 1);
				}
				break;
			case ORCUS_OPCODE_MSET:
			{
				orcus_int size = *(orcus_int*)(F->instructions + offset);
				snprintf(buffer, 0x2FFF, " elements:%zi ", size);
				orcus_mstring_addchars(out, buffer, strlen(buffer));
				offset += ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE;
			}
		}

		orcus_mstring_addchars(out,(char*)"\n",1);
	}
	return out;
}

//same as above, but does not auto-advance or detect variables for alternate debugging
orcus_string* orcus_get_bytecode_rawbinary_debug(orcus_function* F) {
	orcus_string* out = (orcus_string*)malloc(sizeof(orcus_string));
	if (!out)return nullptr;
	out->constant = false;
	out->ptr = nullptr;
	out->size = 0;

	orcus_uint offset = 0;
	while (offset < F->numberofinstructions) {
		orcus_opcode instruction = F->instructions[offset];
		offset++;
		char buffer[0x2FFF];

		char charbuffer[3]="\0\0";

		charbuffer[0]= instruction & 0xFF;
		charbuffer[0] = (charbuffer[0] < '\x20') ? charbuffer[0] = '\x20' : charbuffer[0]; //prevent control characters messing everything up, since this normally gets printed to stdout.
		charbuffer[1]= (instruction >> 8) & 0xFF;
		charbuffer[1] = (charbuffer[1] < '\x20') ? charbuffer[1] = '\x20' : charbuffer[1];

#ifdef ORCUS_64BIT
		snprintf(buffer, 0x2FFF, "[%016zX - %04hX\t%s\t%hi\t%hu\t%s]", offset - 1, instruction, m_get_opcode_str(instruction), instruction, instruction,charbuffer);
#else
		snprintf(buffer, 0x2FFF, "[%08zX - %04hX\t%s\t%hi\t%hu\t%s]", offset - 1, instruction, m_get_opcode_str(instruction), instruction, instruction, charbuffer);
#endif

		orcus_mstring_addchars(out, buffer, strlen(buffer));
		orcus_mstring_addchars(out, (char*)"\n", 1);
	}
	return out;
}


bool orcus_register_library(void* data, const char* key, const char* table,uint8_t type) {
	void* nreg=realloc(ORCUS_LIBS,sizeof(orcus_libdef)*(ORCUS_NUM_LIBS+1) );
	if (!nreg)return false;
	ORCUS_LIBS = (orcus_libdef*)nreg;

	orcus_libdef* ndef = ORCUS_LIBS + ORCUS_NUM_LIBS;
	ndef->dataptr = data;
	ndef->key = key;
	ndef->table = table;
	ndef->type = type;

	ORCUS_NUM_LIBS++;
	return true;
}

orcus_libdef* ORCUS_LIBS = nullptr;
orcus_int ORCUS_NUM_LIBS = 0;




void orcus_populate_table_with_libs(orcus_table* T) {

	for (orcus_int i = 0; i < ORCUS_NUM_LIBS; i++) {
		orcus_libdef* lib = ORCUS_LIBS+i;
		
		orcus_variable v;
		v.type = lib->type;
		switch (lib->type)
		{
		case ORCUS_TYPE_FLOAT:
			v.data.f = *(orcus_float*)lib->dataptr;
			break;
		case ORCUS_TYPE_INT:
		case ORCUS_TYPE_BOOL:
			v.data.i = *(orcus_int*)lib->dataptr;
			break;
		case ORCUS_TYPE_CFUNC:
			v.data.p = (orcus_cfunc)lib->dataptr;
			break;
		case ORCUS_TYPE_NIL:
			v.data.p = lib->dataptr;
			break;
		default:
			continue;
		}

		if (lib->table) {		
			orcus_variable t;
			orcus_table_get_cstring_keyvalue(T, lib->table, &t);

			if (t.type == ORCUS_TYPE_TABLE) {
				orcus_table_set_cstring_keyvalue((orcus_table*)t.data.p, lib->key, &v);
			}
			else {
				orcus_table* nt=orcus_newtable();
				orcus_table_set_cstring_keyvalue(nt, lib->key, &v);

				orcus_variable vv;
				vv.type = ORCUS_TYPE_TABLE;
				vv.data.p = nt;

				orcus_table_set_cstring_keyvalue(T, lib->table, &vv);
			}


		}
		else {
			orcus_table_set_cstring_keyvalue(T, lib->key, &v);
		}


	}


}


//use dll loading stuff to register libraries. simple and easy.

#ifdef _WIN32
BOOL WINAPI DllMain(HMODULE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
#else
static void __attribute__((constructor)) dynamic_lib_load() {
#endif

	static const orcus_int v = ORCUS_VERSION;
	orcus_register_library((void*)&v, "_VERSION", nullptr,ORCUS_TYPE_INT);
	static const orcus_int v_p = ORCUS_VERSION_PATCH;
	orcus_register_library((void*)&v_p, "_VERSION_PATCH", nullptr, ORCUS_TYPE_INT);

#ifdef ORCUS_LIB_STD
	orcus_register_library(orcus_lib_std_print, "print", nullptr);
	orcus_register_library(orcus_lib_std_iterate, "iterate", nullptr);
	orcus_register_library(orcus_lib_std_restricted_call, "rcall", nullptr);
	orcus_register_library(orcus_lib_std_dump, "dump", nullptr);
	orcus_register_library(orcus_lib_std_compile, "compile", nullptr);
	orcus_register_library(orcus_lib_std_type, "type", nullptr);
	orcus_register_library(orcus_lib_std_tostring, "tostring", nullptr);
	orcus_register_library(orcus_lib_std_tonumber, "tonumber", nullptr);
	orcus_register_library(orcus_lib_std_toint, "toint", nullptr);
	orcus_register_library(orcus_lib_std_tofloat, "tofloat", nullptr);
	orcus_register_library(orcus_lib_std_dynamic_library_load, "loadlibrary", nullptr);
	orcus_register_library(orcus_lib_std_deepcopy, "deepcopy", nullptr);
	orcus_register_library(orcus_lib_std_error, "error", nullptr);
	orcus_register_library(orcus_lib_std_addlibs, "addlibs", nullptr);
	orcus_register_library(orcus_lib_std_sleep, "sleep", nullptr);


	orcus_register_library((void*)&orcus_const_type_nil, "TYPE_NIL", nullptr, ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_const_type_int, "TYPE_INT", nullptr, ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_const_type_float, "TYPE_FLOAT", nullptr, ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_const_type_bool, "TYPE_BOOL", nullptr, ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_const_type_table, "TYPE_TABLE", nullptr, ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_const_type_string, "TYPE_STRING", nullptr, ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_const_type_cfunc, "TYPE_CFUNC", nullptr, ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_const_type_array, "TYPE_ARRAY", nullptr, ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_const_type_function, "TYPE_FUNCTION", nullptr, ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_const_type_file, "TYPE_FILE", nullptr, ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_const_type_thread, "TYPE_THREAD", nullptr, ORCUS_TYPE_INT);
#endif

#ifdef ORCUS_LIB_MATH
	orcus_register_library(orcus_lib_math_max, "max", "math");
	orcus_register_library(orcus_lib_math_min, "min", "math");
	orcus_register_library(orcus_lib_math_floor, "floor", "math");
	orcus_register_library(orcus_lib_math_ceil, "ceil", "math");
	orcus_register_library(orcus_lib_math_to_radians, "radians", "math");
	orcus_register_library(orcus_lib_math_to_degrees, "degrees", "math");
	orcus_register_library(orcus_lib_math_log, "log", "math");
	orcus_register_library(orcus_lib_math_to_absolute, "abs", "math");
	orcus_register_library(orcus_lib_math_to_sin, "sin", "math");
	orcus_register_library(orcus_lib_math_to_cos, "cos", "math");
	orcus_register_library(orcus_lib_math_to_tan, "tan", "math");
	orcus_register_library(orcus_lib_math_to_asin, "asin", "math");
	orcus_register_library(orcus_lib_math_to_acos, "acos", "math");
	orcus_register_library(orcus_lib_math_to_atan, "atan", "math");
	orcus_register_library(orcus_lib_math_to_atan2, "atan2", "math");
	orcus_register_library(orcus_lib_math_random, "random", "math");
	orcus_register_library(orcus_lib_math_randomint, "randomint", "math");
	orcus_register_library(orcus_lib_math_randomseed, "randomseed", "math");
	orcus_register_library(orcus_lib_math_isnan, "isnan", "math");
	orcus_register_library(orcus_lib_math_max_array, "amax", "math");
	orcus_register_library(orcus_lib_math_min_array, "amin", "math");
	orcus_register_library(orcus_lib_math_mean, "mean", "math");
	orcus_register_library(orcus_lib_math_mean_array, "amean", "math");
	orcus_register_library(orcus_lib_math_stddev, "stddev", "math");
	orcus_register_library(orcus_lib_math_stddev_array, "astddev", "math");
	orcus_register_library(orcus_lib_math_median, "median", "math");
	orcus_register_library(orcus_lib_math_median_array, "amedian", "math");

	orcus_register_library((void*)&orcus_math_pi, "pi", "math", ORCUS_TYPE_FLOAT);
	orcus_register_library((void*)&orcus_math_root2, "root2", "math", ORCUS_TYPE_FLOAT);
	orcus_register_library((void*)&orcus_math_e, "e", "math", ORCUS_TYPE_FLOAT);
	orcus_register_library((void*)&orcus_math_root3, "root3", "math", ORCUS_TYPE_FLOAT);
	orcus_register_library((void*)&orcus_math_golden, "golden", "math", ORCUS_TYPE_FLOAT);

	orcus_register_library((void*)&orcus_math_intmax, "int_max", "math", ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_math_uintmax, "uint_max", "math", ORCUS_TYPE_INT);
#endif
#ifdef ORCUS_LIB_ARRAY
	orcus_register_library(orcus_lib_array_flush, "flush", "array");
	orcus_register_library(orcus_lib_array_copy, "copy", "array");
	orcus_register_library(orcus_lib_array_insert, "insert", "array");
	orcus_register_library(orcus_lib_array_remove, "remove", "array");
	orcus_register_library(orcus_lib_array_swap, "swap", "array");
	orcus_register_library(orcus_lib_array_sort, "sort", "array");
	orcus_register_library(orcus_lib_array_concat, "concat", "array");

	orcus_register_library((void*)&orcus_sort_greater_to_lesser, "SORTING_GREATER_TO_LESSER", "array", ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_sort_lesser_to_greater, "SORTING_LESSER_TO_GREATER", "array", ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_sort_greater_to_lesser_absolute, "SORTING_GREATER_TO_LESSER_MAGNITUDE", "array", ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_sort_lesser_to_greater_absolute, "SORTING_LESSER_TO_GREATER_MAGNITUDE", "array", ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_sort_alphabet_az, "SORTING_ALPHABETICAL_A_TO_Z", "array", ORCUS_TYPE_INT);
	orcus_register_library((void*)&orcus_sort_alphabet_za, "SORTING_ALPHABETICAL_Z_TO_A", "array", ORCUS_TYPE_INT);
#endif
#ifdef ORCUS_LIB_STRING
	orcus_register_library(orcus_lib_string_sub, "sub", "string");
	orcus_register_library(orcus_lib_string_reverse, "reverse", "string");
	orcus_register_library(orcus_lib_string_find, "find", "string");
	orcus_register_library(orcus_lib_string_replace, "replace", "string");
	orcus_register_library(orcus_lib_string_count, "count", "string");
	orcus_register_library(orcus_lib_string_toarray, "toarray", "string");
	orcus_register_library(orcus_lib_string_fromarray, "fromarray", "string");
	orcus_register_library(orcus_lib_string_separate, "separate", "string");
	orcus_register_library(orcus_lib_string_upper, "upper", "string");
	orcus_register_library(orcus_lib_string_lower, "lower", "string");
	orcus_register_library(orcus_lib_string_format, "format", "string");
	orcus_register_library(orcus_lib_string_p_find, "pfind", "string");
	orcus_register_library(orcus_lib_string_p_extract, "pextract", "string");
	orcus_register_library(orcus_lib_string_p_replace, "preplace", "string");
	orcus_register_library(orcus_lib_string_p_count, "pcount", "string");
	orcus_register_library(orcus_lib_string_escape_orcus, "escape", "string");
	orcus_register_library(orcus_lib_string_escape_url, "escape_url", "string");
	orcus_register_library(orcus_lib_string_escape_c, "escape_c", "string");
	orcus_register_library(orcus_lib_string_escape_html, "escape_html", "string");
	orcus_register_library(orcus_lib_string_copy_string, "copy", "string");
	orcus_register_library(orcus_lib_string_match_function, "match", "string");
	orcus_register_library(orcus_lib_string_p_match_function, "pmatch", "string");
	orcus_register_library(orcus_lib_string_repeat_chars, "repeat", "string");
	orcus_register_library(orcus_lib_string_set_chars, "setchars", "string");

#endif
#ifdef ORCUS_LIB_OS
	orcus_register_library(orcus_lib_os_time, "time", "os");
	orcus_register_library(orcus_lib_os_execute, "execute", "os");
	orcus_register_library(orcus_lib_os_call, "call", "os");
	orcus_register_library(orcus_lib_os_clock, "clock", "os");
	orcus_register_library(orcus_lib_os_getdate, "get_date", "os");
	orcus_register_library(orcus_lib_os_gettime, "get_time", "os");
	orcus_register_library((void*)&orcus_os_isposix, "IS_POSIX", "os", ORCUS_TYPE_BOOL);
	orcus_register_library((void*)&orcus_os_isposix, "IS_UNIX", "os", ORCUS_TYPE_BOOL);
	orcus_register_library((void*)&orcus_os_is64bit, "IS_64BIT", "os", ORCUS_TYPE_BOOL);
	orcus_register_library(orcus_lib_os_exit, "exit", "os");
#endif
#ifdef ORCUS_LIB_IO
	orcus_register_library(orcus_lib_io_open, "open", "io");
	orcus_register_library(orcus_lib_io_read, "read", "io");
	orcus_register_library(orcus_lib_io_close, "close", "io");
	orcus_register_library(orcus_lib_io_write, "write", "io");
	orcus_register_library(orcus_lib_io_getfiles, "getfiles", "io");
	orcus_register_library(orcus_lib_io_getdirs, "getdirs", "io");
	orcus_register_library(orcus_lib_io_lines, "lines", "io");
	orcus_register_library(orcus_lib_io_post, "post", "io");
	orcus_register_library(orcus_lib_io_prompt, "prompt", "io");
	orcus_register_library(orcus_lib_io_input, "input", "io");
	orcus_register_library(orcus_lib_io_remove, "remove", "io");
	orcus_register_library(orcus_lib_io_removedir, "removedir", "io");
	orcus_register_library(orcus_lib_io_createdir, "createdir", "io");
	orcus_register_library(orcus_lib_io_serialize, "serialize", "io");
	orcus_register_library(orcus_lib_io_deserialize, "deserialize", "io");
	orcus_register_library(orcus_lib_io_cwd, "getworkingdir", "io");
	orcus_register_library(orcus_lib_io_set_cwd, "setworkingdir", "io");
	orcus_register_library(orcus_lib_io_executabledirectory, "getexecutabledir", "io");
	orcus_register_library(orcus_lib_io_userdirectory, "getuserdir", "io");
	orcus_register_library(orcus_lib_io_filelength, "size", "io");


#endif
#ifdef ORCUS_LIB_THREAD
	orcus_register_library(orcus_lib_thread_new, "new", "thread");
	orcus_register_library(orcus_lib_thread_checkfinish, "isfinished", "thread");
	orcus_register_library(orcus_lib_thread_checkrunning, "isrunning", "thread");
	orcus_register_library(orcus_lib_thread_getvalue, "fetch", "thread");
	orcus_register_library(orcus_lib_thread_abort, "abort", "thread");
	orcus_register_library(orcus_lib_thread_getnumvalues, "getcount", "thread");
	orcus_register_library(orcus_lib_thread_waitfor, "await", "thread");
	orcus_register_library(orcus_lib_thread_break, "break", "thread");
	orcus_register_library(orcus_lib_thread_check_error, "checkerror", "thread");
#endif
#ifdef ORCUS_LIB_TABLE
	orcus_register_library(orcus_lib_table_copy, "copy", "table");
	orcus_register_library(orcus_lib_table_flush, "flush", "table");
	orcus_register_library(orcus_lib_table_getkeys, "getkeys", "table");
	orcus_register_library(orcus_lib_table_getvalues, "getvalues", "table");
	orcus_register_library(orcus_lib_table_haskey, "haskey", "table");
	orcus_register_library(orcus_lib_table_hasvalue, "hasvalue", "table");
	orcus_register_library(orcus_lib_table_keyswithvalue, "keyswithvalue", "table");
#endif
#ifdef ORCUS_LIB_DEBUG
	orcus_register_library(orcus_lib_debug_stack_dbg, "dumpstack", "debug");
	orcus_register_library(orcus_lib_debug_state_dbg, "dumpstate", "debug");
	orcus_register_library(orcus_lib_debug_enviroment_dbg, "dumpenv", "debug");
	orcus_register_library(orcus_lib_debug_constants_dbg, "dumpconstants", "debug");
	orcus_register_library(orcus_lib_debug_bytecode_dbg, "dumpbytecode", "debug");
	orcus_register_library(orcus_lib_debug_bytecode_rawbinary_dbg, "dumprawbytecode", "debug");
	orcus_register_library(orcus_lib_debug_refcount_dbg, "showrefs", "debug");
	orcus_register_library(orcus_lib_debug_dump_debug_info_dbg, "dumpdebuginfo", "debug");
	orcus_register_library(orcus_lib_debug_get_traceback, "traceback", "debug");
#endif

#ifdef _WIN32
	}
return TRUE;
}
#else
}
#endif
