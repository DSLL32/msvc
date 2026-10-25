#ifndef C1XX_KEYWORDS_H
#define C1XX_KEYWORDS_H
#include <recon.h>

/* Original next/name/flags are +0/+4/+8, token/category +16/+17.
 * The compact projection has no placeholder for original +12 or tail bytes.
 * The actual 20-byte allocation and hash-bucket width remain unchanged. */
struct CompilerIdentifier {
    CompilerIdentifier *next;
    char *name;
    unsigned flags;
    BYTE token, category;
};
struct CompilerKeywordTable {
    CompilerIdentifier *buckets[2048]; /* hash mask 0x7ff; allocation 8192 */
};
#endif
