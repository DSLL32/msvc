#ifndef RESOURCE4_H
#define RESOURCE4_H
typedef struct ResourceItem { unsigned short kind; short weight; long value; } ResourceItem;
static int resource_value(const ResourceItem *item, unsigned scale)
{
    int result = item->weight * (int)scale;
    if (item->kind & 1) result += (int)item->value;
    else result -= (int)item->value;
    return result;
}
#if RESOURCE_MISSING
#include "absent-resource.h"
#endif
#endif
