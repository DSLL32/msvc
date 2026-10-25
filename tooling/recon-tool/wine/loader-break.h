/* Original-only Wine backend qualification, not reconstructed target code. */
#ifndef RECON_ORIGINAL_LOADER_BREAK_H
#define RECON_ORIGINAL_LOADER_BREAK_H
#include <stddef.h>
#include <string.h>
static int original_loader_break(const char *module,const unsigned char *bytes,size_t size)
{
    static const unsigned char signature[18]={
        0x64,0xa3,0,0,0,0,0xcc,0x8b,0x45,0xa8,0x64,0xa3,0,0,0,0,0xc9,0xc3
    };
    static const char owner[]="ntdll.dll";
    size_t i;
    for(i=0;i<sizeof(owner);++i) {
        unsigned char c=(unsigned char)module[i];
        if(c>='A' && c<='Z')c+='a'-'A';
        if(c!=(unsigned char)owner[i])return 0;
    }
    return size==sizeof(signature) && !memcmp(bytes,signature,sizeof(signature));
}
#endif
