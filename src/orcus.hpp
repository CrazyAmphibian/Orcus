#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

#ifdef _WIN32

#ifdef ORCUS_DYNAMIC_LIBRARY_COMPILE
#define ORCUS_DYNAMIC_LIBRARY __declspec(dllexport)
#else
#define ORCUS_DYNAMIC_LIBRARY __declspec(dllimport)
#endif

#else
#define ORCUS_DYNAMIC_LIBRARY __attribute__((visibility("default")))
#endif 

//gee i sure wish C++ had this as a standard keyword
#ifdef __GNUC__
#define ORCUS_CPP_restrict __restrict__
#else
#ifdef _MSC_VER
#define ORCUS_CPP_restrict __restrict
#else
#define ORCUS_CPP_restrict
#endif
#endif


#if INT64_MAX==INTPTR_MAX
typedef int64_t orcus_int; //typedefs to ensure that our variables occupy the same space in memory.
typedef uint64_t orcus_uint;
typedef double orcus_float;
#define ORCUS_64BIT
#else
typedef int32_t orcus_int;
typedef uint32_t orcus_uint;
typedef float orcus_float;
#define ORCUS_32BIT
#endif

#define ORCUS_VERSION 7
#define ORCUS_VERSION_PATCH 0

#define ORCUS
#if defined(DEBUG) || defined(_DEBUG)
#define ORCUS_DEBUG
#endif

typedef uint16_t orcus_opcode;
constexpr size_t ORCUS_INSTRUCTIONS_PER_VARIABLE_SIZE = sizeof(orcus_int) / sizeof(orcus_opcode); //how many instructions fit into one variable. instructions are 16 bits, so on a 64 bit system this is 4. on a 32 bit system, this is 2. if for some reason you're running a 16 bit system, this is 1.

union orcus_rawdata { //to represent stored binary data of almost any type.
	orcus_int i;
	orcus_uint u;
	orcus_float f;
	void* p;
};

struct orcus_variable {
	uint8_t type = 0;
	orcus_rawdata data;
};

struct orcus_string {
	orcus_int size=0;
	orcus_uint references = 0;
	char* ptr = nullptr;
	bool constant = false; //if true, ptr points to another char*, and so we should not free it. optimization to reduce malloc calls.
};

struct orcus_subtable {
	orcus_int size = 0;
	orcus_variable* keys = nullptr;
	orcus_variable* values = nullptr;
};

struct orcus_table {
	orcus_subtable* data;
	orcus_uint references = 0;
	bool enviromental = false;
};


//typedef orcus_subtable** orcus_table;

struct orcus_debug_token {
	char* chars=nullptr;
	orcus_int num_chars=0;
	orcus_int col=0;
	orcus_int line=0;
};

struct orcus_array { //gee bill, two storage types?
	orcus_uint references = 0;
#ifdef ORCUS_64BIT
	orcus_variable****** values = nullptr; //array of arrays of arrays of arrays of arrays of arrays of pointers to structs.  array ->  array ->  array ->  array -> array -> array -> struct*. who needs efficency, anyways? hey look pal, you wanted 64 bit indexes, you're gonna get 64 bit indexes.
#else
	orcus_variable*** values = nullptr; //array of arrays of arrays of pointers to structs. array -> array -> array -> struct*
#endif
};

/*
arrays are split into subarrays. this is to make it so that sparse arrays don't eat up all your dedotated wam.
this is done asymetrically, naturally. This is done so that iterating is still somewhat fast for close indexes.
*/
#ifdef ORCUS_64BIT
/*
on a 64 bit system, bits are layed out like this:
<---------------------------64 bits---------------------------->
<10 bits-><10 bits-><10 bits-><10 bits-><--12 bits-><--12 bits->
<most sig ------------------------------------------> least sig>
ensures that worst case, a single variable will consume 12kb 
*/
inline int get_array_index_from_mint_1(orcus_uint i) {
	return (i >> 54);
}
inline int get_array_index_from_mint_2(orcus_uint i) {
	return (i >> 44) & 0b1111111111;
}
inline int get_array_index_from_mint_3(orcus_uint i) {
	return (i >> 34) & 0b1111111111;
}
inline int get_array_index_from_mint_4(orcus_uint i) {
	return (i >> 24) & 0b1111111111;
}
inline int get_array_index_from_mint_5(orcus_uint i) {
	return (i >> 12) & 0b111111111111;
}
inline int get_array_index_from_mint_6(orcus_uint i) {
	return i & 0b111111111111;
}
constexpr int ORCUS_SIZE_SUBARRAY_1 = 0b1111111111 + 1;
constexpr int ORCUS_SIZE_SUBARRAY_2 = 0b1111111111 + 1;
constexpr int ORCUS_SIZE_SUBARRAY_3 = 0b1111111111 + 1;
constexpr int ORCUS_SIZE_SUBARRAY_4 = 0b1111111111 + 1;
constexpr int ORCUS_SIZE_SUBARRAY_5 = 0b111111111111 + 1;
constexpr int ORCUS_SIZE_SUBARRAY_6 = 0b111111111111 + 1;

