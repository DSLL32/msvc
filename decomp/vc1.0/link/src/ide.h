#ifndef LINK_IDE_H
#define LINK_IDE_H
#include "link.h"
#include <recon.h>
/* Bundled coff/ideapi.c caller-supplied callback interface. Slot+8 is never
 * consumed by the recovered routines; retain an opaque DWORD there. */
typedef struct IdeApi {
    unsigned long (__cdecl *get_channel)(unsigned long kind);
    int (__cdecl *write)(unsigned long channel,const char *bytes,unsigned long count);
    unsigned long reserved;
    void (__cdecl *assertion)(const char *source,unsigned long line);
    char *buffer;
    unsigned long capacity;
} IdeApi;
typedef char ide_api_size[(sizeof(IdeApi)==24)?1:-1];
extern IdeApi *ide_api;
extern unsigned long ide_channel;
unsigned long initialize_ide_api(IdeApi *api);
int emit_ide_message(unsigned long tag,unsigned long value);
int read_ide_flags(char **output);
#endif
