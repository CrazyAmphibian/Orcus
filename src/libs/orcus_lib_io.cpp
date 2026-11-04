#include "../orcus.hpp"
#include "../orcus_error.hpp"
#include "orcus_lib_io.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <climits>
#include <errno.h>

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#include <conio.h>
#include <libloaderapi.h>
#include <Shlobj.h>
#include <Shlobj_core.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <pwd.h>
#include <poll.h>
#endif




void orcus_lib_io_open(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //opens a file. nuff said.
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2)) {
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		orcus_discard_top_of_stack(M);
		return;
	};

	orcus_variable mode_var;
	orcus_popstack(M,&mode_var);
	if (mode_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &mode_var, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&mode_var);
		return;
	}


	orcus_variable file_var;
	orcus_popstack(M,&file_var);
	if (file_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &file_var, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&file_var);
		return;
	}

	orcus_variable out;

	char* file = orcus_mstring_to_cstring((orcus_string*)file_var.data.p);
	const char* mode = orcus_mstring_to_cstring((orcus_string*)mode_var.data.p);
	int mode_l = (int)strlen(mode);

	uint8_t flags = 0;

	if (mode_l == 1) {
		char c = mode[0];
		free((char*)mode);
		mode = nullptr;
		if (c == 'r') { mode = "rb"; flags = ORCUS_FILEFLAG_READ; }
		else if (c == 'w') { mode = "wb"; flags = ORCUS_FILEFLAG_WRITE; }
		else if (c == 'a') { mode = "ab"; flags = ORCUS_FILEFLAG_WRITE; }
	}
	else if (mode_l == 2) {
		char c = mode[0];
		char c2 = mode[1];
		free((char*)mode);
		mode = nullptr;
		if (c == 'r') {
			if (c2 == 'b') { mode = "rb"; flags = ORCUS_FILEFLAG_READ; }
			else if (c2 == '+') { mode = "rb+"; flags = ORCUS_FILEFLAG_READ | ORCUS_FILEFLAG_WRITE; }
		}
		else if (c == 'w') {
			if (c2 == 'b') { mode = "wb"; flags = ORCUS_FILEFLAG_WRITE; }
			else if (c2 == '+') { mode = "wb+"; flags = ORCUS_FILEFLAG_READ | ORCUS_FILEFLAG_WRITE; }
		}
		else if (c == 'a') {
			if (c2 == 'b') { mode = "ab"; flags = ORCUS_FILEFLAG_WRITE; }
			else if (c2 == '+') { mode = "ab+"; flags = ORCUS_FILEFLAG_READ | ORCUS_FILEFLAG_WRITE; }
		}
	}
	else if (mode_l == 3) {
		char c = mode[0];
		char c2 = mode[1];
		char c3 = mode[2];
		free((char*)mode);
		mode = nullptr;
		if (c2 == 'b' && c3=='+') {
			if (c == 'r') { mode = "rb+"; flags = ORCUS_FILEFLAG_READ | ORCUS_FILEFLAG_WRITE; }
			else if (c == 'w') { mode = "wb+"; flags = ORCUS_FILEFLAG_READ | ORCUS_FILEFLAG_WRITE; }
			else if (c == 'a') { mode = "ab+"; flags = ORCUS_FILEFLAG_READ | ORCUS_FILEFLAG_WRITE; }
		}
	}

	if (!mode) {
		orcus_release_var(&file_var);
		orcus_release_var(&mode_var);
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 0);
		return;
	}

	FILE* F=fopen(file,mode);
	if (F) {
		out.type = ORCUS_TYPE_FILE;
		

		orcus_filewrapper* fw = (orcus_filewrapper*)malloc(sizeof(orcus_filewrapper));
		if (!fw) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			fclose(F);
			return;
		}
		fw->references = 0;
		fw->modeflags = flags;
		fw->file = F;

		out.data.p = fw;
	}
	else {
		out.type = ORCUS_TYPE_NIL;
		out.data.i = 0;
	}
	free(file);

	orcus_release_var(&file_var);
	orcus_release_var(&mode_var);

	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}



void orcus_lib_io_read(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1,3)) {
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		if (args_in > 1)orcus_discard_top_of_stack(M);
		if (args_in > 2)orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable offset_var;
	if (args_in > 2) {
		orcus_popstack(M, &offset_var);
		if (offset_var.type != ORCUS_TYPE_INT) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &offset_var, ORCUS_TYPE_INT, 3);
			orcus_release_var(&offset_var);
			return;
		}
	}
	else {
		offset_var.type = ORCUS_TYPE_NIL;
		offset_var.data.i = 0;
	}

	orcus_variable length_var;
	if (args_in > 1) {
		orcus_popstack(M, &length_var);
		if (length_var.type != ORCUS_TYPE_INT && length_var.type) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &length_var, ORCUS_TYPE_INT, 2);
			orcus_release_var(&length_var);
			return;
		}
		if (length_var.data.i < 0)length_var.data.i = 0;
	}
	else {
		length_var.type = ORCUS_TYPE_NIL;
		length_var.data.i = 0;
	}


	orcus_variable file_var;
	orcus_popstack(M, &file_var);
	if (file_var.type != ORCUS_TYPE_FILE) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &file_var, ORCUS_TYPE_FILE, 1);
		orcus_release_var(&file_var);
		return;
	}

	orcus_variable out;

	orcus_filewrapper* fw= (orcus_filewrapper*)file_var.data.p;
	FILE* F = fw->file;

	if (F && (fw->modeflags&ORCUS_FILEFLAG_READ) ) {

		orcus_int len=length_var.data.i;
		if (!length_var.type) {
			fseek(F, 0, SEEK_END);
			len=ftell(F);
		}
		if (len == -1) {
			out.type = ORCUS_TYPE_NIL;
			out.data.i = 0;
		}
		else {
			char* s = (char*)malloc(sizeof(char) * len );
			if (!s) {
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			rewind(F);
			fseek(F, offset_var.data.i, SEEK_SET);
			size_t num_r=fread(s, sizeof(char), len, F);
			orcus_string* str = (orcus_string*)malloc(sizeof(orcus_string));
			if (!str) {
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			str->ptr = s;
			str->size = num_r;
			str->references = 0;
			str->constant = false;
			out.type = ORCUS_TYPE_STRING;
			out.data.p = str;
		}
		
	}
	else {
		out.type = ORCUS_TYPE_NIL;
		out.data.i = 0;
	}

	orcus_release_var(&file_var);

	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

void orcus_lib_io_close(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		return;
	}

	orcus_variable file_var;
	orcus_popstack(M, &file_var);
	if (file_var.type != ORCUS_TYPE_FILE) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &file_var, ORCUS_TYPE_FILE, 1);
		orcus_release_var(&file_var);
		return;
	}


	orcus_filewrapper* fw = (orcus_filewrapper*)file_var.data.p;
	if (fw->modeflags) {
		fw->modeflags = 0;
		if(fw->file)fclose(fw->file);
	}

	orcus_release_var(&file_var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
}

