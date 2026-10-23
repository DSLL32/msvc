/* Aggregate initialization and its nested element contexts. */
#include "braced-initializers.h"
#include "lexer.h"
#include "cpp-context.h"
#include <recon.h>

int initializer_excess_reported; /* original 0x915a8 */
int braced_initializer_depth = -1; /* original 0xa0a80 */
CType *initializer_selected_type; /* original 0x91584 */
BracedInitializer *current_braced_initializer; /* original 0x915ac */
BracedInitializer braced_initializers[32]; /* original 0x915b0..0x91830 */

/* Original 0x46590..0x466fc. Establish the root element context,
   select static/runtime handling and initialize browser ownership. */
void __stdcall begin_braced_initializer(CppSymbol *symbol)
{
    int force_static;
    CppSymbol *tag;
    unsigned storage;
    if (!symbol) {
        token_allocation_flags &= ~0x00100000UL;
        return;
    }
    initializer_excess_reported = 0;
    braced_initializer_depth = -1;
    initializer_selected_type = 0;
    force_static = (symbol->attributes & 0x500) == 0x500;
    initializer_storage_symbol = symbol;
    initializer_value_error_reported = 0;
    initializer_constructor_offset = 0;
    initializer_constructor_count = 0;
    initializer_offset = 0;
    push_braced_initializer(symbol);
    current_braced_initializer->flags = 1;
    if (symbol_construction_class(current_braced_initializer->symbol))
        begin_runtime_initializer(current_braced_initializer->symbol);
    if (is_unqualified_class_type(current_braced_initializer->symbol->type)) {
        tag = cpp_context_stack->items[current_braced_initializer->symbol->type->field08];
        if (!(tag->detail.class_info->flags2 & 1)) {
            report_syntax_error(552, current_braced_initializer->symbol->name);
            current_braced_initializer->flags |= 0x20;
        }
    }
    if (is_scalar_initializer_symbol(current_braced_initializer->symbol) || force_static)
        current_braced_initializer->flags |= 4;
    storage = current_braced_initializer->symbol->storage[0] & 7;
    if (!declaration_mode || storage < 4 || storage > 6) {
        pending_initializer_symbol = symbol;
        if (browser_filename) {
            browser_begin_tag();
            browser_enter_tag(pending_initializer_symbol);
        }
    }
    token_allocation_flags &= ~0x00100000UL;
}
