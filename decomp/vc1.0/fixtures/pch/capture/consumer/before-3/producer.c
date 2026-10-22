#include "pch.h"
int __cdecl pch_transform(const PchItem *item)
{
    return PCH_VALUE(item->value) + item->kind;
}
