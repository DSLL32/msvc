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
void *section_context;

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
        section->symbol.context = section_context;
        section->attribute = 1;
        section->section_kind = 0x13;
        debug_symbols_section = section;
        section->order = 5;

        section = create_section("$$TYPES", "DEBTYP", 8);
        section->alignment = 1;
        section->combine = 0;
        section->symbol.context = section_context;
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