void orcus_lib_io_write(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 2,3)) {
		return;
	}

	orcus_variable offset_var;
	if (args_in > 2) {
		orcus_popstack(M, &offset_var);
		if (offset_var.type != ORCUS_TYPE_INT) {
			orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &offset_var, ORCUS_TYPE_INT, 3);
			orcus_release_var(&offset_var);
			return;
		}
	}
	else {
		offset_var.type = ORCUS_TYPE_NIL;
		offset_var.data.i = 0;
	}

	orcus_variable data_var;
	orcus_popstack(M, &data_var);
	if (data_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &data_var, ORCUS_TYPE_STRING, 2);
		orcus_release_var(&data_var);
		return;
	}

	orcus_variable file_var;
	orcus_popstack(M, &file_var);
	if (file_var.type != ORCUS_TYPE_FILE) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &file_var, ORCUS_TYPE_FILE, 1);
		orcus_release_var(&file_var);
		return;
	}
	orcus_string* str = (orcus_string*)data_var.data.p;
	orcus_filewrapper* fw = (orcus_filewrapper*)file_var.data.p;

	orcus_variable out;
	out.type = ORCUS_TYPE_INT;
	out.data.i = 0;
	if (fw->modeflags&ORCUS_FILEFLAG_WRITE) {
		if (offset_var.type) {
			fseek(fw->file, offset_var.data.i, SEEK_SET);
		}
		else {
			fseek(fw->file, 0, SEEK_END);
		}
		out.data.i = (orcus_int)fwrite(str->ptr, 1, str->size, fw->file);
		fflush(fw->file);
	}
	orcus_release_var(&data_var);
	orcus_release_var(&file_var);

	if (args_out)orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}



void orcus_lib_io_getfiles(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //an array of strings
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable dir_var;
	orcus_popstack(M,&dir_var);
	if (dir_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &dir_var, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&dir_var);
		return;
	}

	orcus_string* mstr = (orcus_string*)dir_var.data.p;
#ifdef _WIN32
	if (mstr->size == 0) {
		orcus_mstring_addchars(mstr, (char*)"*", 1);
	}
	else {
		orcus_mstring_addchars(mstr, (char*)"/*", 2);
	}
#else
	if (mstr->size == 0) {
		orcus_mstring_addchars(mstr, (char*)".", 1);
	}
#endif
	char* dir = orcus_mstring_to_cstring(mstr);
	
	orcus_array* arr=orcus_newarray();

	orcus_int num_fs = 0;


#ifdef _WIN32 //windows
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind;

	hFind=FindFirstFileA(dir, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
		while (true) {
			if (!(FindFileData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE) )) {
				char* fn = FindFileData.cFileName;
				orcus_string* s = orcus_cstring_to_mstring(fn, strlen(fn));
				if (!s) {
					orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
					return;
				}
				orcus_variable v;
				
				v.type = ORCUS_TYPE_STRING;
				v.data.p = s;
				orcus_setarray(arr, &v, num_fs);
				num_fs++;
			}
			if (!FindNextFileA(hFind, &FindFileData))break;
		}
	}
	FindClose(hFind);

#else //linux and whatnot
	DIR* d = opendir(dir);
	if (d) {
		dirent* ent;
		while (true) {
			ent = readdir(d);
			if (!ent)break;
			if (ent->d_type == DT_REG) {
				orcus_string* s = orcus_cstring_to_mstring(ent->d_name, strlen(ent->d_name));
				if (!s) {
					orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
					return;
				}
				orcus_variable v;
				
				v.type = ORCUS_TYPE_STRING;
				v.data.p = s;
				orcus_setarray(arr, &v, num_fs);
				num_fs++;
			}
		}
		closedir(d);
	}
#endif
	orcus_release_var(&dir_var);
	dir_var.data.p = arr;
	dir_var.type = ORCUS_TYPE_ARRAY;
	orcus_pushstack(M, &dir_var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}


void orcus_lib_io_getdirs(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //an array of strings
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable dir_var;
	orcus_popstack(M, &dir_var);
	if (dir_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &dir_var, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&dir_var);
		return;
	}

	orcus_string* mstr = (orcus_string*)dir_var.data.p;
#ifdef _WIN32
	if (mstr->size == 0) {
		orcus_mstring_addchars(mstr, (char*)"*", 1);
	}
	else {
		orcus_mstring_addchars(mstr, (char*)"/*", 2);
	}
#else
	if (mstr->size == 0) {
		orcus_mstring_addchars(mstr, (char*)".", 1);
	}
#endif
	char* dir = orcus_mstring_to_cstring(mstr);

	
	orcus_array* arr = orcus_newarray();

	orcus_int num_fs = 0;


#ifdef _WIN32 //windows
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind;

	hFind = FindFirstFileA(dir, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
		while (true) {
			if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strcmp(FindFileData.cFileName,".") && strcmp(FindFileData.cFileName, "..")) {
				char* fn = FindFileData.cFileName;
				orcus_string* s = orcus_cstring_to_mstring(fn, strlen(fn));
				if (!s) {
					orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
					return;
				}
				orcus_variable v;
				
				v.type = ORCUS_TYPE_STRING;
				v.data.p = s;
				orcus_setarray(arr, &v, num_fs);
				num_fs++;
			}
			if (!FindNextFileA(hFind, &FindFileData))break;
		}
	}
	FindClose(hFind);

#else //linux and whatnot
	DIR* d = opendir(dir);
	if (d) {
		dirent* ent;
		while (true) {
			ent = readdir(d);
			if (!ent)break;
			if (ent->d_type == DT_DIR && strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
				orcus_string* s = orcus_cstring_to_mstring(ent->d_name, strlen(ent->d_name));
				if (!s) {
					orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
					return;
				}
				orcus_variable v;
				
				v.type = ORCUS_TYPE_STRING;
				v.data.p = s;
				orcus_setarray(arr, &v, num_fs);
				num_fs++;
			}
		}
		closedir(d);
	}
