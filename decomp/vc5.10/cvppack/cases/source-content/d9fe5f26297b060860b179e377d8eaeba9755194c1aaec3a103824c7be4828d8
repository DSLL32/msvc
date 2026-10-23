/* Call the encountered imported static factory [00410290,004102a6). */
#include "widen.h"
#include <recon.h>

extern "C" int create_widen(WidenTi **object)
{
    return WidenTi::fCreate(*object, 256, 0);
}

/* Call the genuine thiscall slot0x20 [004102e0,004102fb). */
extern "C" int query_widen(WidenTi *object, WidenOutput *output,
                           const void *input, unsigned size)
{
    return object->Query(output, input, size, 0);
}