constexpr int ORCUS_WIDTH_SUBARRAY_1 = 10;
constexpr int ORCUS_WIDTH_SUBARRAY_2 = 10;
constexpr int ORCUS_WIDTH_SUBARRAY_3 = 10;
constexpr int ORCUS_WIDTH_SUBARRAY_4 = 10;
constexpr int ORCUS_WIDTH_SUBARRAY_5 = 12;
constexpr int ORCUS_WIDTH_SUBARRAY_6 = 12;

inline orcus_int orcus_reconstruct_array_index(int i1, int i2, int i3, int i4, int i5, int i6) {
	return (((((((((i1 << ORCUS_WIDTH_SUBARRAY_2) | i2) << ORCUS_WIDTH_SUBARRAY_3) | i3) << ORCUS_WIDTH_SUBARRAY_4) | i4) << ORCUS_WIDTH_SUBARRAY_5) | i5) << ORCUS_WIDTH_SUBARRAY_6) | i6;
}
#else
/*
on a 32 bit system, bits are layed out like this:
<------------32 bits----------->
<10 bits-><10 bits-><--12 bits->
<most sig ----------> least sig>
ensures that worst case, a single variable will consume 6kb
*/
inline int get_array_index_from_mint_1(orcus_uint i) {
	return (i >> 22);
}
inline int get_array_index_from_mint_2(orcus_uint i) {
	return (i >> 12)&0b1111111111;
}
inline int get_array_index_from_mint_3(orcus_uint i) {
	return i&0b111111111111;
}
constexpr int ORCUS_SIZE_SUBARRAY_1 = 0b1111111111 + 1;
constexpr int ORCUS_SIZE_SUBARRAY_2 = 0b1111111111 + 1;
constexpr int ORCUS_SIZE_SUBARRAY_3 = 0b111111111111 + 1;

constexpr int ORCUS_WIDTH_SUBARRAY_1 = 10;
constexpr int ORCUS_WIDTH_SUBARRAY_2 = 10;
constexpr int ORCUS_WIDTH_SUBARRAY_3 = 12;

inline orcus_int orcus_reconstruct_array_index(int i1, int i2, int i3) {
	return (((i1 << ORCUS_WIDTH_SUBARRAY_2) | i2) << ORCUS_WIDTH_SUBARRAY_3) | i3;
}
#endif

struct orcus_function {
	orcus_uint references = 0;
	orcus_uint numberofinstructions = 0;
	orcus_opcode* instructions = nullptr;
	bool enviromental = false;
	orcus_uint* instruction_dbg_lookup=nullptr; //index with an instruction, gives the index to dbg_tokens
	orcus_uint num_dbg_tokens = 0;
	orcus_debug_token* dbg_tokens = nullptr;
};

enum ORCUS_FILEFLAGS :uint8_t { //append and extended mode doesn't really impact the orcus runtime at all, so
	ORCUS_FILEFLAG_READ = 1 << 0,
	ORCUS_FILEFLAG_WRITE = 1 << 1,
};

struct orcus_filewrapper {
	orcus_uint references = 0;
	FILE* file;
	uint8_t modeflags;
};




struct orcus_state {
	orcus_state* parentstate = nullptr; //the parent of this state. nullptr if there is no parent
	orcus_state* masterstate = nullptr; // the parent of the parent of the... this can also be itself.
	orcus_state* childstate = nullptr; // the designated substate used for function calls. ensures we don't have to allocate a bunch of states we don't need. only allocated if needed.

	orcus_uint sizeofstack = 0;  // number of elements on the stack
	orcus_variable* stack = nullptr;
	orcus_uint allocatedstacksize = 0; // the size of the stack array

