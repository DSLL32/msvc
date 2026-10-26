#ifndef IDE_H
#define IDE_H
/* Original IDE progress transport: supplied callbacks plus an owned buffer.
   The Win32 built-in adapter selects a CRT descriptor, not an IDE process. */
typedef struct IdeContext {
    int (__fastcall *open_channel)(int channel);
    int (__fastcall *write_bytes)(int channel, char *text, unsigned bytes);
    int (__fastcall *report_error)(char *source, unsigned line);
    char *buffer;
    unsigned capacity;
} IdeContext;
int __fastcall ide_bind_context(IdeContext *context);
int __fastcall ide_open_channel(int channel);
int __fastcall ide_write_channel(int channel, char *text, unsigned bytes);
int __fastcall ide_error(char *source, unsigned line);
extern IdeContext *ide_context;
extern int ide_channel;
#include <recon.h>
#endif
