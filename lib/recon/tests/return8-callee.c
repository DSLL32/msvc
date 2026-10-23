#include "return8-shape.h"
RECON_RETURN8_TYPE(ParserValue) __stdcall return_value(unsigned code)
{
    ParserValue value;
    unsigned i;
    for(i=0;i<8;++i)value.bytes[i]=0xa5;
    value.bytes[0]=(unsigned char)code;
    value.declaration.type=(CType *)(0x11223300U+code);
    RECON_RETURN8(value);
}