	orcus_variable* registers = nullptr;
	orcus_variable* constants = nullptr;
	orcus_uint num_constants = 0;

	orcus_uint programcounter = 0;
	orcus_function* bytecode = nullptr;
	uint32_t errorcode = 0;

	orcus_table* enviroment = nullptr;

};


struct orcus_threadholder {
	orcus_uint references = 0;
	volatile bool finished = false; //it could change, you know.
	bool customenv = false;
#ifdef _WIN32
	HANDLE threadobject = NULL;
#else
	pthread_t threadobject = NULL;
#endif
	orcus_state* state = nullptr; //that's right, it carries an enviroment with it. very hilarious, obviously.
};


typedef void (*orcus_cfunc) (orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);

//type defenitions
extern uint8_t ORCUS_NUMBER_OF_TYPES;

enum ORCUS_TYPE_ENUMS:uint8_t {
	ORCUS_TYPE_NIL = 0,
	ORCUS_TYPE_INT = 1,
	ORCUS_TYPE_FLOAT = 2,
	ORCUS_TYPE_BOOL = 3,
	ORCUS_TYPE_TABLE = 4,
	ORCUS_TYPE_STRING = 5,
	ORCUS_TYPE_CFUNC = 6,
	ORCUS_TYPE_ARRAY = 7,
	ORCUS_TYPE_FUNCTION = 8,
	ORCUS_TYPE_FILE = 9,
	ORCUS_TYPE_THREAD=10,
};

extern uint16_t register_max;// = 0xf; //registers rage from 0 to this number


struct orcus_libdef {
	uint8_t type = ORCUS_TYPE_CFUNC;
	void* dataptr = nullptr;
	const char* key = nullptr;
	const char* table = nullptr;
};

extern orcus_libdef* ORCUS_LIBS;
extern orcus_int ORCUS_NUM_LIBS;

//functions defined in the .cpp

//string
ORCUS_DYNAMIC_LIBRARY orcus_string* orcus_cstring_to_mstring(const char* const ORCUS_CPP_restrict str, const orcus_int size);
ORCUS_DYNAMIC_LIBRARY orcus_string* orcus_cstring_const_to_mstring(const char* const ORCUS_CPP_restrict str, const orcus_int size);
ORCUS_DYNAMIC_LIBRARY orcus_string* orcus_cstring_promote_to_mstring(char* const ORCUS_CPP_restrict str, const orcus_int size);
ORCUS_DYNAMIC_LIBRARY char* orcus_mstring_to_cstring(const orcus_string* const ORCUS_CPP_restrict str);
ORCUS_DYNAMIC_LIBRARY orcus_string* orcus_mstrings_concat(const orcus_string* const str1, const orcus_string* const str2);
ORCUS_DYNAMIC_LIBRARY void orcus_mstring_delete(orcus_string* const ORCUS_CPP_restrict str);
ORCUS_DYNAMIC_LIBRARY orcus_string* orcus_mstring_substring(orcus_string* str, orcus_int start, orcus_int end);
ORCUS_DYNAMIC_LIBRARY orcus_string* orcus_tostring(const orcus_variable* const ORCUS_CPP_restrict var);
ORCUS_DYNAMIC_LIBRARY bool orcus_mstrings_append(orcus_string* const basestr, const orcus_string* const appstr);
ORCUS_DYNAMIC_LIBRARY bool orcus_mstring_addchars(orcus_string* const ORCUS_CPP_restrict str, const char* const chars, const orcus_int len=1);
ORCUS_DYNAMIC_LIBRARY orcus_string* orcus_copystring(const orcus_string* const ORCUS_CPP_restrict str);

//table
ORCUS_DYNAMIC_LIBRARY orcus_table* orcus_newtable();
ORCUS_DYNAMIC_LIBRARY void orcus_destroyarray(orcus_array* const ORCUS_CPP_restrict arr);
ORCUS_DYNAMIC_LIBRARY bool orcus_getkey(const orcus_table* const table, orcus_variable* const key, orcus_variable* out);
ORCUS_DYNAMIC_LIBRARY orcus_int orcus_setkey(orcus_table* const table, orcus_variable* const key, const orcus_variable* const value);
ORCUS_DYNAMIC_LIBRARY bool orcus_tables_equal(const orcus_table* const table1, const orcus_table* const table2);
ORCUS_DYNAMIC_LIBRARY orcus_int orcus_tablehaskey(const orcus_table* const table, const orcus_variable* const key);
ORCUS_DYNAMIC_LIBRARY void orcus_destroytable(orcus_table* const table);
ORCUS_DYNAMIC_LIBRARY void orcus_cleartable(const orcus_table* const table);
ORCUS_DYNAMIC_LIBRARY bool orcus_table_get_cstring_keyvalue(const orcus_table* const table, const char* const key, orcus_variable* out);
ORCUS_DYNAMIC_LIBRARY orcus_int orcus_table_set_cstring_keyvalue(orcus_table* const table, const char* const key, const orcus_variable* const value);
ORCUS_DYNAMIC_LIBRARY orcus_int orcus_table_has_cstring_key(const orcus_table* const table, const char* const key);
ORCUS_DYNAMIC_LIBRARY void orcus_prepare_table_for_state(orcus_table* table, orcus_state* M);

