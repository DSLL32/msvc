/* The compiler supplies these C++ runtime declarations as an initial source
 * stream. This is the complete81-line, NULL-terminated table consumed by
 * initialize_source_preprocessing; it includes no neighboring image data. */
#include <stdarg.h>
#include <string.h>
#include "frontend.h"
#include "builtin-source.h"
#include <recon.h>
#pragma intrinsic(strlen, memcpy)

/* CD564 is the existing parser cursor, never a second direct-output cell. */
#define source_text_write_cursor frontend_data.token_text_cursor

static const char *builtin_source_lines[] = {
    "#pragma pack(push, ehdata, 4)",
    "typedef struct _PMD",
    "{",
    "int mdisp;",
    "int pdisp;",
    "int vdisp;",
    "} _PMD;",
    "typedef void (*_PMFN)(void);",
    "#pragma warning(disable:4200)",
    "typedef struct _TypeDescriptor",
    "{",
    "void * pVFTable;",
    "void * spare;",
    "char name[];",
    "} _TypeDescriptor;",
    "#pragma warning(default:4200)",
    "typedef const struct _s__CatchableType {",
    "unsigned int properties;",
    "_TypeDescriptor *pType;",
    "_PMD thisDisplacement;",
    "int sizeOrOffset;",
    "_PMFN copyFunction;",
    "} _CatchableType;",
    "#pragma warning(disable:4200)",
    "typedef const struct _s__CatchableTypeArray {",
    "int nCatchableTypes;",
    "_CatchableType *arrayOfCatchableTypes[];",
    "} _CatchableTypeArray;",
    "#pragma warning(default:4200)",
    "typedef const struct _s__ThrowInfo {",
    "unsigned int attributes;",
    "_PMFN pmfnUnwind;",
    "int (__cdecl*pForwardCompat)(...);",
    "_CatchableTypeArray *pCatchableTypeArray;",
    "} _ThrowInfo;",
    "extern \"C\" void __stdcall _CxxThrowException(void* pExceptionObject, _ThrowInfo* pThrowInfo);",
    "#pragma pack(pop, ehdata)",
    "typedef const struct _s__RTTIBaseClassDescriptor {",
    "_TypeDescriptor *pTypeDescriptor;",
    "unsigned long numContainedBases;",
    "_PMD where;",
    "unsigned long attributes;",
    "} __RTTIBaseClassDescriptor;",
    "#pragma warning(disable:4200)",
    "typedef const struct _s__RTTIBaseClassArray {",
    "__RTTIBaseClassDescriptor *arrayOfBaseClassDescriptors[];",
    "} __RTTIBaseClassArray;",
    "#pragma warning(default:4200)",
    "typedef const struct _s__RTTIClassHierarchyDescriptor {",
    "unsigned long signature;",
    "unsigned long attributes;",
    "unsigned long numBaseClasses;",
    "__RTTIBaseClassArray *pBaseClassArray;",
    "} __RTTIClassHierarchyDescriptor;",
    "typedef const struct _s__RTTICompleteObjectLocator {",
    "unsigned long signature;",
    "unsigned long offset;",
    "unsigned long cdOffset;",
    "_TypeDescriptor *pTypeDescriptor;",
    "__RTTIClassHierarchyDescriptor *pClassDescriptor;",
    "} __RTTICompleteObjectLocator;",
    "typedef const class type_info &__RTtypeidReturnType;",
    "extern \"C\" void* __cdecl __RTDynamicCast (",
    "void*,",
    "long,",
    "void*,",
    "void*,",
    "int);",
    "extern \"C\" void* __cdecl __RTtypeid (void*);",
    "extern \"C\" void* __cdecl __RTCastToVoid (void*);",
    "extern \"C\" int __cdecl atexit(void (__cdecl *)(void));",
    "typedef unsigned int size_t;",
    "extern void * __cdecl operator new(size_t);",
    "extern void __cdecl operator delete(void *);",
    "struct __s_GUID {",
    "unsigned long Data1;",
    "unsigned short Data2;",
    "unsigned short Data3;",
    "unsigned char Data4[8];",
    "};",
    "typedef const struct _GUID &__COMuuidofReturnType;",
    NULL
};
extern "C" {
void __cdecl initialize_source_text_buffer(CompilerSourceTextBuffer **);
void __cdecl append_source_text(CompilerSourceTextBuffer *, const char *, ...);
void __cdecl activate_builtin_source(unsigned);
}
extern "C" void __cdecl initialize_source_preprocessing(void)
{
    const char **line;
    initialize_source_text_buffer(&frontend_bss.source_control.buffer);
    for (line = builtin_source_lines; *line; ++line)
        append_source_text(frontend_bss.source_control.buffer, "%s\n", *line);
    activate_builtin_source(0);
    frontend_data.compiler_mode_flags |= 0x20000;
}
extern "C" void __cdecl reset_source_text_buffer(CompilerSourceTextBuffer *);
extern "C" void __cdecl initialize_source_text_buffer(CompilerSourceTextBuffer **slot)
{
    CompilerSourceTextBuffer *buffer = *slot;
    if (buffer) buffer->first->used = 0;
    else {
        buffer = (CompilerSourceTextBuffer *)checked_malloc(16);
        buffer->first = (CompilerSourceTextBlock *)checked_malloc(0x800);
        buffer->first->next = NULL;
        buffer->first->used = 0;
        *slot = buffer;
    }
    reset_source_text_buffer(buffer);
}
extern "C" void __cdecl reset_source_text_buffer(CompilerSourceTextBuffer *buffer)
{
    buffer->position = 0;
    buffer->current = buffer->first;
    buffer->cursor = buffer->first->data;
}
extern "C" int __cdecl format_source_text(char *, const char *, int, va_list);
extern "C" void __cdecl write_source_text(const void *, unsigned, unsigned, CompilerSourceTextBuffer *);
extern "C" void __cdecl append_source_text(CompilerSourceTextBuffer *buffer, const char *format, ...)
{
    char text[1024];
    va_list arguments;
    va_start(arguments, format);
    format_source_text(text, format, sizeof(text), arguments);
    va_end(arguments);
    write_source_text(text, strlen(text), 1, buffer);
}
extern "C" void __cdecl write_source_text(const void *input, unsigned size, unsigned count,
    CompilerSourceTextBuffer *buffer)
{
    const BYTE *source = (const BYTE *)input;
    unsigned bytes = size * count;
    unsigned available;
    CompilerSourceTextBlock *block;
    if (!buffer) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("source text: direct-output mode pending");
#endif
        if (bytes) memcpy(source_text_write_cursor, source, bytes);
        source_text_write_cursor += bytes;
        return;
    }
    block = buffer->current;
    available = 0x7f8 - buffer->position;
    if (available >= bytes) {
        if (bytes) memcpy(buffer->cursor, source, bytes);
        buffer->cursor += bytes;
        buffer->position += bytes;
        return;
    }
    if (available) memcpy(buffer->cursor, source, available);
    bytes -= available;
    source += available;
    while (bytes) {
        if (!block->next) {
            CompilerSourceTextBlock *next = (CompilerSourceTextBlock *)checked_malloc(0x800);
            next->next = NULL;
            block->next = next;
        }
        block = block->next;
        /* Zero means a full, nonterminal block to the reader. No unwritten
         * payload byte is zeroed or treated as initialized here. */
        block->used = 0;
        if (bytes > 0x7f8) {
            memcpy(block->data, source, 0x7f8);
            source += 0x7f8;
            bytes -= 0x7f8;
        } else {
            memcpy(block->data, source, bytes);
            buffer->position = bytes;
            buffer->current = block;
            buffer->cursor = block->data + bytes;
            bytes = 0;
        }
    }
}
extern "C" void __cdecl finish_source_text_buffer(CompilerSourceTextBuffer *);
extern "C" int __cdecl open_source_file(const char *, unsigned, int, int, int);
extern "C" void __cdecl activate_builtin_source(unsigned flags)
{
    char name[512], *cursor = name;
    const char *text;
    unsigned hash = 0;
#ifdef RECON_NATIVE_TEST
    if (strlen(frontend_data.options.il_prefix) > 509)
        recon_unsupported("builtin source: prefix exceeds actual512-byte stack name");
#endif
    frontend_data.compiler_flags |= 0x200;
    finish_source_text_buffer(frontend_bss.source_control.buffer);
    for (text = frontend_data.options.il_prefix; (*cursor = *text++) != 0; ++cursor)
        hash = (hash >> 4) + hash * 4 + (BYTE)*cursor;
    for (text = "if"; (*cursor = *text++) != 0; ++cursor)
        hash = (hash >> 4) + hash * 4 + (BYTE)*cursor;
    open_source_file(name, hash, 2, flags, 0);
    frontend_data.token_text_cursor = compiler_token_text;
}
extern "C" void __cdecl finish_source_text_buffer(CompilerSourceTextBuffer *buffer)
{
    buffer->current->used = buffer->position;
}

