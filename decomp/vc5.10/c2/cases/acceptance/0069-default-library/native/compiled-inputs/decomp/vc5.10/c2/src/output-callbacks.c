#include "output-callbacks.h"
#include <recon.h>

extern char *c2_output_name;
extern int c2_dos_messages, c2_codeview_types, c2_line_numbers;
extern int c2_default_library, c2_strict_ansi;
char *output_group_name;
int output_enabled = 1;
C2PendingCallback output_slot_497de4;
C2PendingCallback output_slot_499a00;
C2PendingCallback output_slot_497e10;
C2PendingCallback output_slot_497dbc;
C2PendingCallback output_slot_497de0;
C2PendingCallback output_slot_497dec;
C2PendingCallback output_slot_497db0;
C2PendingCallback output_slot_497dd4;
C2PendingCallback output_slot_497ce4;
C2PendingCallback output_slot_497ddc;
C2PendingCallback output_slot_497dcc;
C2PendingCallback output_slot_499750;
C2PendingCallback output_slot_491070;
C2PendingCallback output_slot_497db4;
C2PendingCallback output_slot_491078;
C2PendingCallback output_slot_497f60;
C2PendingCallback output_slot_497dfc;
C2PendingCallback output_slot_497f50;
C2PendingCallback output_slot_49971c;
C2PendingCallback output_slot_497cec;

void initialize_pass_functions(void)
{
    char *library;

    output_group_name = c2_dos_messages ? "FLAT" : "DGROUP";
    if (c2_output_name && c2_codeview_types) {
        output_slot_497de4.target = output_callback_44308f;
        output_slot_499a00.target = output_callback_4432a1;
        output_slot_497e10.target = output_callback_454ba8;
    } else {
        output_slot_497de4.target = output_callback_41eb89;
        output_slot_499a00.target = output_callback_41eb89;
        output_slot_497e10.target = output_callback_41eb89;
    }
    if (!c2_output_name) {
        output_slot_497dbc.target = output_callback_41eb89;
        output_slot_497de0.target = output_callback_41eb89;
        output_slot_497ce4.target = output_callback_41eb89;
        output_slot_497dec.target = output_callback_4849f2;
        output_slot_497db0.target = output_callback_41eb89;
        output_slot_497dd4.target = output_callback_41eb89;
        output_slot_499750.target = output_callback_41eb89;
        output_slot_491070.target = output_callback_41eb89;
        output_slot_497db4.target = output_callback_41eb89;
        output_slot_491078.target = output_callback_41eb89;
        output_slot_497f60.target = output_callback_41eb89;
        output_slot_497ddc.target = output_callback_41eb89;
        output_slot_497dcc.target = output_callback_41eb89;
        output_slot_497dfc.target = output_callback_41eb89;
        output_slot_497f50.target = output_callback_4849f5;
        output_slot_49971c.target = output_callback_41eb89;
        output_slot_497cec.target = output_callback_4849fa;
        return;
    }
    if (output_enabled) {
        output_slot_497dbc.target = output_callback_4346cf;
        output_slot_497de0.target = output_callback_435633;
        output_slot_497dec.target = output_callback_434139;
        output_slot_497db0.target = output_callback_41f3f2;
        output_slot_497dd4.target = output_callback_41f3f2;
        output_slot_497ce4.target = output_callback_41ec20;
        if (c2_codeview_types || c2_line_numbers) {
            output_slot_497ddc.target = output_callback_442d21;
            output_slot_497dcc.target = output_callback_442b9f;
        } else {
            output_slot_497ddc.target = output_callback_41eb89;
            output_slot_497dcc.target = output_callback_41eb89;
        }
        output_slot_499750.target = output_callback_4850f5;
        output_slot_491070.target = output_callback_43696a;
        output_slot_497db4.target = output_callback_4357b8;
        output_slot_491078.target = output_callback_435af6;
        output_slot_497f60.target = output_callback_442d33;
        output_slot_497dfc.target = output_callback_4541f3;
        output_slot_497f50.target = output_callback_44a79d;
        output_slot_49971c.target = output_callback_453e02;
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