//array
ORCUS_DYNAMIC_LIBRARY orcus_array* orcus_newarray();
ORCUS_DYNAMIC_LIBRARY bool orcus_setarray(orcus_array* const array, const orcus_variable* const var, const orcus_int pos);
ORCUS_DYNAMIC_LIBRARY void orcus_getarray(orcus_array* const array, const orcus_int pos, orcus_variable* out);
ORCUS_DYNAMIC_LIBRARY orcus_int orcus_array_len(const orcus_array* const ORCUS_CPP_restrict arr);
ORCUS_DYNAMIC_LIBRARY bool orcus_cleararrayindex(orcus_array* const array, const orcus_int pos);


//state
ORCUS_DYNAMIC_LIBRARY orcus_state* orcus_newstate(const orcus_state* const parent=nullptr);
ORCUS_DYNAMIC_LIBRARY bool orcus_stepstate(orcus_state* const ORCUS_CPP_restrict M);
ORCUS_DYNAMIC_LIBRARY void orcus_clearstate(orcus_state* const ORCUS_CPP_restrict M, bool for_deletion=false);
ORCUS_DYNAMIC_LIBRARY void orcus_destroystate(orcus_state* const ORCUS_CPP_restrict M);

//stack
ORCUS_DYNAMIC_LIBRARY void orcus_popstack(orcus_state* const ORCUS_CPP_restrict M, orcus_variable* out);
ORCUS_DYNAMIC_LIBRARY void orcus_pullstack(orcus_state* const ORCUS_CPP_restrict M, orcus_variable* out);
ORCUS_DYNAMIC_LIBRARY bool orcus_pushstack(orcus_state* const ORCUS_CPP_restrict M, orcus_variable* const var);

ORCUS_DYNAMIC_LIBRARY void orcus_release_var(orcus_variable* const ORCUS_CPP_restrict var);


//misc
ORCUS_DYNAMIC_LIBRARY bool orcus_checkbool(const orcus_variable* const ORCUS_CPP_restrict var);
ORCUS_DYNAMIC_LIBRARY orcus_int orcus_checkint(const orcus_variable* const ORCUS_CPP_restrict var);
ORCUS_DYNAMIC_LIBRARY orcus_float orcus_checkfloat(const orcus_variable* const ORCUS_CPP_restrict var);
ORCUS_DYNAMIC_LIBRARY void* orcus_checkpointer(const orcus_variable* const ORCUS_CPP_restrict var);
ORCUS_DYNAMIC_LIBRARY bool orcus_vars_equal(const orcus_variable* const var1, const orcus_variable* const var2);


ORCUS_DYNAMIC_LIBRARY bool orcus_register_library(void* data,const char* key, const char* table, uint8_t type=ORCUS_TYPE_CFUNC);
ORCUS_DYNAMIC_LIBRARY void orcus_populate_table_with_libs(orcus_table* T);

ORCUS_DYNAMIC_LIBRARY orcus_string* orcus_get_bytecode_debug(orcus_function* F);
ORCUS_DYNAMIC_LIBRARY orcus_string* orcus_get_bytecode_rawbinary_debug(orcus_function* F);
ORCUS_DYNAMIC_LIBRARY void orcus_debugdumptable(orcus_table* tab, int level = 0);

ORCUS_DYNAMIC_LIBRARY void orcus_clone_function(orcus_function* in, orcus_function* out);
ORCUS_DYNAMIC_LIBRARY void orcus_clear_function(orcus_function* in);

//inlines, for SPEED
inline void orcus_clear_variable(orcus_variable* var) {
	var->data.i = 0;
	var->type = ORCUS_TYPE_NIL;
}

