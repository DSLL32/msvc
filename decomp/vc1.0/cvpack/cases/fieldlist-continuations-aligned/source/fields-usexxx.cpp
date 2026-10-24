#include "huge-fields.h"
extern "C" __declspec(dllexport) int __stdcall use_fields(HugeFields *item)
{
    if (!item) item=&shared_fields;
    return item->field_0000+item->field_2048+item->field_4095;
}
