#ifndef C1XX_KEYWORDS_H
#define C1XX_KEYWORDS_H
#include "names.h"
#include <recon.h>

/* The keyword-only extension follows the common header in the compact view.
 * Original token/category are +10/+11; original+c and the tail stay unmodeled.
 * Generic records expose only CompilerNameEntry, never this extension. */
struct CompilerIdentifier {
    CompilerNameEntry header;
    BYTE token, category;
};
struct CompilerKeywordTable {
    CompilerNameEntry *buckets[2048]; /* hash mask 0x7ff; allocation8192 */
};
#endif
