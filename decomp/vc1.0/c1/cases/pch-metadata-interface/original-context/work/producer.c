#include "metadata.h"
const MetadataRecord metadata_seed = { 0, META_KIND, 1, 17, "metadata seed", 0.5 };
const volatile unsigned metadata_qualified = META_SCALE;
unsigned metadata_mutable = META_KIND;
int metadata_scale(const MetadataRecord *item)
{
    return (int)(item->value * META_SCALE) + item->kind;
}