#endif


	orcus_release_var(&dir_var);
	dir_var.data.p = arr;
	dir_var.type = ORCUS_TYPE_ARRAY;
	orcus_pushstack(M, &dir_var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}




void orcus_lib_io_lines(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //returns an array of the lines, sans newline characters.
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable fil_var;
	orcus_popstack(M, &fil_var);
	if (fil_var.type != ORCUS_TYPE_FILE) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &fil_var, ORCUS_TYPE_FILE, 1);
		orcus_release_var(&fil_var);
		return;
	}

	
	orcus_array* arr = orcus_newarray();
	orcus_int count = 0;

	orcus_filewrapper* fw = (orcus_filewrapper*)fil_var.data.p;
	if (fw->modeflags & ORCUS_FILEFLAG_READ) {
		FILE* f = fw->file;
		rewind(f);
		fseek(f, 0, SEEK_END);
		orcus_int total_len = ftell(f);
		rewind(f);
		char* buffer=(char*)malloc(total_len);
		if (!buffer && total_len) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		if (total_len) { //support for empty file reading
			fread(buffer, 1, total_len, f);
			rewind(f);
		}
		orcus_int run_end = 0;
		orcus_int run_start = 0;
		while (true) {
			if (run_end == total_len) { //end of file
				if (run_end - run_start) {
					orcus_string* s = orcus_cstring_to_mstring(buffer+ run_start, run_end -run_start);
					if (!s) {
						orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
						free(buffer);
						orcus_destroyarray(arr);
						return;
					}
					orcus_variable v;
					v.type = ORCUS_TYPE_STRING;
					v.data.p = s;
					orcus_setarray(arr, &v, count);
					count++;
				}
				break;
			}
			char cur_char=buffer[run_end];
			if (cur_char == '\n' || cur_char == '\r') {
				if (run_end - run_start) {
					orcus_string* s = orcus_cstring_to_mstring(buffer + run_start, run_end - run_start);
					if (!s) {
						orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
						free(buffer);
						orcus_destroyarray(arr);
						return;
					}
					orcus_variable v;
					v.type = ORCUS_TYPE_STRING;
					v.data.p = s;
					orcus_setarray(arr, &v, count);
					count++;
				}
				run_end++;
				run_start = run_end;
			}
			else {
				run_end++;
			}
		}
		free(buffer);

		
	}
	

	orcus_release_var(&fil_var);
	fil_var.type = ORCUS_TYPE_ARRAY;
	fil_var.data.p = arr;
	orcus_pushstack(M, &fil_var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_io_post(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //send characters to stdout directly
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		return;
	}

	orcus_variable str_var;
	orcus_popstack(M,&str_var);
	if (str_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_nonpointer(M, ORCUS_ERROR_WRONG_TYPE, str_var.type, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&str_var);
		return;
	}

	orcus_string* s = (orcus_string*)str_var.data.p;
	fwrite(s->ptr, 1, s->size, stdout);

	orcus_release_var(&str_var);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
}

void orcus_lib_io_prompt(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //read a line from stdin
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0)) {
		return;
	}

	orcus_int sizec = 200;
	orcus_int len = 0;
	char* c = (char*)malloc(sizec);
	if (!c) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		return;
	}


	int ch = 0;
	while ((ch = fgetc(stdin)) != EOF && ch!='\n' && ch != '\r') {
		c[len] = ch;
		len++;
		if (len >= sizec) {
			sizec += 200;
			void* n=realloc(c, sizec);
			if (!n) {
				orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
				return;
			}
			c = (char*)n;
		}
	}

	if (args_out) {
		orcus_string* s= orcus_cstring_promote_to_mstring(c, len);
		if (!s) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		orcus_variable v;
		
		v.type = ORCUS_TYPE_STRING;
		v.data.p = s;
		orcus_pushstack(M, &v);
	}
	else {
		free(c);
	}
	


	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}



void orcus_lib_io_remove(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		return;
	}

	orcus_variable dir_var;
	orcus_popstack(M, &dir_var);
	if (dir_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &dir_var, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&dir_var);
		return;
	}
	orcus_string* fst = (orcus_string*)dir_var.data.p;

	char* cfilestr=orcus_mstring_to_cstring(fst);


	int r=remove(cfilestr);
	if (r == -1 && errno==ENOENT) {
		r = 0;
	}

	if (args_out) {
		orcus_variable out;
		out.type = ORCUS_TYPE_BOOL;
		out.data.i = r == 0 ? 1 : 0;
		orcus_pushstack(M, &out);
	}

	orcus_release_var(&dir_var);
	free(cfilestr);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out,1);
}


void orcus_lib_io_removedir(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		return;
	}

	orcus_variable dir_var;
	orcus_popstack(M, &dir_var);
	if (dir_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &dir_var, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&dir_var);
		return;
	}
	orcus_string* fst = (orcus_string*)dir_var.data.p;

	char* cfilestr = orcus_mstring_to_cstring(fst);

#if defined(_WIN32) || defined(_WIN64)
	int r = _rmdir(cfilestr);
	if (r == -1 && errno == ENOENT) {
		r = 0;
	}
#else
	int r = rmdir(cfilestr);
	if (r == -1 && errno == ENOENT) {
		r = 0;
	}
#endif

	if (args_out) {
		orcus_variable out;
		out.type = ORCUS_TYPE_BOOL;
		out.data.i = r == 0 ? 1 : 0;
		orcus_pushstack(M, &out);
	}

	orcus_release_var(&dir_var);
	free(cfilestr);
	

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}




void orcus_lib_io_createdir(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		return;
	}

	orcus_variable dir_var;
	orcus_popstack(M, &dir_var);
	if (dir_var.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &dir_var, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&dir_var);
		return;
	}
	orcus_string* fst = (orcus_string*)dir_var.data.p;

	char* cfilestr = orcus_mstring_to_cstring(fst);
#if defined(_WIN32) || defined(_WIN64)
	int r = _mkdir(cfilestr);
	if (r == -1 && errno == EEXIST) {
		r = 0;
	}
