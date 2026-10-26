#include "output-callbacks.h"
#include <recon.h>

extern char *c2_output_name;
extern int c2_dos_messages, c2_codeview_types, c2_line_numbers;
extern int c2_default_library, c2_strict_ansi;
char *output_group_name;
int output_enabled = 1;
C2SymbolCallback output_slot_497de4;
C2PendingCallback output_slot_499a00;
C2PendingCallback output_slot_497e10;
C2SymbolCallback output_slot_497dbc;
C2SymbolCallback output_slot_497de0;
C2FunctionCodeCallback output_slot_497dec;
C2BytesCallback output_slot_497db0;
C2BytesCallback output_slot_497dd4;
C2SectionCallback output_slot_497ce4;
C2FilenameCallback output_slot_497ddc;
C2LineCallback output_slot_497dcc;
C2CodeRelocationCallback output_slot_499750;
C2CodeRelocationCallback output_slot_491070;
C2CodeRelocationCallback output_slot_497db4;
C2CodeRelocationCallback output_slot_491078;
C2PendingCallback output_slot_497f60;
C2PendingCallback output_slot_497dfc;
C2PendingCallback output_slot_497f50;
C2DirectiveCallback output_slot_49971c;
C2SymbolCallback output_slot_497cec;

void initialize_pass_functions(void)
{
    char *library;

    output_group_name = c2_dos_messages ? "FLAT" : "DGROUP";
    if (c2_output_name && c2_codeview_types) {
        output_slot_497de4.target = output_callback_44308f;
        output_slot_499a00.target = output_callback_4432a1;
        output_slot_497e10.target = output_callback_454ba8;
    } else {
        output_slot_497de4.target = output_noop_symbol;
        output_slot_499a00.target = c2_set_argv;
        output_slot_497e10.target = c2_set_argv;
    }
    if (!c2_output_name) {
        output_slot_497dbc.target = output_noop_symbol;
        output_slot_497de0.target = output_noop_symbol;
        output_slot_497ce4.target = output_noop_section;
        output_slot_497dec.target = output_callback_4849f2;
        output_slot_497db0.target = output_noop_bytes;
        output_slot_497dd4.target = output_noop_bytes;
        output_slot_499750.target = output_noop_relocation;
        output_slot_491070.target = output_noop_relocation;
        output_slot_497db4.target = output_noop_relocation;
        output_slot_491078.target = output_noop_relocation;
        output_slot_497f60.target = c2_set_argv;
        output_slot_497ddc.target = output_noop_filename;
        output_slot_497dcc.target = output_noop_line;
        output_slot_497dfc.target = c2_set_argv;
        output_slot_497f50.target = output_callback_4849f5;
        output_slot_49971c.target = (C2OutputDirectiveWriter) c2_set_argv;
        output_slot_497cec.target = output_callback_4849fa;
        return;
    }
    if (output_enabled) {
        output_slot_497dbc.target = output_callback_4346cf;
        output_slot_497de0.target = output_callback_435633;
        output_slot_497dec.target = output_callback_434139;
        output_slot_497db0.target = output_callback_41f3f2;
        output_slot_497dd4.target = output_callback_41f3f2;
        output_slot_497ce4.target = select_output_section;
        if (c2_codeview_types || c2_line_numbers) {
            output_slot_497ddc.target = output_callback_442d21;
            output_slot_497dcc.target = output_callback_442b9f;
        } else {
            output_slot_497ddc.target = output_noop_filename;
            output_slot_497dcc.target = output_noop_line;
        }
        output_slot_499750.target = output_callback_4850f5;
        output_slot_491070.target = output_callback_43696a;
        output_slot_497db4.target = output_callback_4357b8;
        output_slot_491078.target = output_callback_435af6;
        output_slot_497f60.target = output_callback_442d33;
        output_slot_497dfc.target = output_callback_4541f3;
        output_slot_497f50.target = output_callback_44a79d;
        output_slot_49971c.target = emit_output_directive;
        output_slot_497cec.target = output_callback_41fa04;
    }
    if (c2_default_library) {
        library = default_runtime_library();
        if (library)
            add_output_directive(4, library);
        if (!c2_strict_ansi && !c2_il_eight_byte_target)
            add_output_directive(4, "OLDNAMES");
    }
    if (output_enabled)
        initialize_object_output();
    if (c2_codeview_types)
        initialize_codeview_output();
}

extern int c2_runtime_md, c2_runtime_mt, c2_runtime_ml;
extern int c2_runtime_md_debug, c2_runtime_mt_debug, c2_runtime_ml_debug;
char *default_runtime_library(void)
{
    if (c2_runtime_md) return "MSVCRT";
    if (c2_runtime_mt) return "LIBCMT";
    if (c2_runtime_ml) return "LIBC";
    if (c2_runtime_md_debug) return "MSVCRTD";
    if (c2_runtime_mt_debug) return "LIBCMTD";
    return c2_runtime_ml_debug ? "LIBCD" : NULL;
}

C2OutputDirective *output_directives;

C2OutputDirective *__fastcall add_output_directive(int kind, char *value)
{
    C2OutputDirective **link = &output_directives;
    C2OutputDirective *node;

    while (*link)
        link = &(*link)->next;
    node = c2_pool_allocate(1, sizeof(*node));
    node->kind = kind;
    node->value = value;
    *link = node;
    return node;
}

void write_output_directives(void)
{
    C2OutputDirective *directive;
    for (directive = output_directives; directive; directive = directive->next)
        output_slot_49971c.target(directive);
}

int coff_directive_option;

void __fastcall emit_output_directive(C2OutputDirective *directive)
{
    switch (directive->kind) {
    case 1:
        emit_incremental_string("Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 11.00");
        break;
    case 3:
        write_coff_directive("-?comment:", directive->value, NULL, 1);
        break;
    case 4:
        write_coff_directive("-defaultlib:", directive->value, NULL, 0);
        break;
    case 6:
        emit_incremental_string(directive->value);
        break;
    case 7:
        coff_directive_option = 1;
        break;
    case 9:
        write_coff_directive("-nodefaultlib:", directive->value, NULL, 0);
        break;
    case 10:
        write_coff_directive("", directive->value, NULL, 0);
        break;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 1514);
        break;
    }
}
