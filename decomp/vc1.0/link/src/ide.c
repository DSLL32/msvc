/* Bundled IDE callback protocol, currently reached through external tests. */
#include "ide.h"
#include <recon.h>
#pragma intrinsic(strlen)
IdeApi *ide_api;
unsigned long ide_channel;

/* Original [00035700,00035716). The caller owns the interface and buffer.
 * Return the defined getter result as well as retaining its channel token. */
unsigned long initialize_ide_api(IdeApi *api)
{
    unsigned long channel;
    ide_api=api;
    channel=api->get_channel(2);
    ide_channel=channel;
    return channel;
}
/* Original [000355a0,000356fd), including3 bytes of internal alignment.
 * The tag determines whether the second DWORD is text or an integer. */
int emit_ide_message(unsigned long tag,unsigned long value)
{
    int length=0;
    if (!ide_api->buffer)
        ide_api->assertion("C:\\cudabld\\1\\build\\utils32\\coff\\ideapi.c",0x52);
    switch ((int)tag) {
    case 0:
        length=sprintf(ide_api->buffer,"%s%d","@I",tag);
        break;
    case 1: case 2: case 3: case 4:
        length=sprintf(ide_api->buffer,"%s%d%s","@I",tag,(const char *)value);
        break;
    case 5: case 6:
        length=sprintf(ide_api->buffer,"%s%d%lu","@I",tag,value);
        break;
    default:
        ide_api->assertion("C:\\cudabld\\1\\build\\utils32\\coff\\ideapi.c",0x78);
        break;
    }
    if (length!=0xffff && length) {
        length=strlen(ide_api->buffer);
        if (!length)
            ide_api->assertion("C:\\cudabld\\1\\build\\utils32\\coff\\ideapi.c",0x80);
        if (ide_api->capacity<(unsigned long)length)
            ide_api->assertion("C:\\cudabld\\1\\build\\utils32\\coff\\ideapi.c",0x81);
        if (strncmp(ide_api->buffer,"@I",2))
            ide_api->assertion("C:\\cudabld\\1\\build\\utils32\\coff\\ideapi.c",0x82);
        length=ide_api->write(ide_channel,ide_api->buffer,length);
    }
    return length;
}
/* Original [00035720,0003575e). The output aliases CRT environment storage;
 * no copy is allocated and an absent variable returns zero. */
int read_ide_flags(char **output)
{
    char *flags;
    if (!output)
        ide_api->assertion("C:\\cudabld\\1\\build\\utils32\\coff\\ideapi.c",0xb9);
    flags=getenv("_MSC_IDE_FLAGS");
    *output=flags;
    return flags!=NULL;
}