/* Shared by builtin and PCH text input. A nonterminal block has used==0;
 * finalization marks the last block with its actual written byte count. */
extern "C" int __cdecl read_pch_input(CompilerSourceTextBuffer *buffer,
    BYTE *destination, unsigned requested)
{
    unsigned total = 0, count, i;
    BYTE *source;
    CompilerSourceTextBlock *block, *next;
    while ((block = buffer->current) != NULL) {
#ifdef RECON_NATIVE_TEST
        if (buffer->position > (block->used ? block->used : 0x7f8) ||
            (requested && !block->used && !block->next))
            recon_unsupported("source text: unfinalized or unwritten input span");
#endif
        source = buffer->cursor;
        if (block->used) {
            count = block->used - buffer->position;
            if (count >= requested) count = requested;
            buffer->position += count;
            buffer->cursor += count;
            next = NULL;
        } else {
            count = 0x7f8 - buffer->position;
            if (count >= requested) {
                count = requested;
                buffer->position += count;
                buffer->cursor += count;
                next = NULL;
            } else {
                buffer->position = 0;
                requested -= count;
                next = block->next;
                buffer->current = next;
                buffer->cursor = next ? next->data : NULL;
            }
        }
        total += count;
        /* Actual forward byte copies, including their overlap order. */
        if ((int)count > 0)
            for (i = 0; i < count; ++i) *destination++ = *source++;
        if (!next) break;
    }
    return total;
}
