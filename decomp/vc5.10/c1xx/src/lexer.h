#ifndef C1XX_LEXER_H
#define C1XX_LEXER_H
#include "frontend-init.h"
#include <recon.h>

/* The actual recorded-input allocation is64 bytes. Only its written header,
 * context buffers and active bit are interpreted; no tail/default claim. */
struct CompilerRecordedLexer : CompilerTokenStream {
    void *initial_object;
    unsigned initial_value;
    void *current_object;
    unsigned current_value;
    CompilerContextBuffer buffer, read_buffer, write_buffer;
    unsigned arena_kind;
    unsigned char active : 1;
    unsigned char marker_emitted : 1;
    RECON_FORCEINLINE CompilerRecordedLexer(void *object, unsigned value, unsigned arena) {
        initial_object = current_object = object;
        initial_value = current_value = value;
        buffer.Initialize(arena);
        read_buffer = write_buffer = buffer;
        arena_kind = arena;
        active = 0;
    }
    /* Outlined constructor-shaped entry1dbdb, actual thiscall/RET8. */
    CompilerRecordedLexer *RECON_THISCALL Initialize(unsigned, BYTE);
    CompilerRecordedLexer *RECON_THISCALL CopyFrom(const CompilerRecordedLexer *);
    RECON_FORCEINLINE CompilerRecordedLexer(const CompilerRecordedLexer *source) {
        initial_object = source->initial_object; initial_value = source->initial_value;
        current_object = source->current_object; current_value = source->current_value;
        buffer = source->buffer; read_buffer = source->read_buffer;
        write_buffer = source->write_buffer; arena_kind = source->arena_kind;
        active = source->active; marker_emitted = source->marker_emitted;
    }

    unsigned RECON_THISCALL Read(void);
    unsigned RECON_THISCALL Peek(void);
    void RECON_THISCALL Unread(unsigned);
    void RECON_THISCALL Trace(unsigned);
    int RECON_THISCALL Valid(void);
    void RECON_THISCALL Reset(void);
    void *RECON_THISCALL Destroy(unsigned);
};
typedef char CompilerRecordedLexer_actual_allocation[sizeof(CompilerRecordedLexer) == 64 ? 1 : -1];


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
    PRAGMA_IMPLEMENTATION_KEY = 52,
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
    PRAGMA_STOP_MAP_REGION = 51,
    PRAGMA_START_MAP_REGION = 50,
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

#endif
