#include "../orcus.hpp"
#include "../orcus_error.hpp"

#include <cstdlib>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <pthread.h>
#endif




#ifdef _WIN32

//what the fuck is an LPVOID?
DWORD WINAPI threadfunction(LPVOID param) {
	if (!param)return 0;
	orcus_threadholder* threadvar= (orcus_threadholder*)param;

	orcus_int num_start_args = threadvar->state->sizeofstack;
	orcus_variable* start_args = (orcus_variable*)malloc(sizeof(orcus_variable) * num_start_args);
	if (start_args) {
		memcpy(start_args, threadvar->state->stack, sizeof(orcus_variable) * num_start_args);//copy initial stack
		if (threadvar->cfunction) {
			threadvar->cfunction(threadvar->state, threadvar->state->sizeofstack, -1);
		}
		else {
			while (orcus_stepstate(threadvar->state));
		}

		//because we increment the ref count before opening the thread, the thread must then decrement, and check for a free
		for (orcus_int n = 0; n < num_start_args; n++) {
			orcus_decrement_variable_refrence_count(start_args + n);
			orcus_release_var(start_args + n);
		}
		free(start_args);

		if (threadvar->state->errorcode) { //clear stack on error to prevent drawing of args that shouldn't be drawn.
			while (threadvar->state->sizeofstack) {
				orcus_discard_top_of_stack(threadvar->state);
			}
		}
	}
	else {
		orcus_raise_error(threadvar->state, ORCUS_ERROR_ALLOCATION);
	}
	threadvar->finished = true;
	return 0;
}

#else
void* threadfunction(void* param) {
	if (!param)return nullptr;
	orcus_threadholder* threadvar = (orcus_threadholder*)param;

	if (threadvar->cfunction) {
		threadvar->cfunction(threadvar->state, threadvar->state->sizeofstack, 1);
	}
	else {
		while (orcus_stepstate(threadvar->state));
	}

	if (threadvar->state->errorcode) { //clear stack on error to prevent drawing of args that shouldn't be drawn.
		while (threadvar->state->sizeofstack) {
			orcus_discard_top_of_stack(threadvar->state);
		}
	}

	threadvar->finished = true;
	return nullptr;
}
#endif




