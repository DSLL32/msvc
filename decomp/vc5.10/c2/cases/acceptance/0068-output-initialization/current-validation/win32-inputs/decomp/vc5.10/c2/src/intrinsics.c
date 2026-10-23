#include "c2.h"
#include <recon.h>

/* Shared recipe/name identities are observable in the initialized table.
 * Ordinary names used once remain literals in their descriptor rows. */
char empty_intrinsic_text[] = "";
static char call_runtime_recipe[] = "C";
static char runtime_aint[] = "aint";
static char runtime_anint[] = "anint";
static char empty_runtime_name[] = "";
static char previous_intrinsic_recipe[] = "d";
static char unary_operator_recipe[] = "u";
static char binary_operator_recipe[] = "b";
static char runtime_amod_fmod[] = "amod\377fmod";
static char recipe_s1MPPnPDnDDnnnn[] = "s1MPPnPDnDDnnnn";
static char recipe_mb[] = "mb";
static char recipe_ma[] = "ma";
static char recipe_SxC[] = "SxC";
static char runtime_sqrt[] = "sqrt";
static char runtime_exp[] = "exp";
static char runtime_cexp[] = "cexp";
static char runtime_log[] = "log";
static char runtime_clog[] = "clog";
static char runtime_alog10_log10[] = "alog10\377log10";
static char runtime_sin[] = "sin";
static char runtime_csin[] = "csin";
static char runtime_cos[] = "cos";
static char runtime_ccos[] = "ccos";
static char runtime_tan[] = "tan";
static char runtime_cotan[] = "cotan";
static char runtime_erf[] = "erf";
static char runtime_erfc[] = "erfc";
static char runtime_gamma[] = "gamma";
static char runtime_asin[] = "asin";
static char runtime_acos[] = "acos";
static char runtime_atan[] = "atan";
static char runtime_atan2[] = "atan2";
static char runtime_sinh[] = "sinh";
static char runtime_cosh[] = "cosh";
static char runtime_tanh[] = "tanh";
static char reverse_call_recipe[] = "vC";
static char recipe_E0[] = "E0";
static char runtime_FClccheck[] = "FClccheck";
static char reverse_binary_recipe[] = "vb";
static char normalized_copy_recipe[] = "nvyE0E1";
static char reverse_both_arguments_recipe[] = "vE0E1";
static char reverse_first_argument_recipe[] = "vE0";
static char recipe_t[] = "t";
static char recipe_vt[] = "vt";
static char reverse_arguments_recipe[] = "v";

C2Intrinsic intrinsics[C2_INTRINSIC_COUNT];

/* Serialized intrinsic numbers are specific to this backend. The default
 * records all have zero type/flags and the same shared empty recipe/name.
 * Only the247 nondefault records need explicit descriptor rows. */
