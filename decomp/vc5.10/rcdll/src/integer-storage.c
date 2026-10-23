#include "directives.h"
#include "command.h"
#include <string.h>
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

void *__stdcall rc_store_integer(unsigned token,const unsigned *value)
{
    RcIntegerToken *node;
    RcIntegerType *type;
    unsigned char code=(unsigned char)token;
    unsigned type_code;

    node=rc_allocate(sizeof(RcIntegerToken));
    if (!node) goto no_memory;
    node->flags=1;
    node->token=code;
    if (code==4 || code==5 || code==6) {
        type_code=code+2;
        node->value=*value;
    } else if (code>6 && code<11) {
        if (code==9 || code==10)
            type_code=((code!=9)+3)|0x10;
        else
            type_code=(code!=7)+3;
        node->value=*value;
        if (!node->value) node->flags=0x41;
    } else {
#ifdef RECON_NATIVE_TEST
        /* The fallback consumes raw caller bits not characterized by this case. */
        __asm int 3
#endif
        type_code=token;
    }
    type=rc_allocate(sizeof(RcIntegerType));
    if (!type) goto no_memory;
    type->flags=(unsigned short)(type_code|0x1000);
    node->type=rc_intern_type(type);
    return node;
no_memory:
    strcpy(rc_error_text,rc_message(1002));
    rc_input_error(1002);
    return NULL;
}
