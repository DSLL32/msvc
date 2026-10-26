#ifndef C1_PRAGMA_H
#define C1_PRAGMA_H
#include "compiler-state.h"
#include <recon.h>
/* C1 token selectors0..49; C++-only map-region/implementation selectors omitted. */
enum CompilerPragma {
    PRAGMA_AUTO_INLINE = 27,
    PRAGMA_ALLOC_TEXT = 5,
    PRAGMA_BSS_SEG = 48,
    PRAGMA_CONST_SEG = 49,
    PRAGMA_COMPONENT = 46,
    PRAGMA_COMMENT = 19,
    PRAGMA_CODE_SEG = 24,
    PRAGMA_CHECK_STACK = 0,
    PRAGMA_CHECK_POINTER = 7,
    PRAGMA_DATA_SEG = 22,
    PRAGMA_FUNCTION = 4,
    PRAGMA_HDRSTOP = 29,
    PRAGMA_INTRINSIC = 3,
    PRAGMA_INLINE_RECURSION = 26,
    PRAGMA_INLINE_DEPTH = 25,
    PRAGMA_INIT_SEG = 30,
    PRAGMA_INCLUDE_ALIAS = 36,
    PRAGMA_LOOP_OPT = 1,
    PRAGMA_LINESIZE = 16,
    PRAGMA_MESSAGE = 18,
    PRAGMA_NATIVE_CALLER = 23,
    PRAGMA_OPTIONS = 42,
    PRAGMA_OPTIMIZE = 21,
    PRAGMA_ONCE = 37,
    PRAGMA_PUSH = 40,
    PRAGMA_PROCNAME = 39,
    PRAGMA_POP = 38,
    PRAGMA_POINTERS_TO_MEMBERS = 33,
    PRAGMA_PLMN = 9,
    PRAGMA_PLMF = 10,
    PRAGMA_PARAMETER = 35,
    PRAGMA_PAGESIZE = 15,
    PRAGMA_PAGE = 11,
    PRAGMA_PACK = 2,
    PRAGMA_SWITCH_CHECK = 8,
    PRAGMA_SUBTITLE = 13,
    PRAGMA_STRING_POOLING = 34,
    PRAGMA_SKIPPING = 41,
    PRAGMA_SKIP = 12,
    PRAGMA_SETLOCALE = 32,
    PRAGMA_SEGMENT = 20,
    PRAGMA_SEARCH_LIB = 17,
    PRAGMA_SAME_SEG = 6,
    PRAGMA_TITLE = 14,
    PRAGMA_VTORDISP = 31,
    PRAGMA_WARNING = 28,
    PRAGMA_UNKNOWN = -1
};
struct CompilerIrSymbol;
/* These existing pragma/segment cells remain source-only until their first
 * supported declaration consumer establishes defaults and restoration. */
#define declaration_reference_defaults c1_data.segment_defaults
extern unsigned control_option_kind;
extern const BYTE *control_option_value;
extern WORD control_state_first, control_state_second;
extern void *c1_pch_stop_owner; /* RVA5f31c, source-only until PCH creation */
#endif