const C2IntrinsicInitializer intrinsic_initializers[] = {
    { 1, 0x4004, empty_intrinsic_text, empty_intrinsic_text, 0x20, 0 },
    { 2, 0x4008, empty_intrinsic_text, empty_intrinsic_text, 0x20, 0 },
    { 3, 0x4004, call_runtime_recipe, runtime_aint, 0x20, 0 },
    { 4, 0x4008, call_runtime_recipe, runtime_aint, 0x20, 0 },
    { 5, 0x4004, call_runtime_recipe, runtime_anint, 0x20, 0 },
    { 6, 0x4008, call_runtime_recipe, runtime_anint, 0x20, 0 },
    { 7, 0x4004, "s0MPPnDPnDPnnnL", empty_runtime_name, 0x00, 0 },
    { 8, 0x4004, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 9, 0x4004, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 10, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 11, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 12, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 13, 0x1001, unary_operator_recipe, empty_intrinsic_text, 0x20, 0 },
    { 14, 0x1002, unary_operator_recipe, empty_intrinsic_text, 0x20, 0 },
    { 15, 0x1004, unary_operator_recipe, empty_intrinsic_text, 0x20, 0 },
    { 16, 0x4004, unary_operator_recipe, empty_intrinsic_text, 0x20, 0 },
    { 17, 0x4008, unary_operator_recipe, empty_intrinsic_text, 0x20, 0 },
    { 18, 0x4004, call_runtime_recipe, "cabs", 0x20, 11 },
    { 19, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x20, 0 },
    { 20, 0x1001, binary_operator_recipe, empty_intrinsic_text, 0x20, 0 },
    { 21, 0x1002, previous_intrinsic_recipe, empty_intrinsic_text, 0x20, 0 },
    { 22, 0x1004, previous_intrinsic_recipe, empty_intrinsic_text, 0x20, 0 },
    { 23, 0x4004, "wCY", runtime_amod_fmod, 0x20, 43 },
    { 24, 0x4008, previous_intrinsic_recipe, runtime_amod_fmod, 0x20, 43 },
    { 25, 0x1001, "s0MPPnPnDDnnn", empty_runtime_name, 0x00, 0 },
    { 26, 0x1002, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 27, 0x1004, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 28, 0x4004, call_runtime_recipe, "sign", 0x20, 11 },
    { 29, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x20, 0 },
    { 30, 0x1001, "s1MPPnPn", empty_runtime_name, 0x00, 0 },
    { 31, 0x1002, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 32, 0x1004, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 33, 0x4004, call_runtime_recipe, "dim", 0x20, 11 },
    { 34, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x20, 0 },
    { 35, 0x1001, binary_operator_recipe, empty_intrinsic_text, 0x00, 0 },
    { 36, 0x1002, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 37, 0x1004, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 38, 0x1001, binary_operator_recipe, empty_intrinsic_text, 0x00, 0 },
    { 39, 0x1002, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 40, 0x1004, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 41, 0x1001, binary_operator_recipe, empty_intrinsic_text, 0x00, 0 },
    { 42, 0x1002, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 43, 0x1004, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 44, 0x1001, unary_operator_recipe, empty_intrinsic_text, 0x00, 0 },
    { 45, 0x1002, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 46, 0x1004, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 47, 0x0000, recipe_s1MPPnPDnDDnnnn, empty_runtime_name, 0x10, 0 },
    { 48, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x10, 0 },
    { 49, 0x0000, empty_intrinsic_text, empty_intrinsic_text, 0x10, 0 },
    { 50, 0x0000, recipe_s1MPPnPDnDDnnnn, empty_intrinsic_text, 0x10, 0 },
    { 51, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x10, 0 },
    { 52, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x10, 0 },
    { 53, 0x1001, recipe_s1MPPnPDnDDnnnn, empty_runtime_name, 0x00, 0 },
    { 54, 0x1002, previous_intrinsic_recipe, empty_intrinsic_text, 0x00, 0 },
    { 55, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 56, 0x1001, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 57, 0x1002, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 58, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 59, 0x1001, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 60, 0x1002, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 61, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 62, 0x1001, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 63, 0x1002, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 64, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 65, 0x1001, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 66, 0x1002, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 67, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 71, 0x4004, call_runtime_recipe, "dprod", 0x20, 11 },
    { 72, 0x1004, recipe_mb, empty_intrinsic_text, 0x00, 0 },
    { 73, 0x4004, recipe_mb, empty_intrinsic_text, 0x20, 0 },
    { 74, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x20, 0 },
    { 75, 0x1004, recipe_ma, empty_intrinsic_text, 0x00, 0 },
    { 76, 0x4008, recipe_ma, empty_intrinsic_text, 0x00, 0 },
    { 77, 0x1004, recipe_mb, empty_intrinsic_text, 0x00, 0 },
    { 78, 0x4004, recipe_mb, empty_intrinsic_text, 0x20, 0 },
    { 79, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x20, 0 },
    { 80, 0x1004, recipe_ma, empty_intrinsic_text, 0x00, 0 },
    { 81, 0x4008, recipe_ma, empty_intrinsic_text, 0x00, 0 },
    { 82, 0x0000, call_runtime_recipe, "len1", 0x00, 0 },
    { 83, 0x0000, call_runtime_recipe, "len2", 0x00, 0 },
    { 84, 0x0000, call_runtime_recipe, "len4", 0x00, 0 },
    { 85, 0x0000, "hC", "FClindex", 0x40, 0 },
    { 86, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x40, 0 },
    { 87, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x40, 0 },
    { 88, 0x4004, recipe_SxC, "aimag", 0x20, 0 },
    { 89, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x20, 0 },
    { 90, 0x4004, recipe_SxC, "conjg", 0x20, 0 },
    { 91, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x20, 0 },
    { 92, 0x4004, call_runtime_recipe, runtime_sqrt, 0x20, 1 },
    { 93, 0x4008, previous_intrinsic_recipe, runtime_sqrt, 0x20, 1 },
    { 94, 0x4004, call_runtime_recipe, "csqrt", 0x20, 0 },
    { 95, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x20, 0 },
    { 96, 0x4004, call_runtime_recipe, runtime_exp, 0x20, 3 },
    { 97, 0x4008, previous_intrinsic_recipe, runtime_exp, 0x20, 0 },
    { 98, 0x4004, call_runtime_recipe, runtime_cexp, 0x20, 4 },
    { 99, 0x4008, previous_intrinsic_recipe, runtime_cexp, 0x20, 0 },
    { 100, 0x4004, call_runtime_recipe, runtime_log, 0x20, 5 },
    { 101, 0x4008, previous_intrinsic_recipe, runtime_log, 0x20, 0 },
    { 102, 0x4004, call_runtime_recipe, runtime_clog, 0x20, 5 },
    { 103, 0x4008, previous_intrinsic_recipe, runtime_clog, 0x20, 0 },
    { 104, 0x4004, "CY", runtime_alog10_log10, 0x20, 5 },
    { 105, 0x4008, previous_intrinsic_recipe, runtime_alog10_log10, 0x20, 5 },
    { 106, 0x4004, call_runtime_recipe, runtime_sin, 0x20, 4 },
    { 107, 0x4008, previous_intrinsic_recipe, runtime_sin, 0x20, 4 },
    { 108, 0x4004, call_runtime_recipe, runtime_csin, 0x20, 5 },
    { 109, 0x4008, previous_intrinsic_recipe, runtime_csin, 0x20, 5 },
    { 110, 0x4004, call_runtime_recipe, runtime_cos, 0x20, 4 },
    { 111, 0x4008, previous_intrinsic_recipe, runtime_cos, 0x20, 4 },
    { 112, 0x4004, call_runtime_recipe, runtime_ccos, 0x20, 5 },
    { 113, 0x4008, previous_intrinsic_recipe, runtime_ccos, 0x20, 5 },
    { 114, 0x4004, call_runtime_recipe, runtime_tan, 0x20, 3 },
    { 115, 0x4008, previous_intrinsic_recipe, runtime_tan, 0x20, 3 },
    { 116, 0x4004, call_runtime_recipe, runtime_cotan, 0x20, 3 },
    { 117, 0x4008, previous_intrinsic_recipe, runtime_cotan, 0x20, 3 },
    { 118, 0x4004, call_runtime_recipe, runtime_erf, 0x20, 5 },
    { 119, 0x4008, previous_intrinsic_recipe, runtime_erf, 0x20, 5 },
    { 120, 0x4004, call_runtime_recipe, runtime_erfc, 0x20, 5 },
    { 121, 0x4008, previous_intrinsic_recipe, runtime_erfc, 0x20, 5 },
    { 122, 0x4004, call_runtime_recipe, runtime_gamma, 0x20, 5 },
    { 123, 0x4008, previous_intrinsic_recipe, runtime_gamma, 0x20, 5 },
    { 124, 0x4004, call_runtime_recipe, "algama", 0x20, 5 },
    { 125, 0x4008, previous_intrinsic_recipe, "algamma", 0x20, 5 },
    { 126, 0x4004, call_runtime_recipe, runtime_asin, 0x20, 4 },
    { 127, 0x4008, previous_intrinsic_recipe, runtime_asin, 0x20, 4 },
    { 128, 0x4004, call_runtime_recipe, runtime_acos, 0x20, 4 },
    { 129, 0x4008, previous_intrinsic_recipe, runtime_acos, 0x20, 4 },
    { 130, 0x4004, call_runtime_recipe, runtime_atan, 0x20, 2 },
    { 131, 0x4008, previous_intrinsic_recipe, runtime_atan, 0x20, 2 },
    { 132, 0x4004, "wC", runtime_atan2, 0x20, 42 },
    { 133, 0x4008, previous_intrinsic_recipe, runtime_atan2, 0x20, 42 },
    { 134, 0x4004, call_runtime_recipe, runtime_sinh, 0x20, 4 },
    { 135, 0x4008, previous_intrinsic_recipe, runtime_sinh, 0x20, 4 },
    { 136, 0x4004, call_runtime_recipe, runtime_cosh, 0x20, 4 },
    { 137, 0x4008, previous_intrinsic_recipe, runtime_cosh, 0x20, 4 },
    { 138, 0x4004, call_runtime_recipe, runtime_tanh, 0x20, 5 },
    { 139, 0x4008, previous_intrinsic_recipe, runtime_tanh, 0x20, 5 },
    { 140, 0x0000, reverse_call_recipe, "clge@16", 0x40, 0 },
    { 141, 0x0000, reverse_call_recipe, "clgt@16", 0x40, 0 },
    { 142, 0x0000, reverse_call_recipe, "clle@16", 0x40, 0 },
    { 143, 0x0000, reverse_call_recipe, "cllt@16", 0x40, 0 },
    { 144, 0x0000, reverse_call_recipe, "clne@16", 0x40, 0 },
    { 145, 0x0000, reverse_call_recipe, "cleq@16", 0x40, 0 },
    { 146, 0x1002, call_runtime_recipe, "FFeof", 0x00, 0 },
    { 150, 0x0000, "E0E1", empty_intrinsic_text, 0x10, 0 },
    { 151, 0x0000, recipe_E0, empty_intrinsic_text, 0x10, 0 },
    { 152, 0x1004, "zijMPPnPn", empty_runtime_name, 0x80, 0 },
    { 153, 0x1004, "zis1MPPPnPnNtYPDnPnn", empty_runtime_name, 0x80, 0 },
    { 154, 0x1004, "DZ", runtime_FClccheck, 0x80, 0 },
    { 155, 0x1004, "LZ", runtime_FClccheck, 0x80, 0 },
    { 147, 0x0000, "N1KC", "FClcconc", 0x40, 0 },
    { 148, 0x0000, "N1cC", "FClccop", 0x40, 0 },
    { 157, 0x0000, binary_operator_recipe, empty_runtime_name, 0x00, 0 },
    { 156, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 301, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 158, 0x0000, recipe_E0, empty_intrinsic_text, 0x00, 0 },
    { 159, 0x1004, reverse_binary_recipe, empty_intrinsic_text, 0x00, 0 },
    { 160, 0x1004, reverse_binary_recipe, empty_intrinsic_text, 0x00, 0 },
    { 161, 0x1004, reverse_binary_recipe, empty_intrinsic_text, 0x00, 0 },
    { 162, 0x1004, reverse_binary_recipe, empty_intrinsic_text, 0x00, 0 },
    { 163, 0x1004, "vyE1", empty_intrinsic_text, 0x00, 0 },
    { 164, 0x3004, normalized_copy_recipe, empty_intrinsic_text, 0x40, 0 },
    { 165, 0x3004, reverse_both_arguments_recipe, empty_intrinsic_text, 0x40, 0 },
    { 166, 0x3004, normalized_copy_recipe, empty_intrinsic_text, 0x40, 0 },
    { 167, 0x3004, reverse_first_argument_recipe, empty_intrinsic_text, 0x40, 0 },
    { 168, 0x2002, "ot", empty_intrinsic_text, 0x00, 0 },
    { 169, 0x2002, "vot", empty_intrinsic_text, 0x10, 0 },
    { 170, 0x1004, reverse_both_arguments_recipe, empty_intrinsic_text, 0x00, 0 },
    { 171, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 172, 0x1004, normalized_copy_recipe, empty_intrinsic_text, 0x00, 0 },
    { 173, 0x1004, "vyE0", empty_intrinsic_text, 0x00, 0 },
    { 174, 0x4008, reverse_binary_recipe, "fexp\377pow", 0x20, 43 },
    { 175, 0x2002, recipe_t, empty_intrinsic_text, 0x00, 0 },
    { 176, 0x2002, recipe_vt, empty_intrinsic_text, 0x00, 0 },
    { 179, 0x0000, "nE0", empty_intrinsic_text, 0x00, 0 },
    { 180, 0x4004, call_runtime_recipe, "nearest", 0x20, 0 },
    { 181, 0x4008, previous_intrinsic_recipe, "nearsest", 0x20, 0 },
    { 182, 0x0000, reverse_call_recipe, "index@20", 0x40, 0 },
    { 183, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x40, 0 },
    { 184, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x40, 0 },
    { 185, 0x0000, reverse_call_recipe, "lentrim@8", 0x40, 0 },
    { 186, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x40, 0 },
    { 187, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x40, 0 },
    { 188, 0x0000, reverse_call_recipe, "scan@20", 0x40, 0 },
    { 189, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x40, 0 },
    { 190, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x40, 0 },
    { 191, 0x0000, reverse_call_recipe, "verify@20", 0x40, 0 },
    { 192, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x40, 0 },
    { 193, 0x0000, previous_intrinsic_recipe, empty_intrinsic_text, 0x40, 0 },
    { 195, 0x4008, call_runtime_recipe, "ceil", 0x20, 0 },
    { 196, 0x4008, call_runtime_recipe, "floor", 0x20, 0 },
    { 197, 0x400a, call_runtime_recipe, "acosl", 0x20, 0 },
    { 198, 0x400a, call_runtime_recipe, "asinl", 0x20, 0 },
    { 199, 0x400a, call_runtime_recipe, "atanl", 0x20, 0 },
    { 200, 0x400a, call_runtime_recipe, "atan2l", 0x20, 0 },
    { 201, 0x400a, call_runtime_recipe, "ceill", 0x20, 0 },
    { 202, 0x400a, call_runtime_recipe, "cosl", 0x20, 0 },
    { 203, 0x400a, call_runtime_recipe, "coshl", 0x20, 0 },
    { 204, 0x400a, call_runtime_recipe, "expl", 0x20, 0 },
    { 205, 0x400a, call_runtime_recipe, "floorl", 0x20, 0 },
    { 206, 0x400a, call_runtime_recipe, "fmodl", 0x20, 0 },
    { 207, 0x400a, call_runtime_recipe, "logl", 0x20, 0 },
    { 208, 0x400a, call_runtime_recipe, "log10l", 0x20, 0 },
    { 209, 0x400a, reverse_binary_recipe, empty_intrinsic_text, 0x20, 0 },
    { 210, 0x400a, call_runtime_recipe, "sinl", 0x20, 0 },
    { 211, 0x400a, call_runtime_recipe, "sinhl", 0x20, 0 },
    { 212, 0x400a, call_runtime_recipe, "sqrtl", 0x20, 0 },
    { 213, 0x400a, call_runtime_recipe, "tanl", 0x20, 0 },
    { 214, 0x400a, call_runtime_recipe, "tanhl", 0x20, 0 },
    { 216, 0x3004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 217, 0x2004, empty_intrinsic_text, empty_intrinsic_text, 0x20, 0 },
    { 218, 0x3004, empty_intrinsic_text, empty_intrinsic_text, 0x20, 0 },
    { 219, 0x3004, empty_intrinsic_text, "setjmp", 0x00, 0 },
    { 220, 0x3004, empty_intrinsic_text, "va_arg", 0x00, 0 },
    { 221, 0x3004, empty_intrinsic_text, "va_start", 0x00, 0 },
    { 224, 0x2002, recipe_t, empty_intrinsic_text, 0x00, 0 },
    { 225, 0x2002, recipe_vt, empty_intrinsic_text, 0x10, 0 },
    { 226, 0x3004, reverse_first_argument_recipe, empty_intrinsic_text, 0x00, 0 },
    { 227, 0x3004, reverse_first_argument_recipe, empty_intrinsic_text, 0x00, 0 },
    { 228, 0x3004, reverse_first_argument_recipe, empty_intrinsic_text, 0x10, 0 },
    { 229, 0x3004, empty_intrinsic_text, empty_intrinsic_text, 0x20, 0 },
    { 302, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 303, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 304, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 305, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x00, 0 },
    { 309, 0x4004, call_runtime_recipe, "cdiv", 0x20, 16 },
    { 310, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x20, 16 },
    { 311, 0x4004, call_runtime_recipe, "cpow", 0x20, 16 },
    { 312, 0x4008, previous_intrinsic_recipe, empty_intrinsic_text, 0x20, 16 },
    { 230, 0x4004, empty_intrinsic_text, empty_intrinsic_text, 0x20, 0 },
    { 231, 0x4004, empty_intrinsic_text, empty_intrinsic_text, 0x20, 0 },
    { 232, 0x4004, empty_intrinsic_text, empty_intrinsic_text, 0x20, 0 },
    { 233, 0x0000, reverse_arguments_recipe, empty_intrinsic_text, 0x30, 0 },
    { 234, 0x0000, reverse_arguments_recipe, empty_intrinsic_text, 0x30, 0 },
    { 235, 0x0000, reverse_arguments_recipe, empty_intrinsic_text, 0x30, 0 },
    { 236, 0x1004, reverse_arguments_recipe, empty_intrinsic_text, 0x20, 0 },
    { 237, 0x2004, reverse_arguments_recipe, empty_intrinsic_text, 0x20, 0 },
    { 313, 0x2004, reverse_arguments_recipe, "__emit", 0x00, 0 },
    { 314, 0x2004, reverse_arguments_recipe, "__gregister_get", 0x00, 0 },
    { 315, 0x2004, reverse_arguments_recipe, "__gregister_set", 0x00, 0 },
    { 316, 0x2004, reverse_arguments_recipe, "__sregister_get", 0x00, 0 },
    { 317, 0x2004, reverse_arguments_recipe, "__sregister_set", 0x00, 0 },
    { 318, 0x3004, reverse_first_argument_recipe, empty_intrinsic_text, 0x10, 0 },
    { 319, 0x3004, reverse_first_argument_recipe, empty_intrinsic_text, 0x00, 0 },
    { 331, 0x3004, reverse_first_argument_recipe, empty_intrinsic_text, 0x10, 0 },
    { 336, 0x1004, empty_intrinsic_text, empty_intrinsic_text, 0x20, 0 },
    { 339, 0x0000, empty_intrinsic_text, empty_intrinsic_text, 0x20, 0 },
    { 340, 0x0000, empty_intrinsic_text, empty_intrinsic_text, 0x20, 0 },
    { C2_INTRINSIC_COUNT }
};

void initialize_intrinsics(void)
{
    unsigned i;
    const C2IntrinsicInitializer *definition;
    C2Intrinsic *description;

    /* Leave both reserved fields untouched, as the original does. */
    for (i = 0; i < C2_INTRINSIC_COUNT; ++i) {
        description = &intrinsics[i];
        description->result_type = 0;
        description->commands = empty_intrinsic_text;
        description->runtime_name = empty_intrinsic_text;
        description->flags = 0;
        description->argument_signature = 0;
    }
    for (definition = intrinsic_initializers;
         definition->number < C2_INTRINSIC_COUNT; ++definition) {
        description = &intrinsics[definition->number];
        description->flags = definition->flags;
        description->argument_signature = definition->argument_signature;
        description->commands = definition->commands;
        description->runtime_name = definition->runtime_name;
        description->result_type = definition->result_type;
    }
}
