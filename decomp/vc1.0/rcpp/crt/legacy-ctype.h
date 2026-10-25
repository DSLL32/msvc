#ifndef RCPP_LEGACY_CTYPE_H
#define RCPP_LEGACY_CTYPE_H
#include <ctype.h>
#include <recon.h>
/* Native RCPP passes signed bytes to these macros. The original table's
 * negative prefix differs from later LIBC layout. Retain the actual observed
 * PRINT/SPACE/DIGIT flags, while ordinary indices use compatible LIBC data.
 * This is inline so the native _isctype call edge/ABI remains unchanged. */
extern unsigned short rc_negative_native_ctype_flags[128];
#define RC_NATIVE_CTYPE(ch,mask) \
    ((unsigned short)MB_CUR_MAX > 1 ? _isctype((ch),(mask)) : \
     ((ch) < 0 ? rc_negative_native_ctype_flags[(ch)+128] : _pctype[(ch)]) & (mask))
#define rc_native_isprint(ch) RC_NATIVE_CTYPE(ch,_BLANK|_PUNCT|_ALPHA|_DIGIT)
#define rc_native_isspace(ch) RC_NATIVE_CTYPE(ch,_SPACE)
#define rc_native_isdigit(ch) RC_NATIVE_CTYPE(ch,_DIGIT)
#endif
