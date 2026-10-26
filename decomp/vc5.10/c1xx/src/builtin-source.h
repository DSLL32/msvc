#ifndef C1XX_BUILTIN_SOURCE_H
#define C1XX_BUILTIN_SOURCE_H
#include <windows.h>
#include <recon.h>

/* Actual 0x800-byte linked blocks: only bytes appended before a read have
 * defined contents. Allocation does not initialize the data payload. */
struct CompilerSourceTextBlock {
    BYTE data[0x7f8];
    CompilerSourceTextBlock *next;
    unsigned used;
};
struct CompilerSourceTextBuffer {
    unsigned position;
    BYTE *cursor;
    CompilerSourceTextBlock *current, *first;
};
typedef char CompilerSourceTextBlock_size[sizeof(CompilerSourceTextBlock) == 0x800 ? 1 : -1];
typedef char CompilerSourceTextBuffer_size[sizeof(CompilerSourceTextBuffer) == 16 ? 1 : -1];
#endif
