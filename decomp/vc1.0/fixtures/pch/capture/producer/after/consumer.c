#include "pch.h"
int main(void)
{
    PchItem item = { 6, PCH_LARGE, "PCH round trip" };
    PchTransform transform = pch_transform;
    if (transform(&item) != 50) return 1;
    if (PCH_VALUE(PCH_SMALL) != 17) return 2;
    if (item.name[4] != 'r') return 3;
    return 0;
}
