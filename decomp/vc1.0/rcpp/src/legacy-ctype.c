/* Classification produced by native negative ctype indices. Most of this
 * original prefix covers unused FILE slots 14..19, which remain zero: RCPP
 * has three standard streams, one output and at most ten active input files.
 * The remaining class bits arise from the embedded CRT's last-stream, locale
 * and ctype-pointer storage before its C-locale array. Only flags tested by
 * RCPP are represented, never the original pointer words or section bytes. */
#include "legacy-ctype.h"
#include <recon.h>
#define A 0x100 /* The CRT's additional alphabetic classification bit. */
unsigned short rc_negative_native_ctype_flags[128] = {
    0,0,0,0,0,0,0,0, /* -128..-121 */
    0,0,0,0,0,0,0,0, /* -120..-113 */
    0,0,0,0,0,0,0,0, /* -112..-105 */
    0,0,0,0,0,0,0,0, /* -104..-97 */
    0,0,0,0,0,0,0,0, /* -96..-89 */
    0,0,0,0,0,0,0,0, /* -88..-81 */
    0,0,0,0,0,0,0,0, /* -80..-73 */
    0,0,0,0,0,0,0,0, /* -72..-65 */
    0,0,0,0,0,0,0,0, /* -64..-57 */
    0,0,0,0,0,0,0,0, /* -56..-49 */
    0,0,0,0,0,0,0,0, /* -48..-41 */
    0,0,0,A,A|_UPPER,0,0,0, /* -40..-33 */
    0,0,0,_UPPER,0,_LOWER|_DIGIT|_SPACE,0,A|_PUNCT|_DIGIT, /* -32..-25 */
    A|_UPPER,_UPPER,0,0,0,0,0,0, /* -24..-17 */
    0,0,0,0,0,0,0,0, /* -16..-9 */
    0,0,0,A|_BLANK|_SPACE|_LOWER,A|_UPPER,A|_BLANK|_SPACE|_LOWER,A|_UPPER,0, /* -8..-1 */
};
#undef A
