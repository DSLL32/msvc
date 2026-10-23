/* Logical-segment adapter for the COFF writer. */
#include "coff-segments.h"
#include "coff-writer.h"
/* Original [0006ca10,0006ca50). */
void __fastcall coff_select_segment(Segment *segment)
{
    unsigned short identifier;
    unsigned position;
    if (!segment->object_section) coff_declare_segment(segment);
    identifier = (unsigned short)segment->object_section;
    coff_select_section(identifier);
    position = coff_section_position(identifier);
    if (segment->symbol.field04 > position)
        coff_fill_bytes(0, segment->symbol.field04 - position);
}

#include <string.h>
#include "coff-output.h"
#include "backend.h"
#include <recon.h>
extern int optimize_speed, stack_alignment;
unsigned char __fastcall coff_comdat_selection(unsigned char selection);
void __fastcall register_coff_segment(Segment *segment);
/* Original [0006d010,0006d310), including its twelve-way .text switch table. */
unsigned short __fastcall coff_declare_segment(Segment *segment)
{
    Segment *parent;
    char *name, *short_name;
    unsigned flags, i;
    unsigned short associated;
    if (!segment->object_section) {
        parent = segment->word_size == 7 ? segment->parent : segment;
        if (segment->section_number == 1 &&
            !(segment->word_size == 7 && segment->object_attributes.coff.selection == 5)) {
            if (!strncmp(parent->symbol.name, ".rdata", 6) ||
                !strncmp(parent->symbol.name, ".const", 6))
                segment->section_number = 2;
            else if (!strncmp(parent->symbol.name, ".text", 5))
                segment->section_number = 0;
            else if (!strncmp(parent->symbol.name, ".drectve", 8)) {
                segment->section_number = 10;
                segment->alignment = directive_segment->alignment;
            }
        }
        switch ((signed char)segment->section_number) {
        case 0:
            name = parent->symbol.identifier == 5 ||
                (segment->word_size == 7 && segment->object_attributes.coff.selection == 5) ?
                ".text\0\0" : parent->symbol.name;
            flags = 0x60000020;
            segment->alignment = optimize_speed ? stack_alignment : 1;
            break;
        case 1:
            name = parent->symbol.identifier == 2 ||
                (segment->word_size == 7 && segment->object_attributes.coff.selection == 5) ?
                ".data\0\0" : parent->symbol.name;
            flags = 0xc0000040;
            break;
        case 2:
            name = parent->symbol.identifier == 3 ||
                (segment->word_size == 7 && segment->object_attributes.coff.selection == 5) ?
                ".rdata\0" : parent->symbol.name;
            flags = 0x40000040;
            break;
        case 3:
            name = parent->symbol.identifier == 4 ||
                (segment->word_size == 7 && segment->object_attributes.coff.selection == 5) ?
                ".bss\0\0\0" : parent->symbol.name;
            flags = 0xc0000080;
            break;
        case 4:
            name = parent->symbol.identifier == 7 ||
                (segment->word_size == 7 && segment->object_attributes.coff.selection == 5) ?
                ".debug$S" : parent->symbol.name;
            flags = 0x42000048;
            break;
        case 5:
            if (parent->symbol.identifier == 8 ||
                (segment->word_size == 7 && segment->object_attributes.coff.selection == 5))
                name = coff_precompiled_types ? ".debug$P" : ".debug$T";
            else name = parent->symbol.name;
            flags = 0x42000048;
            break;
        case 9:
            name = segment->symbol.name;
            flags = 0x42000048;
            break;
        case 10:
            name = segment->symbol.name;
            flags = 0x00000a00;
            break;
        case 11:
            name = parent->symbol.identifier == 9 ||
                (segment->word_size == 7 && segment->object_attributes.coff.selection == 5) ?
                ".tls$\0\0" : parent->symbol.name;
            flags = 0xc0000040;
            break;
        /* The original leaves name and flags indeterminate for unused kinds. */
        }
        short_name = arena_alloc(7, 9);
        for (i = 0; i != 8; ++i) short_name[i] = name[i];
        short_name[8] = 0;
        if (segment->word_size == 7) {
            associated = segment->object_attributes.coff.selection == 5 ?
                (unsigned short)segment->parent->object_section : 0;
            segment->object_section = coff_create_comdat_section(short_name,
                flags | 0x1000, coff_comdat_selection(segment->object_attributes.coff.selection), associated, 1);
        } else segment->object_section = coff_create_section(short_name, flags, 1);
        coff_open_contribution((unsigned short)segment->object_section, flags);
        register_coff_segment(segment);
    }
    return (unsigned short)segment->object_section;
}

CoffSegmentLink *coff_segment_index[16]; /* 00079104..00079144 */
/* Original [0006da30,0006da60). */
void __fastcall register_coff_segment(Segment *segment)
{
    CoffSegmentLink *link = arena_alloc(7, sizeof(CoffSegmentLink));
    CoffSegmentLink **bucket;
    link->segment = segment;
    bucket = &coff_segment_index[segment->symbol.identifier & 15];
    link->next = *bucket;
    *bucket = link;
}

/* Original [0006da60,0006da9f). Translate compiler COMDAT policy to COFF
   selection: internal1 means ANY, internal2 means NODUPLICATES. */
unsigned char __fastcall coff_comdat_selection(unsigned char selection)
{
    switch (selection) {
    case 1: return 2;
    case 2: return 1;
    case 3: return 3;
    case 4: return 4;
    case 5: return 5;
    }
    compiler_internal_error("@(#)coff.c:1.14", 0x4e9);
    return 0;
}
