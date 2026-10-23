/* Module symbol requirements, original [00032a20,00032bb0). */
#include "backend-passes.h"
#include "segments.h"
#include "records.h"
#include <recon.h>
extern int coff_object, floating_frontend, codeview_debug;
unsigned char floating_point_used; /* 000805f8 */
SymbolHeader *__fastcall require_external_symbol(char *name);
void prepare_backend_symbols(void)
{
    char *name;
    const char *expected;
    if (current_symbol && !coff_object) {
        name = current_symbol->symbol.name;
        expected = "_main";
        while (*name && *name == *expected) { ++name; ++expected; }
        if (*name != *expected) {
            name = current_symbol->symbol.name;
            expected = "_WinMain";
            while (*name && *name == *expected) { ++name; ++expected; }
        }
        if (*name == *expected) require_external_symbol("__acrtused");
    }
    if (floating_point_used) require_external_symbol("__fltused");
    if (!floating_frontend) return;
    debug_types_segment = 0;
    debug_symbols_segment = 0;
    code_segment = (Segment *)lookup_global_symbol(5);
    data_segment = (Segment *)lookup_global_symbol(2);
    const_segment = (Segment *)lookup_global_symbol(3);
    bss_segment = (Segment *)lookup_global_symbol(4);
    if (codeview_debug) {
        debug_types_segment = (Segment *)lookup_global_symbol(8);
        debug_symbols_segment = (Segment *)lookup_global_symbol(7);
    }
    if (code_segment) code_segment->identity.order = 1;
    if (data_segment) data_segment->identity.order = 2;
    if (bss_segment) bss_segment->identity.order = 4;
    if (const_segment) const_segment->identity.order = 3;
    if (debug_symbols_segment) {
        debug_symbols_segment->identity.order = 5;
        debug_symbols_segment->alignment = 1;
        debug_symbols_segment->combine = 0;
        debug_symbols_segment->format_flags = 1;
    }
    if (debug_types_segment) {
        debug_types_segment->identity.order = 6;
        debug_types_segment->identity.order = 6;
        debug_types_segment->alignment = 1;
        debug_types_segment->combine = 0;
        debug_types_segment->format_flags = 1;
    }
}
