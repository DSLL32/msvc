/* Logical object-file segments and their target defaults. */
#include "segments.h"
extern int floating_frontend, codeview_debug, stack_alignment;
int default_segment_alignment = 4; /* 000806cc; also used by argument/temporary allocation */
char *code_segment_name = "_TEXT", *data_segment_name = "_DATA";
char *bss_segment_name = "_BSS", *const_segment_name = "CONST";
char *tls_segment_name = "_TLS", *default_segment_group = "";
Segment *code_segment, *data_segment, *const_segment, *bss_segment;
Segment *debug_symbols_segment, *debug_types_segment, *tls_segment;
/* Original [0006a180,0006a310). */
void initialize_segments(void)
{
    if (floating_frontend) return;
    code_segment = create_segment(code_segment_name, "CODE", 5);
    segment_defaults(code_segment);
    code_segment->alignment = stack_alignment;
    code_segment->format_flags = 1;
    code_segment->section_number = 0;
    code_segment->identity.order = 1;
    data_segment = create_segment(data_segment_name, "DATA", 2);
    segment_defaults(data_segment);
    data_segment->format_flags = 0;
    data_segment->section_number = 1;
    data_segment->identity.order = 2;
    const_segment = create_segment(const_segment_name, "CONST", 3);
    segment_defaults(const_segment);
    const_segment->format_flags = 0;
    const_segment->section_number = 2;
    const_segment->identity.order = 3;
    bss_segment = create_segment(bss_segment_name, "BSS", 4);
    segment_defaults(bss_segment);
    bss_segment->format_flags = 0;
    bss_segment->section_number = 3;
    bss_segment->identity.order = 4;
    if (codeview_debug) {
        debug_symbols_segment = create_segment("$$SYMBOLS", "DEBSYM", 7);
        debug_symbols_segment->alignment = 1;
        debug_symbols_segment->combine = 0;
        debug_symbols_segment->symbol.field18.group = default_segment_group;
        debug_symbols_segment->format_flags = 1;
        debug_symbols_segment->section_number = 4;
        debug_symbols_segment->identity.order = 5;
        debug_types_segment = create_segment("$$TYPES", "DEBTYP", 8);
        debug_types_segment->alignment = 1;
        debug_types_segment->combine = 0;
        debug_types_segment->symbol.field18.group = default_segment_group;
        debug_types_segment->format_flags = 1;
        debug_types_segment->section_number = 5;
        debug_types_segment->identity.order = 6;
    }
    tls_segment = create_segment(tls_segment_name, "TLS", 9);
    segment_defaults(tls_segment);
    tls_segment->format_flags = 1;
    tls_segment->section_number = 11;
    tls_segment->identity.order = 7;
}

#include "backend.h"
#include <recon.h>
void __fastcall append_segment(Segment *segment);
/* Original [0006a120,0006a160). */
Segment *__fastcall create_segment(char *name, char *class_name, unsigned identifier)
{
    Segment *segment = allocate_descriptor(9, 0);
    segment->symbol.identifier = identifier;
    segment->symbol.name = name;
    segment->class_name = class_name;
    insert_symbol(&segment->symbol, 0);
    append_segment(segment);
    return segment;
}

Segment *segment_list;
/* Original [0006a100,0006a120). */
void __fastcall append_segment(Segment *segment)
{
    Segment **link = &segment_list;
    while (*link) link = &(*link)->segment_next;
    *link = segment;
}

/* Original [0006a160,0006a180). */
Segment *__fastcall segment_defaults(Segment *segment)
{
    segment->alignment = default_segment_alignment;
    segment->combine = 2;
    segment->symbol.field18.group = default_segment_group;
    return segment;
}
