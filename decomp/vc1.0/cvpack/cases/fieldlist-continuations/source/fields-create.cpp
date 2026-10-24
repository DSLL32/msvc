#include "huge-fields.h"
HugeFields shared_fields;
extern "C" __declspec(dllexport) int __stdcall create_fields(int seed)
{
    shared_fields.next=&shared_fields;
    shared_fields.field_0000=seed;
    shared_fields.field_2048=seed+1;
    shared_fields.field_4095=seed+2;
    return shared_fields.next->field_4095;
}
