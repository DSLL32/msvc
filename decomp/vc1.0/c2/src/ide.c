/* IDE progress messages and the target's descriptor-based default adapter. */
#include "backend.h"
#include "functions.h"
#include "ide.h"
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdarg.h>
#include <recon.h>
#pragma intrinsic(strlen)
IdeContext *ide_context; /* original7e920: active callback/buffer owner */
int ide_channel; /* original85244: result of the supplied open callback */
typedef char IdeContextSizeCheck[sizeof(IdeContext) == 20 ? 1 : -1];
/* Original [0006f370,0006f3b8). The three adjacent callback functions
   are separate graph vertices, outside this72-byte constructor. */
void initialize_debugging(void)
{
    IdeContext *context = arena_alloc(7, sizeof(IdeContext));
    context->open_channel = ide_open_channel;
    context->write_bytes = ide_write_channel;
    context->report_error = ide_error;
    context->buffer = arena_alloc(7, 64);
    context->capacity = 64;
    ide_bind_context(context);
}

/* Original [0006f570,0006f584). Publish the supplied callbacks before
   invoking open so callback-side observations see the active context. */
int __fastcall ide_bind_context(IdeContext *context)
{
    ide_context = context;
    ide_channel = context->open_channel(2);
    return ide_channel;
}

/* Original [0006f410,0006f413). The default adapter uses the supplied
   channel value itself as the CRT descriptor. */
int __fastcall ide_open_channel(int channel)
{
    return channel;
}

/* Original [0006f420,0006f561). Format one IDE record and pass its
   exact byte span to the active transport. Capacity is checked after the
   original unbounded formatter; do not silently change that old contract. */
int __cdecl ide_message(int code, ...)
{
    IdeContext *context = ide_context;
    va_list arguments;
    int result = 0;
    unsigned bytes;
    if (!context->buffer) {
        context->report_error("d:\\usr\\cuda\\src\\Common\\ideapi.c", 0x73);
        context = ide_context;
    }
    va_start(arguments, code);
    switch (code) {
    case 0:
        result = sprintf(context->buffer, "%s%d", "@I", code);
        break;
    case 1: case 2: case 3: case 4:
        result = sprintf(context->buffer, "%s%d%s", "@I", code,
                         va_arg(arguments, char *));
        break;
    case 5: case 6:
        result = sprintf(context->buffer, "%s%d%lu", "@I", code,
                         va_arg(arguments, unsigned long));
        break;
    default:
        context->report_error("d:\\usr\\cuda\\src\\Common\\ideapi.c", 0x97);
        break;
    }
    va_end(arguments);
    if (result == -1 || !result) return result;
    context = ide_context;
    bytes = strlen(context->buffer);
    if (!bytes) {
        context->report_error("d:\\usr\\cuda\\src\\Common\\ideapi.c", 0x9e);
        context = ide_context;
    }
    if (bytes > context->capacity) {
        context->report_error("d:\\usr\\cuda\\src\\Common\\ideapi.c", 0x9f);
        context = ide_context;
    }
    if (strncmp(context->buffer, "@I", 2))
        ide_context->report_error("d:\\usr\\cuda\\src\\Common\\ideapi.c", 0xa0);
    context = ide_context;
    return context->write_bytes(ide_channel, context->buffer, bytes);
}

/* Original [0006f3e0,0006f40d). Flush the actual descriptor's FILE
   buffer before bypassing stdio for the protocol's exact raw byte span. */
int __fastcall ide_write_channel(int channel, char *text, unsigned bytes)
{
    fflush(&_iob[channel]);
    return _write(channel, text, bytes);
}

/* Original6f3c0..6f3d3 (19 bytes), ignored ECX/EDX callback inputs, RET0.
   Report the adapter implementation site, not the caller-supplied strings.
   The zero return is nominal only: the original fatal child does not return. */
int __fastcall ide_error(char *source, unsigned line)
{
    (void)source; (void)line;
    compiler_internal_error("@(#)ide.c:1.2", 0x44);
    return 0;
}