//frees the top stack and decrements the stack size. analagous to popstack releasevar, albeit a bit faster.
inline void orcus_discard_top_of_stack(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->sizeofstack) {
		M->sizeofstack--;
		orcus_release_var(M->stack + M->sizeofstack);
	}
}

inline bool orcus_type_is_complex(const uint8_t type) {
	switch (type) {
		case ORCUS_TYPE_NIL:
		case ORCUS_TYPE_INT:
		case ORCUS_TYPE_FLOAT:
		case ORCUS_TYPE_BOOL:
		case ORCUS_TYPE_CFUNC:
			return false;
		case ORCUS_TYPE_STRING:
		case ORCUS_TYPE_ARRAY:
		case ORCUS_TYPE_TABLE:
		case ORCUS_TYPE_FUNCTION:
		case ORCUS_TYPE_THREAD:
		case ORCUS_TYPE_FILE:
		default:
			return true;
	}
}

inline void orcus_increment_variable_refrence_count(const orcus_variable* const ORCUS_CPP_restrict var) {
	switch (var->type) {
	case ORCUS_TYPE_NIL:
	case ORCUS_TYPE_INT:
	case ORCUS_TYPE_FLOAT:
	case ORCUS_TYPE_BOOL:
		return;
	case ORCUS_TYPE_STRING:
		((orcus_string*)var->data.p)->references++;
		return;
	case ORCUS_TYPE_TABLE:
		((orcus_table*)var->data.p)->references++;
		return;
	case ORCUS_TYPE_ARRAY:
		((orcus_array*)var->data.p)->references++;
		return;
	case ORCUS_TYPE_FUNCTION:
		((orcus_function*)var->data.p)->references++;
		return;
	case ORCUS_TYPE_FILE:
		((orcus_filewrapper*)var->data.p)->references++;
		return;
	case ORCUS_TYPE_THREAD:
		((orcus_threadholder*)var->data.p)->references++;
		return;
	}
}
inline void orcus_decrement_variable_refrence_count(const orcus_variable* const ORCUS_CPP_restrict var) {
	switch (var->type) {
	case ORCUS_TYPE_NIL:
	case ORCUS_TYPE_INT:
	case ORCUS_TYPE_FLOAT:
	case ORCUS_TYPE_BOOL:
		return;
	case ORCUS_TYPE_STRING:
		((orcus_string*)var->data.p)->references--;
		return;
	case ORCUS_TYPE_TABLE:
		((orcus_table*)var->data.p)->references--;
		return;
	case ORCUS_TYPE_ARRAY:
		((orcus_array*)var->data.p)->references--;
		return;
	case ORCUS_TYPE_FUNCTION:
		((orcus_function*)var->data.p)->references--;
		return;
	case ORCUS_TYPE_FILE:
		((orcus_filewrapper*)var->data.p)->references--;
		return;
	case ORCUS_TYPE_THREAD:
		((orcus_threadholder*)var->data.p)->references--;
		return;
	}
}

//returns the top stack without removing it. do not free the returned pointer. will return nullptr if stack is 0.
inline void orcus_peek_stack(const orcus_state* ORCUS_CPP_restrict M, orcus_variable* out) {
	if (!M->sizeofstack) {
		orcus_clear_variable(out);
	};
	*out= M->stack[M->sizeofstack - 1];
}

inline orcus_state* orcus_get_child_state(orcus_state* const ORCUS_CPP_restrict M) {
	if (M->childstate)return M->childstate;
	M->childstate=orcus_newstate(M);
	return M->childstate;
}


inline bool orcus_mstrings_equal(const orcus_string* const str1, const orcus_string* const str2) {

	if (str1->size != str2->size) {
		return false;
	}
	if (str1->ptr == str2->ptr)return true;

	for (orcus_int c = 0; c < str1->size; c++) {
		if (str1->ptr[c] != str2->ptr[c]) {
			return false;
		}
	}

	return true;
}

inline bool orcus_mstring_equal_cstring(const orcus_string* const mstr, const char* cstr) {
	if (mstr->ptr == cstr)return true;
	for (orcus_int i = 0; i < mstr->size; i++) {
		if (!cstr[i])return false; //no point in running strlen if we're going to traverse it anyways.
		if (mstr->ptr[i] != cstr[i])return false;
	}
	if (cstr[mstr->size])return false; //should be null terminator here if they are the same.
	return true;
}