#else
	int r = mkdir(cfilestr,0755); //rwxr-xr-x
	if (r == -1 && errno == EEXIST) {
		r = 0;
	}
#endif
	

	if (args_out) {
		orcus_variable out;
		out.type = ORCUS_TYPE_BOOL;
		out.data.i = r == 0 ? 1 : 0;
		orcus_pushstack(M, &out);
	}

	orcus_release_var(&dir_var);
	free(cfilestr);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



void orcus_lib_io_input(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) { //read a single char stdin. no newline required! (platform dependant, though. :/)
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0)) {
		return;
	}

	//got rid of it. should just be raw input. on the coder to not read it in a while loop (you can also manually check for ctrl+c)
	//orcus_variable* ck = orcus_popstack(M);

	unsigned char c[256];
	int desired_length = 1;
	int o;
#ifdef _WIN32
	o = _getch();
	c[0] = o;
	if (o == 0xE0 || 01 == 0x00) {
		desired_length = 2;
		o = _getch(); //this is a mess
		c[1] = o;
	}
#else
	termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO | ISIG);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	desired_length = read(STDIN_FILENO, c, 0xff);
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

	//ctrl+c interuption.
	//if (c == '\3' ){//&& orcus_checkbool(ck) ) {
	//	M->programcounter = M->bytecode.numberofinstructions;
	//}

	//orcus_free_var(ck);

	if (args_out) {
		orcus_string* s = orcus_cstring_to_mstring((const char*)c, desired_length);
		if (!s) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		orcus_variable v;
		
		v.type = ORCUS_TYPE_STRING;
		v.data.p = s;
		orcus_pushstack(M, &v);
	}

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}

const size_t char_array_blocksize = 16;
enum orcus_serialize_types:unsigned char{
	SERIALIZED_NIL = '\0',
	SERIALIZED_INT64,
	SERIALIZED_INT32,
	SERIALIZED_INT16,
	SERIALIZED_INT8,
	SERIALIZED_INT8UNSIGNED,
	SERIALIZED_FLOAT32,
	SERIALIZED_FLOAT64,
	SERIALIZED_BOOLTRUE,
	SERIALIZED_BOOLFALSE,
	SERIALIZED_STRING_SIZEEMPTY, //empty string
	SERIALIZED_STRING_SIZESINGLE, //1 character string
	SERIALIZED_STRING_SIZE8,
	SERIALIZED_STRING_SIZE16,
	SERIALIZED_STRING_SIZE32,
	SERIALIZED_STRING_SIZE64,
	SERIALIZED_ARRAY_SIZE8, //unused
	SERIALIZED_ARRAY_SIZE16, //unused
	SERIALIZED_ARRAY_SIZE32, //unused
	SERIALIZED_ARRAY_SIZE64, //unused
	SERIALIZED_ARRAY_SIZEBITWIDTH,
	SERIALIZED_TABLE_SIZE8, //unused
	SERIALIZED_TABLE_SIZE16, //unused
	SERIALIZED_TABLE_SIZE132, //unused
	SERIALIZED_TABLE_SIZE164, //unused
	SERIALIZED_TABLE_SIZEBITWIDTH,
};


inline bool check_chars_preallocation(unsigned char** chars, orcus_int* num_chars, orcus_int* chars_allocated, orcus_int chars_requested) {
#ifdef ORCUS_64BIT
	if (*num_chars > *chars_allocated) {
		printf("serialization called a preallocation with %zi chars set, but only %zi are allocated\n",*num_chars,*chars_allocated);
	}
#endif
	orcus_int defecit = *num_chars + chars_requested - *chars_allocated;
	if (defecit>0) {
		orcus_int blocks_needed = (defecit + char_array_blocksize - 1) / char_array_blocksize;
		void* nptr=realloc(*chars, (size_t)(*chars_allocated + (char_array_blocksize * blocks_needed) ) );
		if (!nptr)return false;
		*chars = (unsigned char*)nptr;
		(*chars_allocated) += char_array_blocksize * blocks_needed;
	}
	return true;
}

inline bool check_pointer_serial(void* ptr, const orcus_uint* num_pointers_covered, void*** pointers_covered) {
	for (orcus_uint i = 0; i < (*num_pointers_covered); i++) {
		if ((*pointers_covered)[i] == ptr) {
			return true;
		}
	}
	return false;
}

inline bool add_serial_pointer(void* ptr, orcus_uint* num_pointers_covered, void*** pointers_covered) {
	if ((*num_pointers_covered) % char_array_blocksize == 0) { //only reallocate every 16 to save speed, probly.
		void* nptr = realloc(*pointers_covered, sizeof(void*) * ((*num_pointers_covered) + char_array_blocksize));
		if (!nptr)return false;
		*pointers_covered = (void**)nptr;
	}

	(*pointers_covered)[*num_pointers_covered] = ptr;

	(*num_pointers_covered)++;
	return true;
}

inline bool can_serialize_var(const orcus_variable* var) {
	switch (var->type) {
	case ORCUS_TYPE_INT:
	case ORCUS_TYPE_FLOAT:
	case ORCUS_TYPE_BOOL:
	case ORCUS_TYPE_STRING:
	case ORCUS_TYPE_ARRAY:
	case ORCUS_TYPE_TABLE:
		return true;
	default:
		return false;
	}
}