//creates a thread, given a function and input variable. optional table to act as the enviroment. defaults current env.
void orcus_lib_thread_new(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (!args_in) {
		orcus_raise_error_nonpointer(M, ORCUS_ERROR_NOT_ENOUGH_ARGS, args_in, 1);
		return;
	}
	if (!args_out) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0);
		return;
	}


	orcus_variable table_var;
	orcus_variable func_var;

	orcus_variable* vart = nullptr;
	if (args_in > 2) {
		vart = (orcus_variable*)malloc(sizeof(orcus_variable) * args_in - 2);
		if (!vart && args_in>2) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
	}
	

	for (orcus_int i = 2; i < args_in; i++) {
		orcus_popstack(M, vart+i - 2);
	}


	if (args_in >=2) {
		orcus_popstack(M, &table_var);
		orcus_popstack(M,&func_var);
	}
	else {
		orcus_popstack(M, &func_var);
		table_var.type = ORCUS_TYPE_NIL;
	}


	if ( func_var.type != ORCUS_TYPE_FUNCTION && func_var.type != ORCUS_TYPE_CFUNC) {
		free(vart);
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_EXPECTS_ANY_FUNCTION_VARIABLEPROVIDED, &func_var, 1);
		orcus_release_var(&func_var);
		return;
	}
	if ((table_var.type && table_var.type != ORCUS_TYPE_TABLE)) {
		free(vart);
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &table_var, ORCUS_TYPE_TABLE, 2);
		orcus_release_var(&table_var);
		return;
	}


	orcus_threadholder* t=(orcus_threadholder*)malloc(sizeof(orcus_threadholder));
	if (!t) {
		free(vart);
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}

	t->finished = false;
	t->references = 0;
	if (table_var.type) {
		t->state = orcus_newstate();
		t->customenv = true;
		if (t->state) {
			orcus_destroytable(t->state->enviroment);
			t->state->enviroment = (orcus_table*)table_var.data.p;
			t->state->enviroment->references++;
			//printf("making with custom\n");
		}
	}
	else {
		//printf("making with parent\n");
		t->state = orcus_newstate(M);
		t->customenv = false;
	}
	
	orcus_variable out;
	out.data.i = 0;
	out.type = ORCUS_TYPE_NIL;
	
	if (t->state) {
		
		if (func_var.type == ORCUS_TYPE_FUNCTION) {
			t->cfunction = nullptr;
			t->state->bytecode = (orcus_function*)malloc(sizeof(orcus_function));
			if (!t->state->bytecode) {
				orcus_destroystate(t->state);
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			orcus_clone_function((orcus_function*)func_var.data.p, t->state->bytecode);
			if (((orcus_function*)func_var.data.p)->numberofinstructions != t->state->bytecode->numberofinstructions) {
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
				return;
			}
			t->state->bytecode->references = 1;
		}
		else { //C function
			t->cfunction = (orcus_cfunc)func_var.data.p;
		}

		if (vart) {
			if (func_var.type == ORCUS_TYPE_FUNCTION) {
				for (orcus_int i = 0; i < args_in - 2; i++) {
					orcus_increment_variable_refrence_count(vart + i); //increment ref count to prevent the variable being freed while the thread is using it, if this current state undergoes a closure.
					orcus_pushstack(t->state, vart + i);
				}
			}
			else {//C function. reverse order.
				for (orcus_int i = args_in - 2-1;i>=0; i--) {
					orcus_increment_variable_refrence_count(vart + i);
					orcus_pushstack(t->state, vart + i);
				}
			}
			free(vart);
			vart = nullptr;
		}

#ifdef _WIN32

		HANDLE h = CreateThread(NULL, 0, threadfunction, (LPVOID)t, 0, NULL);
		if (h) {
			t->threadobject = h;
			out.type = ORCUS_TYPE_THREAD;
			out.data.p = t;
		}
		else {
			if (table_var.type)t->state->enviroment = nullptr; //if we set a custom env, don't destroy the data.
			orcus_release_var(&table_var);
			//t->state->bytecode.instructions = nullptr; //don't mess with the host function's data
			orcus_destroystate(t->state);
			free(t);
		}

#else

		int c=pthread_create(&(t->threadobject), NULL, threadfunction, t);
		if (!c) {
			out.type = ORCUS_TYPE_THREAD;
			out.data.p = t;
		}
		else {
			if (table_var.type)t->state->enviroment = nullptr; //if we set a custom env, don't destroy the data.
			//t->state->bytecode.instructions = nullptr; //don't mess with the host function's data
			orcus_release_var(&table_var);
			orcus_destroystate(t->state);
			free(t);
		}


#endif


	}

	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


//bool if thread is finished. simple.
void orcus_lib_thread_checkfinish(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}


	orcus_variable in;
	orcus_popstack(M,&in);
	if (in.type != ORCUS_TYPE_THREAD) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_THREAD, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_variable out;
	out.type = ORCUS_TYPE_BOOL;
	out.data.i = ((orcus_threadholder*)in.data.p)->finished ? 1 : 0;

	orcus_release_var(&in);
	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



//gets a value
void orcus_lib_thread_getvalue(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable in;
	orcus_popstack(M, &in);
	if (in.type != ORCUS_TYPE_THREAD) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_THREAD, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_threadholder* t=(orcus_threadholder*)in.data.p;
	if (!t->finished) {
#if defined(_WIN32) || defined(_WIN64)
		WaitForSingleObject(t->threadobject, INFINITE);
		CloseHandle(t->threadobject);
		t->threadobject = NULL;
#else
		pthread_join(t->threadobject, NULL);
		t->threadobject = NULL;
#endif
	}
	orcus_release_var(&in);
	orcus_variable out;
	orcus_pullstack(t->state, &out); //take the bottom of stack. it's the proper order with returns.
	orcus_pushstack(M, &out);


	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


//dire circumstances. use with caution
void orcus_lib_thread_abort(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;

	orcus_variable in;
	orcus_popstack(M,&in);
	if (in.type != ORCUS_TYPE_THREAD) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_THREAD, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_threadholder* t = (orcus_threadholder*)in.data.p;
	if (!t->finished) {
#if defined(_WIN32) || defined(_WIN64)
		TerminateThread(t->threadobject, 0);
		CloseHandle(t->threadobject);
		t->threadobject = NULL;
#else
		pthread_cancel(t->threadobject);
		pthread_join(t->threadobject, NULL);
		t->threadobject = NULL;
#endif
	}
	t->finished = true;

	orcus_release_var(&in);
	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 0);
}



//returns number of values. 0 if not finished.
void orcus_lib_thread_getnumvalues(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable in;
	orcus_popstack(M,&in);
	if (in.type != ORCUS_TYPE_THREAD) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_THREAD, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_threadholder* t = (orcus_threadholder*)in.data.p;

	orcus_variable out;
	out.type = ORCUS_TYPE_INT;
	out.data.i = 0;

	if (t->finished) {
		out.data.i = t->state->sizeofstack;
	}
	
	orcus_release_var(&in);
	orcus_pushstack(M, &out);


	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



//waits for a closure
void orcus_lib_thread_waitfor(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;

	orcus_variable in;
	orcus_popstack(M,&in);
	if (in.type != ORCUS_TYPE_THREAD) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_THREAD, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_threadholder* t = (orcus_threadholder*)in.data.p;
	if (!t->finished) {
#if defined(_WIN32) || defined(_WIN64)
		WaitForSingleObject(t->threadobject, INFINITE);
		CloseHandle(t->threadobject);
		t->threadobject = NULL;
#else
		pthread_join(t->threadobject, NULL);
		t->threadobject = NULL;
#endif
	}

	orcus_release_var(&in);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 0);
}


//inverse of is finished.
void orcus_lib_thread_checkrunning(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable in;
	orcus_popstack(M,&in);
	if (in.type != ORCUS_TYPE_THREAD) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_THREAD, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_variable out;
	out.type = ORCUS_TYPE_BOOL;
	out.data.i = ((orcus_threadholder*)in.data.p)->finished ? 0 : 1;

	orcus_release_var(&in);
	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


//tries to abort execution on a thread without leaking as much.
void orcus_lib_thread_break(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;

	orcus_variable in;
	orcus_popstack(M, &in);
	if (in.type != ORCUS_TYPE_THREAD) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_THREAD, 1);
		orcus_release_var(&in);
		return;
	}

	orcus_threadholder* t = (orcus_threadholder*)in.data.p;
	if (!t->finished) {
		t->state->errorcode = ORCUS_ERROR_EXECUTION_ABORT;
		t->state->programcounter = t->state->bytecode->numberofinstructions;
	}

	orcus_release_var(&in);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
}


void orcus_lib_thread_check_error(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1))return;
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable in;
	orcus_popstack(M, &in);
	if (in.type != ORCUS_TYPE_THREAD) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_THREAD, 1);
		orcus_release_var(&in);
		return;
	}
	orcus_threadholder* t = (orcus_threadholder*)in.data.p;
	uint32_t ec = t->state->errorcode;
	orcus_release_var(&in);
	if (!args_out)return;

	in.type = ORCUS_TYPE_BOOL;
	in.data.i = ec != 0;
	orcus_pushstack(M,&in);


	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}