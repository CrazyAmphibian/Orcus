#pragma once
/*
 io library.
 contains functions for file reading or modification
*/
#include "../orcus.hpp"

#define ORCUS_LIB_IO

void orcus_lib_io_open(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_read(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_close(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_write(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_getfiles(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_getdirs(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_lines(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_post(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_prompt(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_input(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_remove(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_removedir(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_createdir(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_serialize(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_deserialize(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_cwd(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_executabledirectory(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_filelength(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_userdirectory(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_io_set_cwd(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);

