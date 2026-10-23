/* Builtin declaration-name lookup, grouped by the first identifier byte. */
#include "declarations.h"
#include "initialization.h"
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

/* Original declaration rules at0xa0780, in the same order as name suffixes.
   The operation and low flag word become the declaration's intrinsic state;
   the result-type flags also distinguish precise floating-point operations. */
typedef struct IntrinsicRule {
    unsigned operation;
    unsigned result_type;
    unsigned short flags, category;
} IntrinsicRule;
typedef char IntrinsicRuleSizeMustBe12[(sizeof(IntrinsicRule) == 12) ? 1 : -1];
IntrinsicRule intrinsic_rules[51] = {
    {0xa3,0x10041,0x02,0x10}, /* _strset */
    {0xdb,0x00064,0x81,0x00}, /* _setjmp */
    {0xa0,0x04064,0x02,0x10}, /* _rotr */
    {0x9f,0x04064,0x02,0x10}, /* _rotl */
    {0xb0,0x04042,0x02,0x10}, /* _outpw */
    {0xe1,0x04044,0x02,0x10}, /* _outpd */
    {0xa9,0x00064,0x02,0x10}, /* _outp */
    {0xa2,0x04044,0x02,0x10}, /* _lrotr */
    {0xa1,0x04044,0x02,0x10}, /* _lrotl */
    {0xaf,0x04042,0x01,0x10}, /* _inpw */
    {0xe0,0x04044,0x01,0x10}, /* _inpd */
    {0xa8,0x00064,0x01,0x10}, /* _inp */
    {0xda,0x10080,0x80,0x00}, /* _exception_info */
    {0xd9,0x04044,0x80,0x00}, /* _exception_code */
    {0xb1,0x00080,0x00,0x00}, /* _enable */
    {0xb2,0x00080,0x00,0x00}, /* _disable */
    {0xd8,0x10080,0x81,0x00}, /* _alloca */
    {0xd7,0x00064,0x80,0x00}, /* _abnormal_termination */
    {0x85,0x00108,0x02,0x20}, /* atan2 */
    {0x83,0x00108,0x01,0x20}, /* atan */
    {0x7f,0x00108,0x01,0x20}, /* asin */
    {0x81,0x00108,0x01,0x20}, /* acos */
    {0x0f,0x00064,0x01,0x10}, /* abs */
    {0x89,0x00108,0x01,0x20}, /* cosh */
    {0x6f,0x00108,0x01,0x20}, /* cos */
    {0x61,0x00108,0x01,0x20}, /* exp */
    {0x18,0x00108,0x02,0x20}, /* fmod */
    {0x11,0x00108,0x01,0x20}, /* fabs */
    {0xaf,0x04042,0x41,0x10}, /* inpw */
    {0xe0,0x04044,0x41,0x10}, /* inpd */
    {0xa8,0x00064,0x41,0x10}, /* inp */
    {0x69,0x00108,0x01,0x20}, /* log10 */
    {0x65,0x00108,0x01,0x20}, /* log */
    {0x0f,0x00044,0x01,0x10}, /* labs */
    {0xad,0x10080,0x03,0x10}, /* memset */
    {0xac,0x10080,0x03,0x10}, /* memcpy */
    {0xaa,0x00064,0x03,0x10}, /* memcmp */
    {0xb0,0x04042,0x42,0x10}, /* outpw */
    {0xe1,0x04064,0x42,0x10}, /* outpd */
    {0xa9,0x00064,0x42,0x10}, /* outp */
    {0xae,0x00108,0x02,0x20}, /* pow */
    {0xa3,0x10041,0x42,0x10}, /* strset */
    {0xa7,0x04064,0x01,0x10}, /* strlen */
    {0xa4,0x10041,0x02,0x10}, /* strcpy */
    {0xa5,0x00064,0x02,0x10}, /* strcmp */
    {0xa6,0x10041,0x02,0x10}, /* strcat */
    {0x5d,0x00108,0x01,0x20}, /* sqrt */
    {0x87,0x00108,0x01,0x20}, /* sinh */
    {0x6b,0x00108,0x01,0x20}, /* sin */
    {0x8b,0x00108,0x01,0x20}, /* tanh */
    {0x73,0x00108,0x01,0x20}  /* tan */
};

/* Original 0x3d450..0x3d4d5. Explicitly mandatory intrinsics bypass
   /Oi; extension and precise-float modes restrict the remaining rules. The
   original comparisons against172/173 are retained as written. */
int __stdcall intrinsic_enabled(int index)
{
    IntrinsicRule *rule;
    if ((options.configuration.option_MT & 4) && index >= 172 && index <= 173)
        return 0;
    rule = &intrinsic_rules[index];
    if (rule->flags & 0x80) return 1;
    if (!options.configuration.option_Oi) return 0;
    if (!options.configuration.extensions_enabled && (rule->flags & 0x40)) return 0;
    if (normalized_Op && (rule->result_type & 0x100)) return 0;
    return 1;
}
