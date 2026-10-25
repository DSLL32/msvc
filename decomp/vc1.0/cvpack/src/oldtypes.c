/* Typed dispatch from encountered C6 leaf records to their converters. */
#include "types.h"
#include "image.h"
#include <recon.h>

typedef struct OldTypeConverter {
    unsigned char leaf;
    void (__cdecl *convert)(TypeRecord *);
} OldTypeConverter;
typedef char OldTypeConverterSize[(sizeof(OldTypeConverter)==8)?1:-1];

OldTypeConverter old_type_converters[13] = {
    { 0x7a, convert_old_pointer },
    { 0x91, convert_old_based_pointer },
    { 0x79, convert_old_structure },
    { 0x78, convert_old_array },
    { 0x75, convert_old_procedure },
    { 0x80, convert_old_leaf_80 },
    { 0x5c, convert_old_leaf_5c },
    { 0x72, convert_old_leaf_72 },
    { 0x8d, convert_old_leaf_8d },
    { 0x8e, convert_old_leaf_8e },
    { 0x8c, convert_old_barray },
    { 0xa6, convert_old_modifier },
    { 0xa7, convert_old_leaf_a7 },
};

void convert_old_type_record(unsigned short relative)
{
    TypeRecord *entry;
    unsigned short unused;
    unsigned char leaf;
    unsigned i;
    if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
        entry = empty_type;
    else {
        entry = &local_type_records[relative];
        if (entry->flags & 0x1000)
            entry = get_type_record((unsigned short)(entry->forward_index-0x1000), &unused);
    }
    leaf = entry->data[3];
    for (i = 0; i < 13; ++i)
        if (old_type_converters[i].leaf == leaf) break;
    if (i != 13) old_type_converters[i].convert(entry);
    else {
        warning(5, module_name(current_module), NULL);
        convert_old_leaf_80(entry);
    }
}
