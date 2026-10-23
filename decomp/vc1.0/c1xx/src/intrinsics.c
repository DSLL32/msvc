/* Builtin declaration-name lookup, grouped by the first identifier byte. */
#include "declarations.h"
#include <recon.h>

/* The omitted first byte is supplied by the group. For example the initial
   underscore group contains _strset, while the a group contains atan2..abs. */
char *intrinsic_name_suffixes[52] = {
    "strset", "setjmp", "rotr", "rotl", "outpw", "outpd", "outp",
    "lrotr", "lrotl", "inpw", "inpd", "inp", "exception_info",
    "exception_code", "enable", "disable", "alloca", "abnormal_termination",
    "tan2", "tan", "sin", "cos", "bs", "osh", "os", "xp", "mod", "abs",
    "npw", "npd", "np", "og10", "og", "abs", "emset", "emcpy", "emcmp",
    "utpw", "utpd", "utp", "ow", "trset", "trlen", "trcpy", "trcmp",
    "trcat", "qrt", "inh", "in", "anh", "an", 0
};
/* Bounds for '_', '`', 'a'..'z', followed by the final end bound. */
signed char intrinsic_name_groups[29] = {
    0,18,18,23,23,25,25,26,28,28,28,31,31,31,34,37,37,40,41,41,41,
    49,51,51,51,51,51,51,51
};

/* Original 0x3d320..0x3d3ab. Search just the first-byte group and
   return the intrinsic's position, preserving the signed initial-byte test. */
int __stdcall find_intrinsic_index(char *name)
{
    char first = *name++;
    char *left, *right;
    int index, end;
    if (first < '_') return -1;
    index = intrinsic_name_groups[first - '_'];
    end = intrinsic_name_groups[first - '_' + 1];
    for (; index != end; ++index) {
        left = intrinsic_name_suffixes[index];
        right = name;
        while (*left == *right && *left) { ++left; ++right; }
        if (*left == *right) return index;
    }
    return -1;
}