bool orcus_serialize_variable(const orcus_variable* var, unsigned char** chars,orcus_int* num_chars,orcus_int* chars_allocated,void*** pointerscovered,orcus_uint* num_pointers_covered) {
	
	switch (var->type) {
		case ORCUS_TYPE_INT:
			if (var->data.i<= SCHAR_MAX && var->data.i >= SCHAR_MIN) {
				if (!check_chars_preallocation(chars, num_chars, chars_allocated, 2))return false;
				(*chars)[*num_chars] = SERIALIZED_INT8;
				(*num_chars)++;
				(*chars)[*num_chars] = (signed char)var->data.i;
				(*num_chars)++;
				return true;
			}
			else if (var->data.i<=UCHAR_MAX && var->data.i>0) {
				if (!check_chars_preallocation(chars, num_chars, chars_allocated, 2))return false;
				(*chars)[*num_chars] = SERIALIZED_INT8UNSIGNED;
				(*num_chars)++;
				(*chars)[*num_chars] = (unsigned char)var->data.i;
				(*num_chars)++;
				return true;
			}
			else if (var->data.i<= SHRT_MAX && var->data.i >= SHRT_MIN) {
				if (!check_chars_preallocation(chars, num_chars, chars_allocated, 3))return false;
				(*chars)[*num_chars] = SERIALIZED_INT16;
				(*num_chars)++;
				*(short*)(*chars+*num_chars) = (short)var->data.i;
				(*num_chars)+=2;
				return true;
			}
			else if (var->data.i<= INT_MAX && var->data.i >= INT_MIN) {
				if (!check_chars_preallocation(chars, num_chars, chars_allocated, 5))return false;
				(*chars)[*num_chars] = SERIALIZED_INT32;
				(*num_chars)++;
				*(int*)(*chars + *num_chars) = (int)var->data.i;
				(*num_chars) += 4;
				return true;
			}
			else {
				if (!check_chars_preallocation(chars, num_chars, chars_allocated, 9))return false;
				(*chars)[*num_chars] = SERIALIZED_INT64;
				(*num_chars)++;
				*(int64_t*)(*chars + *num_chars) = (int64_t)var->data.i;
				(*num_chars) += 8;
				return true;
			}
			return false;
		case ORCUS_TYPE_FLOAT:
#ifdef ORCUS_64BIT
			if (!check_chars_preallocation(chars, num_chars, chars_allocated, 9))return false;
			(*chars)[*num_chars] = SERIALIZED_FLOAT64;
			(*num_chars)++;
			*(double*)(*chars + *num_chars) = (double)var->data.f;
			(*num_chars) += 8;
			return true;
#else
			if (!check_chars_preallocation(chars, num_chars, chars_allocated, 5))return false;
			(*chars)[*num_chars] = SERIALIZED_FLOAT32;
			(*num_chars)++;
			*(float*)(*chars + *num_chars) = (float)var->data.f;
			(*num_chars) += 4;
			return true;
#endif
			return false;
		case ORCUS_TYPE_BOOL:
			if (!check_chars_preallocation(chars, num_chars, chars_allocated, 1))return false;
			(*chars)[*num_chars] = var->data.i ? SERIALIZED_BOOLTRUE : SERIALIZED_BOOLFALSE;
			(*num_chars)++;
			return true;
		case ORCUS_TYPE_STRING:
			{
			orcus_string* str=(orcus_string*)var->data.p;
			if (!str->size) {
				if (!check_chars_preallocation(chars, num_chars, chars_allocated, 1))return false;
				(*chars)[*num_chars] = SERIALIZED_STRING_SIZEEMPTY;
				(*num_chars)++;
				return true;
			}
			else if (str->size == 1) {
				if (!check_chars_preallocation(chars, num_chars, chars_allocated, 2))return false;
				(*chars)[*num_chars] = SERIALIZED_STRING_SIZESINGLE;
				(*num_chars)++;
				(*chars)[*num_chars] = str->ptr[0];
				(*num_chars)++;
				return true;
			}
			else if (str->size <= UCHAR_MAX) {
				if (!check_chars_preallocation(chars, num_chars, chars_allocated, str->size+2))return false;
				(*chars)[*num_chars] = SERIALIZED_STRING_SIZE8;
				(*num_chars)++;
				*(unsigned char*)(*chars + *num_chars) = (unsigned char)str->size;
				(*num_chars)++;
				memcpy(*chars + *num_chars, str->ptr, str->size);
				(*num_chars)+=str->size;
				return true;
			}
			else if (str->size <= USHRT_MAX) {
				if (!check_chars_preallocation(chars, num_chars, chars_allocated, str->size + 3))return false;
				(*chars)[*num_chars] = SERIALIZED_STRING_SIZE16;
				(*num_chars)++;
				*(unsigned short*)(*chars + *num_chars) = (unsigned short)str->size;
				(*num_chars)+=2;
				memcpy(*chars + *num_chars, str->ptr, str->size);
				(*num_chars) += str->size;
				return true;
			}
			else if (str->size <= INT_MAX) {
				if (!check_chars_preallocation(chars, num_chars, chars_allocated, str->size + 5))return false;
				(*chars)[*num_chars] = SERIALIZED_STRING_SIZE32;
				(*num_chars)++;
				*(int*)(*chars + *num_chars) = (int)str->size;
				(*num_chars) += 4;
				memcpy(*chars + *num_chars, str->ptr, str->size);
				(*num_chars) += str->size;
				return true;
			}
			else {
				if (!check_chars_preallocation(chars, num_chars, chars_allocated, str->size + 9))return false;
				(*chars)[*num_chars] = SERIALIZED_STRING_SIZE32;
				(*num_chars)++;
				*(int64_t*)(*chars + *num_chars) = (int64_t)str->size;
				(*num_chars) += 8;
				memcpy(*chars + *num_chars, str->ptr, str->size);
				(*num_chars) += str->size;
				return true;
			}
			}
			return false;
		case ORCUS_TYPE_ARRAY:
			//if (check_pointer_serial(var->data.p, num_pointers_covered, pointerscovered))return true;
			{
			if (!check_chars_preallocation(chars, num_chars, chars_allocated, 1 + sizeof(orcus_int)))return false;
			(*chars)[*num_chars] = SERIALIZED_ARRAY_SIZEBITWIDTH;
			(*num_chars)++;
			*(orcus_int*)(*chars + *num_chars) = 0;
			orcus_int elements_offset = *num_chars;
			(*num_chars) += sizeof(orcus_int);
			orcus_int cur_elems = 0;
			orcus_array* arr = (orcus_array*)var->data.p;
			orcus_uint orig_n_ptr_conv = *num_pointers_covered;
			if (!add_serial_pointer(arr, num_pointers_covered, pointerscovered))return false;
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
							if (can_serialize_var(&var)) {
								orcus_variable indexvar;
								indexvar.type = ORCUS_TYPE_INT;
								indexvar.data.i = index;

								if (var.type == ORCUS_TYPE_ARRAY || var.type == ORCUS_TYPE_TABLE) {
									if (check_pointer_serial(var.data.p, num_pointers_covered, pointerscovered))continue;
								}

								if (!orcus_serialize_variable(&indexvar, chars, num_chars, chars_allocated, pointerscovered, num_pointers_covered))return false;
								if (!orcus_serialize_variable(&var, chars, num_chars, chars_allocated, pointerscovered, num_pointers_covered))return false;

								cur_elems++;
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

			*(orcus_int*)(*chars + elements_offset) = cur_elems;
			*num_pointers_covered = orig_n_ptr_conv;
			return true;
			}
			return false;
		case ORCUS_TYPE_TABLE:
			//if (check_pointer_serial(var->data.p, num_pointers_covered, pointerscovered))return true;
			{
				if (!check_chars_preallocation(chars, num_chars, chars_allocated, 1 + sizeof(orcus_int)))return false;
				(*chars)[*num_chars] = SERIALIZED_TABLE_SIZEBITWIDTH;
				(*num_chars)++;
				*(orcus_int*)(*chars + *num_chars) = 0;
				orcus_int elements_offset =  *num_chars;
				(*num_chars) += sizeof(orcus_int);
				orcus_int cur_elems = 0;
				orcus_table* tab = (orcus_table*)var->data.p;
				orcus_uint orig_n_ptr_conv = *num_pointers_covered;
				if (!add_serial_pointer(tab, num_pointers_covered, pointerscovered))return false;

				for (uint8_t t = 0; t < ORCUS_NUMBER_OF_TYPES; t++) {
					orcus_subtable st = tab->data[t];
					for (orcus_int i = 0; i < st.size; i++) {
						if (can_serialize_var(st.values+i) && can_serialize_var(st.keys + i)) {
							if(st.keys[i].type==ORCUS_TYPE_ARRAY || st.keys[i].type == ORCUS_TYPE_TABLE){
								if(check_pointer_serial(st.keys[i].data.p,num_pointers_covered,pointerscovered))continue;
							}
							if (st.values[i].type == ORCUS_TYPE_ARRAY || st.values[i].type == ORCUS_TYPE_TABLE) {
								if (check_pointer_serial(st.values[i].data.p, num_pointers_covered, pointerscovered))continue;
							}
							if (!orcus_serialize_variable(st.keys+i, chars, num_chars, chars_allocated, pointerscovered, num_pointers_covered))return false;
							if (!orcus_serialize_variable(st.values+i, chars, num_chars, chars_allocated, pointerscovered, num_pointers_covered))return false;
							cur_elems++;
						}

					}
				}

				*(orcus_int*)(*chars + elements_offset) = cur_elems;
				*num_pointers_covered = orig_n_ptr_conv;
				return true;
			}
			return false;
		case ORCUS_TYPE_NIL:
		default:
			if (!check_chars_preallocation(chars, num_chars, chars_allocated, 1))return false;
			(*chars)[*num_chars] = SERIALIZED_NIL;
			(*num_chars)++;
			return true;
	}
	return false;
}


void orcus_lib_io_serialize(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable in;
	orcus_popstack(M, &in);
	orcus_variable out;

	unsigned char* chars = nullptr;
	orcus_int num_chars=0;
	orcus_int chars_allocated=0;
	void** pointerscovered = nullptr;
	orcus_uint num_pointers_covered = 0;


	if (orcus_serialize_variable(&in, &chars, &num_chars, &chars_allocated, &pointerscovered, &num_pointers_covered)) {
		orcus_string* str=(orcus_string*)malloc(sizeof(orcus_string));
		if (!str) {
			free(chars);
			free(pointerscovered);
			orcus_raise_error_nonpointer(M, ORCUS_ERROR_ALLOCATION);
			return;
		}
		str->ptr = (char*)chars;
		str->constant = false;
		str->references = 1;
		str->size = num_chars;
		out.data.p = str;
		out.type = ORCUS_TYPE_STRING;
	}
	else {
		free(chars);
		free(pointerscovered);
		out.type = ORCUS_TYPE_NIL;
		out.data.i = 0;
	}

	orcus_pushstack(M,&out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


inline bool deser_chars_avalible(orcus_int size,orcus_int position,orcus_int requested) {
#ifdef ORCUS_DEBUG 
	if (!(position + requested <= size)) {
		printf("deserialize failed to find needed chars! we need %zi more, but only have %zi remaining\n",requested,size-position);
	}
#endif
	return position + requested <= size;
}

bool orcus_deserialize_variable(orcus_variable* out,const unsigned char* chars,const orcus_int size,orcus_int* position) {
	if (!deser_chars_avalible(size, *position, 1))return false;
	unsigned char cur_char = chars[*position];
	switch (cur_char) {
		case SERIALIZED_NIL:
			out->data.i = 0;
			out->type = ORCUS_TYPE_NIL;
			(*position)++;
			return true;
		case SERIALIZED_BOOLFALSE:
			out->data.i = 0;
			out->type = ORCUS_TYPE_BOOL;
			(*position)++;
			return true;
		case SERIALIZED_BOOLTRUE:
			out->data.i = 1;
			out->type = ORCUS_TYPE_BOOL;
			(*position)++;
			return true;
		case SERIALIZED_INT8:
			(*position)++;
			if(!deser_chars_avalible(size, *position, 1))return false;
			out->data.i = *(signed char*)(chars+*position);
			out->type = ORCUS_TYPE_INT;
			(*position)++;
			return true;
		case SERIALIZED_INT8UNSIGNED:
			(*position)++;
			if (!deser_chars_avalible(size, *position, 1))return false;
			out->data.u = *(unsigned char*)(chars + *position);
			out->type = ORCUS_TYPE_INT;
			(*position)++;
			return true;
		case SERIALIZED_INT16:
			(*position)++;
			if (!deser_chars_avalible(size, *position, 2))return false;
			out->data.i = *(short*)(chars + *position);
			out->type = ORCUS_TYPE_INT;
			(*position)+=2;
			return true;
		case SERIALIZED_INT32:
			(*position)++;
			if (!deser_chars_avalible(size, *position, 4))return false;
			out->data.i = *(int*)(chars + *position);
			out->type = ORCUS_TYPE_INT;
			(*position) += 4;
			return true;
		case SERIALIZED_INT64:
			(*position)++;
			if (!deser_chars_avalible(size, *position, 8))return false;
			out->data.i = *(int64_t*)(chars + *position);
			out->type = ORCUS_TYPE_INT;
			(*position) += 8;
			return true;
		case SERIALIZED_FLOAT32:
			(*position)++;
			if (!deser_chars_avalible(size, *position, 4))return false;
			out->data.f = *(float*)(chars + *position);
			out->type = ORCUS_TYPE_FLOAT;
			(*position) += 4;
			return true;
		case SERIALIZED_FLOAT64:
			(*position)++;
			if (!deser_chars_avalible(size, *position, 8))return false;
			out->data.f = *(double*)(chars + *position);
			out->type = ORCUS_TYPE_FLOAT;
			(*position) += 8;
			return true;
		case SERIALIZED_STRING_SIZEEMPTY:
			{
			(*position)++;
			orcus_string* str = (orcus_string*)malloc(sizeof(orcus_string));
			if (!str)return false;
			memset(str, 0, sizeof(orcus_string));
			str->references = 0;
			out->type = ORCUS_TYPE_STRING;
			out->data.p = str;
			}
			return true;
		case SERIALIZED_STRING_SIZESINGLE:
		{
			(*position)++;
			if (!deser_chars_avalible(size, *position, 1))return false;
			orcus_string* str = (orcus_string*)malloc(sizeof(orcus_string));
			if (!str)return false;
			memset(str, 0, sizeof(orcus_string));
			str->references = 0;
			str->size = 1;
			str->ptr = (char*)malloc(1);
			if (!str->ptr) { free(str); return false; }
			str->ptr[0] = chars[*position];
			(*position)++;
			out->type = ORCUS_TYPE_STRING;
			out->data.p = str;
		}
			return true;
		case SERIALIZED_STRING_SIZE8:
		{
			(*position)++;

			if (!deser_chars_avalible(size, *position, 1))return false;
			orcus_int needed_size=*(unsigned char*)(chars+*position);
			(*position)++;

			if (!deser_chars_avalible(size, *position, needed_size))return false;
			orcus_string* str = (orcus_string*)malloc(sizeof(orcus_string));
			if (!str)return false;
			memset(str, 0, sizeof(orcus_string));
			str->references = 0;
			str->size = needed_size;
			str->ptr = (char*)malloc(needed_size);
			if (!str->ptr) { free(str); return false; }
			memcpy(str->ptr, chars + *position, needed_size);
			(*position) += needed_size;
			out->type = ORCUS_TYPE_STRING;
			out->data.p = str;
		}
			return true;
		case SERIALIZED_STRING_SIZE16:
		{
			(*position)++;

			if (!deser_chars_avalible(size, *position, 2))return false;
			orcus_int needed_size = *(unsigned short*)(chars + *position);
			(*position)+=2;

			if (!deser_chars_avalible(size, *position, needed_size))return false;
			orcus_string* str = (orcus_string*)malloc(sizeof(orcus_string));
			if (!str)return false;
			memset(str, 0, sizeof(orcus_string));
			str->references = 0;
			str->size = needed_size;
			str->ptr = (char*)malloc(needed_size);
			if (!str->ptr) { free(str); return false; }
			memcpy(str->ptr, chars + *position, needed_size);
			(*position) += needed_size;
			out->type = ORCUS_TYPE_STRING;
			out->data.p = str;
		}
			return true;
		case SERIALIZED_STRING_SIZE32:
		{
			(*position)++;

			if (!deser_chars_avalible(size, *position, 4))return false;
			orcus_int needed_size = *(int*)(chars + *position);
			(*position) += 4;

			if (!deser_chars_avalible(size, *position, needed_size))return false;
			orcus_string* str = (orcus_string*)malloc(sizeof(orcus_string));
			if (!str)return false;
			memset(str, 0, sizeof(orcus_string));
			str->references = 0;
			str->size = needed_size;
			str->ptr = (char*)malloc(needed_size);
			if (!str->ptr) { free(str); return false; }
			memcpy(str->ptr, chars + *position, needed_size);
			(*position) += needed_size;
			out->type = ORCUS_TYPE_STRING;
			out->data.p = str;
		}
			return true;
		case SERIALIZED_STRING_SIZE64:
		{
			(*position)++;

			if (!deser_chars_avalible(size, *position, 8))return false;
			orcus_int needed_size = *(int*)(chars + *position);
			(*position) += 8;

			if (!deser_chars_avalible(size, *position, needed_size))return false;
			orcus_string* str = (orcus_string*)malloc(sizeof(orcus_string));
			if (!str)return false;
			memset(str, 0, sizeof(orcus_string));
			str->references = 0;
			str->size = needed_size;
			str->ptr = (char*)malloc(needed_size);
			if (!str->ptr) { free(str); return false; }
			memcpy(str->ptr, chars + *position, needed_size);
			(*position) += needed_size;
			out->type = ORCUS_TYPE_STRING;
			out->data.p = str;
		}
			return true;
		case SERIALIZED_ARRAY_SIZEBITWIDTH:
			{
				(*position)++;
				orcus_array* arr=orcus_newarray();
				if (!deser_chars_avalible(size, *position, sizeof(orcus_int) ))return false;
				if (!arr)return false;
				orcus_int asize = *(orcus_int*)(chars + *position);
				(*position) += sizeof(orcus_int);
				while (asize) {
					orcus_variable key;
					orcus_variable value;
					if (!orcus_deserialize_variable(&key, chars, size, position))return false;
					if (key.type != ORCUS_TYPE_INT) {
						orcus_release_var(&key);
						return false;
					}
					if (!orcus_deserialize_variable(&value, chars, size, position))return false;
					if (!orcus_setarray(arr, &value, key.data.i)) {
						orcus_release_var(&value);
						return false;
					}
					asize--;
				}
				out->type = ORCUS_TYPE_ARRAY;
				out->data.p = arr;
			}
			return true;
		case SERIALIZED_TABLE_SIZEBITWIDTH:
		{
			(*position)++;
			orcus_table* arr = orcus_newtable();
			if (!deser_chars_avalible(size, *position, sizeof(orcus_int)))return false;
			if (!arr)return false;
			orcus_int asize = *(orcus_int*)(chars + *position);
			(*position) += sizeof(orcus_int);
			while (asize) {
				orcus_variable key;
				orcus_variable value;
				if (!orcus_deserialize_variable(&key, chars, size, position))return false;
				if (!orcus_deserialize_variable(&value, chars, size, position)) {
					orcus_release_var(&key);
					return false;
				};
				if (orcus_setkey(arr, &key,&value)==-1) {
					orcus_release_var(&key);
					orcus_release_var(&value);
					return false;
				}
				asize--;
			}
			out->type = ORCUS_TYPE_TABLE;
			out->data.p = arr;
		}	
			return true;
		default:
			return false;
	}
}

void orcus_lib_io_deserialize(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable in;
	orcus_popstack(M, &in);

	if (in.type != ORCUS_TYPE_STRING) {
		orcus_release_var(&in);
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &in, ORCUS_TYPE_STRING, 1);
		return;
	}

	orcus_variable out;
	out.data.i = 0;
	out.type = ORCUS_TYPE_NIL;

	orcus_int pos = 0;
	orcus_string* str = (orcus_string*)in.data.p;

	if (!orcus_deserialize_variable(&out, (unsigned char*)str->ptr, str->size, &pos)) {
		orcus_release_var(&out);
		out.data.i = 0;
		out.type = ORCUS_TYPE_NIL;
	}


	orcus_release_var(&in);
	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



void orcus_lib_io_cwd(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0)) {
		return;
	}

	if (args_out) {
		char* buffer=nullptr;
		#ifdef _WIN32
		buffer=_getcwd(buffer, 0);
		#else
		buffer = getcwd(buffer, 0);
		#endif
		if (!buffer) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 0);
			return;
		}

		orcus_string* str = (orcus_string*)malloc(sizeof(orcus_string));
		if (!str) {
			free(buffer);
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
			return;
		}
		str->constant = false;
		str->ptr = buffer;
		str->references = 0;
		str->size = strlen(buffer);

		orcus_variable nvar;
		nvar.type = ORCUS_TYPE_STRING;
		nvar.data.p = str;
		orcus_pushstack(M, &nvar);
	}

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_io_executabledirectory(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0)) {
		return;
	}

	if (args_out) {
#ifdef _WIN32
		char* buffer = (char*)malloc(_MAX_PATH + 2);
#else
		char* buffer = (char*)malloc(PATH_MAX + 2);
#endif
		if (!buffer) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 0);
			return;
		}
