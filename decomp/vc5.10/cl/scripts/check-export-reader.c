/* Native parser controls only; never executes target or PE instructions. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef unsigned char BYTE;
#include "export-reader.h"

typedef struct ImageView {
    const BYTE *data;
    DWORD bytes,base,calls,fail_call;
} ImageView;
static int image_read(void *context,DWORD address,void *out,DWORD bytes)
{
    ImageView *view=(ImageView *)context;
    ++view->calls;
    if (view->calls==view->fail_call || address<view->base ||
        address-view->base>view->bytes || bytes>view->bytes-(address-view->base)) return 0;
    memcpy(out,view->data+(address-view->base),bytes);
    return 1;
}
/* Same old remote algorithm, using fixed PE32 field offsets instead of SDK
 * structs so this test runs on a native host of either pointer width. */
static int old_lookup(ImageView *view,const char *wanted,DWORD *result)
{
    BYTE dos[64],nt[248],exports[40],word[4];
    DWORD directory,i,j,name_rva,ordinal,rva;
    char name[256];
    if (!image_read(view,view->base,dos,64) ||
        !image_read(view,view->base+export_word(dos+60),nt,248)) return -5;
    directory=export_word(nt+24+96);
    if (!image_read(view,view->base+directory,exports,40)) return -5;
    for (i=0;i<export_word(exports+24);++i) {
        if (!image_read(view,view->base+export_word(exports+32)+4*i,word,4)) return -5;
        name_rva=export_word(word);
        for (j=0;j+1<sizeof(name);++j) {
            if (!image_read(view,view->base+name_rva+j,name+j,1)) return -5;
            if (!name[j]) break;
        }
        name[sizeof(name)-1]=0;
        if (!strcmp(name,wanted)) {
            if (!image_read(view,view->base+export_word(exports+36)+2*i,word,2)) return -5;
            ordinal=export_short(word);
            if (!image_read(view,view->base+export_word(exports+28)+4*ordinal,word,4)) return -5;
            rva=export_word(word);*result=view->base+rva;return 1;
        }
    }
    *result=0;return 0;
}
int parser_probe(const BYTE *image,DWORD bytes,DWORD base,const char *wanted,
                 DWORD *address,DWORD *calls,DWORD fail_call,int old)
{
    ImageView view;int result;
    view.data=image;view.bytes=bytes;view.base=base;view.calls=0;view.fail_call=fail_call;
    *address=0;
    result=old?old_lookup(&view,wanted,address):export_lookup(image_read,&view,base,wanted,address);
    *calls=view.calls;return result;
}
