#ifndef SEGMENTS_H
#define SEGMENTS_H
#include "c2.h"
#include "symbols.h"
#include <stddef.h>
#include <recon.h>
/* OMF group descriptor: original name index at0 and group number at4. */
typedef struct OmfGroup {
    unsigned name_index, number;
} OmfGroup;
typedef char OmfGroupSizeCheck[sizeof(OmfGroup) == 8 &&
    offsetof(OmfGroup, name_index) == 0 && offsetof(OmfGroup, number) == 4 ? 1 : -1];
extern OmfGroup ungrouped_segment; /* original84fc8..84fd0 */
extern OmfGroup *default_segment_group; /* original84fd4 */
typedef struct Segment {
    SymbolHeader symbol;
    unsigned field20;
    struct Segment *segment_next;
    char *class_name;
    unsigned field2c, alignment;
    unsigned char format_flags, combine, section_number, field37;
    unsigned object_section;
    union { struct Segment *parent; unsigned omf_header_end; };
    union { unsigned order; SymbolHeader *owner; } identity;
    union {
        struct { unsigned char selection, field45; unsigned short field46; } coff;
        struct { unsigned char selection, flags; unsigned short reserved; } omf_comdat;
        unsigned omf_class_index;
    } object_attributes;
} Segment;
typedef char SegmentSizeCheck[sizeof(Segment) == 72 ? 1 : -1];
extern Segment *code_segment, *data_segment, *const_segment, *bss_segment;
extern Segment *debug_symbols_segment, *debug_types_segment, *tls_segment;
Segment *__fastcall create_segment(char *name, char *class_name, unsigned identifier);
void __fastcall append_segment(Segment *segment);
Segment *__fastcall segment_defaults(Segment *segment);
#endif