#ifdef _WIN32
		GetModuleFileNameA(0, buffer, _MAX_PATH + 1);
#else
		ssize_t len = readlink("/proc/self/exe", buffer, PATH_MAX + 1);
		if (len == -1) {
			buffer[0] = '\0';
		}
		else {
			buffer[len] = '\0';
		}
#endif
		for (int ptr = strlen(buffer); ptr >= 0; ptr--) { //remove the executable itself from the path.
			if (buffer[ptr] == '/' || buffer[ptr] == '\\') {
				buffer[ptr] = '\0';
				break;
			}
		}

		orcus_string* str = (orcus_string*)malloc(sizeof(orcus_string));
		if (!str) {
			free(buffer);
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
			return;
		}
		str->constant = false;
		str->ptr = buffer;
		str->references = 0;
		str->size = strlen(buffer);

		orcus_variable nvar;
		nvar.type = ORCUS_TYPE_STRING;
		nvar.data.p = str;
		orcus_pushstack(M, &nvar);
	}

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_io_filelength(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		return;
	}
	if (!args_out) {
		orcus_discard_top_of_stack(M);
		return;
	}

	orcus_variable file_var;
	orcus_popstack(M, &file_var);
	if (file_var.type != ORCUS_TYPE_FILE) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &file_var, ORCUS_TYPE_FILE, 1);
		orcus_release_var(&file_var);
		return;
	}
	orcus_filewrapper* fw = (orcus_filewrapper*)file_var.data.p;

	
	orcus_variable out;
	out.type = ORCUS_TYPE_INT;

	if (fw->modeflags) {
		fseek(fw->file, 0, SEEK_END);
		out.data.i = ftell(fw->file);
		rewind(fw->file);
	}
	else {
		out.data.i = 0;
	}

	orcus_release_var(&file_var);

	orcus_pushstack(M, &out);

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}


