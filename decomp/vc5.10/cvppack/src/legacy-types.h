#ifndef CV510_LEGACY_TYPES_H
#define CV510_LEGACY_TYPES_H
#include "types.h"
#include <recon.h>
typedef struct LegacyTypeConverter {
    unsigned char leaf;
    void (__cdecl *convert)(TypeRecord *);
} LegacyTypeConverter;
extern LegacyTypeConverter legacy_type_converters[13];
unsigned read_legacy_numeric(unsigned char **cursor, unsigned short *bytes);
void convert_legacy_argument_list(unsigned index, unsigned short count);
unsigned char *resize_legacy_type_payload(TypeRecord *record, unsigned short size);
unsigned short write_unsigned_numeric(unsigned char *output, unsigned value);
void convert_legacy_pointer(TypeRecord *record);
void convert_legacy_based_pointer(TypeRecord *record);
void convert_legacy_structure(TypeRecord *record);
unsigned short convert_legacy_field_list(unsigned short types, unsigned short names, unsigned short count);
void convert_legacy_array(TypeRecord *record);
void convert_legacy_procedure(TypeRecord *record);
void convert_legacy_nil(TypeRecord *record);
void convert_legacy_bitfield(TypeRecord *record);
void convert_legacy_label(TypeRecord *record);
void convert_legacy_scalar_array(TypeRecord *record);
void convert_legacy_untranslated(TypeRecord *record);
void convert_legacy_based_array(TypeRecord *record);
void convert_legacy_cobol0(TypeRecord *record);
void convert_legacy_cobol1(TypeRecord *record);
#endif
