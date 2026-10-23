#include "c2.h"
#include <recon.h>

/* The default CONST name and its class share their original address. */
char const_section_class[] = "CONST";
char *code_section_name = "_TEXT";
char *data_section_name = "_DATA";
char *const_section_name = const_section_class;
char *bss_section_name = "_BSS";
char *tls_section_name = "_TLS";
C2Section *code_section, *data_section, *const_section, *bss_section;
C2Section *debug_symbols_section, *debug_types_section, *tls_section;
/* Address-only anchor for the pending object at original49e300. This char
 * supplies a distinct address in both builds; it is not the target object's
 * payload, layout or extent. C2SectionContext stays incomplete: no dereference,
 * arithmetic, serialization or unknown escape is permitted before modeling it.
 */
static char section_context_identity;
C2SectionContext *section_context = (C2SectionContext *)&section_context_identity;

void initialize_sections(void)
{
    C2Section *section;
    if (c2_il_eight_byte_target)
        return;

    code_section = create_section(code_section_name, "CODE", 5);
    initialize_section_defaults(code_section);
    code_section->alignment = stack_alignment;
    code_section->attribute = 1;
    code_section->section_kind = 0;
    code_section->order = 1;

    data_section = create_section(data_section_name, "DATA", 2);
    initialize_section_defaults(data_section);
    data_section->attribute = 0;
    data_section->section_kind = 1;
    data_section->order = 2;

    const_section = create_section(const_section_name, const_section_class, 3);
    initialize_section_defaults(const_section);
    const_section->attribute = 0;
    const_section->section_kind = 4;
    const_section->order = 3;

    bss_section = create_section(bss_section_name, "BSS", 4);
    initialize_section_defaults(bss_section);
    bss_section->attribute = 0;
    bss_section->section_kind = 3;
    bss_section->order = 4;

    if (c2_codeview_types) {
        section = create_section("$$SYMBOLS", "DEBSYM", 7);
        section->alignment = 1;
        section->combine = 0;
        section->symbol.association.context = section_context;
        section->attribute = 1;
        section->section_kind = 0x13;
        debug_symbols_section = section;
        section->order = 5;

        section = create_section("$$TYPES", "DEBTYP", 8);
        section->alignment = 1;
        section->combine = 0;
        section->symbol.association.context = section_context;
        section->attribute = 1;
        section->section_kind = 0x14;
        debug_types_section = section;
        section->order = 6;
    }

    tls_section = create_section(tls_section_name, "TLS", 9);
    initialize_section_defaults(tls_section);
    tls_section->attribute = 1;
    tls_section->section_kind = 0x0b;
    tls_section->order = 7;
}

C2Section *__fastcall create_section(char *name, char *class_name, unsigned serial)
{
    C2Section *section = c2_allocate_node(9, 0);

    section->symbol.serial = serial;
    section->symbol.name = name;
    section->class_name = class_name;
    insert_symbol(&section->symbol, 0);
    append_section(section);
    return section;
}

C2Section *section_head, *section_tail;

void __fastcall append_section(C2Section *section)
{
    if (section_head) {
        C2Section *previous = section_tail;
        section_tail = section;
        previous->next = section;
    } else {
        section_head = section;
        section_tail = section;
    }
}

unsigned default_section_alignment = 4;

C2Section *__fastcall initialize_section_defaults(C2Section *section)
{
    section->alignment = default_section_alignment;
    section->combine = 2;
    section->symbol.association.context = section_context;
    return section;
}