void orcus_lib_io_userdirectory(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 0)) {
		return;
	}

	if (args_out) {
#ifdef _WIN32
		char* buffer = (char*)malloc(_MAX_PATH + 2);
#else
		char* buffer = (char*)malloc(PATH_MAX + 2);
#endif
		if (!buffer) {
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 0);
			return;
		}

		orcus_string* str = (orcus_string*)malloc(sizeof(orcus_string));
		if (!str) {
			free(buffer);
			orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
			ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out);
			return;
		}
		str->references = 0;

#ifdef _WIN32
		str->constant = false;
		if (SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, buffer) != S_OK) { //on faliure, return relative directory so that a user will see it as ./
			buffer[0] = '.'; 
			buffer[1] = '\0';
		}
		str->ptr = buffer;
		str->size = strlen(buffer);
#else
		if ((str->ptr=getenv("HOME")) ==nullptr) {
			str->ptr = getpwuid(getuid())->pw_dir;
			str->constant = true;
			str->size = strlen(str->ptr);
		}
		else {
			str->size = strlen(str->ptr);
			str->constant = true;
			free(buffer);
		}
#endif

	
		orcus_variable nvar;
		nvar.type = ORCUS_TYPE_STRING;
		nvar.data.p = str;
		orcus_pushstack(M, &nvar);
	}

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}



