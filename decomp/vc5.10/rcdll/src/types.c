#include <string.h>
#include "directives.h"
#include "command.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

struct RcTypeEntry {
    struct RcTypeEntry *next;
    RcIntegerType type;
};
/* The interner indexes256 loader-zeroed bucket pointers with hash &255. */
struct RcTypeEntry *rc_type_buckets[256];

RcIntegerType *__stdcall rc_intern_type(RcIntegerType *type)
{
    unsigned hash=(type->key4+type->key8+type->flags)&255;
    struct RcTypeEntry *entry;
    for(entry=rc_type_buckets[hash];entry;entry=entry->next)
        if(rc_equal_type(type,&entry->type))return &entry->type;
    entry=rc_allocate(sizeof(*entry));
    if(!entry){
        strcpy(rc_error_text,rc_message(1002));
        rc_input_error(1002);
        return NULL;
    }
    entry->next=rc_type_buckets[hash];
    rc_type_buckets[hash]=entry;
    entry->type=*type;
    entry->type.other16=0;
    return &entry->type;
}

int __stdcall rc_equal_type(const RcIntegerType *left,const RcIntegerType *right)
{
    return left->flags==right->flags &&
           left->key8==right->key8 &&
           left->key4==right->key4;
}
