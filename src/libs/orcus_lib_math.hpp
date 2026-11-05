/*
the standard library.
contains things for basic debugging and some misc. utility functions.

*/
#include "../orcus.hpp"


#define ORCUS_LIB_MATH


void orcus_lib_math_min(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_max(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_floor(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_ceil(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_to_radians(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_to_degrees(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_log(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_to_absolute(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_to_sin(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_to_cos(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_to_tan(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_to_asin(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_to_acos(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_to_atan(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_to_atan2(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_isnan(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_min_array(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_max_array(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_mean(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_mean_array(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_stddev(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_stddev_array(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_median(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_median_array(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);

void orcus_lib_math_random(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_randomint(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);
void orcus_lib_math_randomseed(orcus_state* const ORCUS_CPP_restrict M, const orcus_int args_in, const orcus_int args_out);

const orcus_float orcus_math_pi = 3.141592653589793; //15 digits should be enough i hope. doubles cannot store a more precise approximation than this.
const orcus_float orcus_math_root2 = 1.41421356237309504; //gee bill, 17 digits?
const orcus_float orcus_math_e = 2.718281828459045; // ditto.
const orcus_float orcus_math_root3 = 1.7320508075688772; //16 digits
const orcus_float orcus_math_golden = 1.6180339887498948; //16 digits
#ifdef ORCUS_64BIT
const orcus_int orcus_math_intmax = 0x7FFFFFFFFFFFFFFF;
const orcus_int orcus_math_uintmax =0xFFFFFFFFFFFFFFFF;
#else
const orcus_int orcus_math_intmax = 0x7FFFFFFF;
const orcus_int orcus_math_uintmax = 0xFFFFFFFF;
#endif