void orcus_lib_io_set_cwd(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out) {
	if (ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_INPUT_ARGS(M, args_in, 1)) {
		return;
	}

	orcus_variable pathvar;
	orcus_popstack(M,&pathvar);

	if (pathvar.type != ORCUS_TYPE_STRING) {
		orcus_raise_error_firstargpointeronly(M, ORCUS_ERROR_WRONG_TYPE_VARIABLEPROVIDED, &pathvar, ORCUS_TYPE_STRING, 1);
		orcus_release_var(&pathvar);
		return;
	}

	char* cstring = orcus_mstring_to_cstring((orcus_string*)pathvar.data.p);

	if (!cstring) {
		orcus_raise_error(M, ORCUS_ERROR_ALLOCATION);
		orcus_release_var(&pathvar);
		return;
	}

	bool success = false;

#ifdef _WIN32
	success = _chdir(cstring) == 0;
#else
	success = chdir(cstring) == 0;
#endif

	free(cstring);
	orcus_release_var(&pathvar);

	if (args_out) {
		pathvar.type = ORCUS_TYPE_BOOL;
		pathvar.data.i = success ? 1 : 0;
		orcus_pushstack(M, &pathvar);
	}

	ORCUS_CFUNCTION_ENSURE_CORRECT_NUMBER_OUTPUT_ARGS(M, args_out, 1);
}