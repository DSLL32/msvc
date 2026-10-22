/* C++ class/enum tag definitions and their surrounding symbol context. */
#include "lexer.h"

CppSymbol *current_enum_symbol; /* original 0x948f8 */

/* Original 0x2d570..0x2d8c1. A definition owns a temporary completed type;
   the tag's interned type is finalized by the corresponding end-tag routine. */
CppSymbol *__stdcall define_tag_symbol(char *name, unsigned short kind, unsigned short attributes)
{
    CppSymbol *symbol = 0, *first;
    TypeDescription *description;
    CType *type;
    ClassContext *context;
    unsigned char scope_kind;
    if (token_allocation_flags & 0x40) flush_current_function(1);
    if (name) {
        scope_kind = (unsigned char)current_scope->flags;
        if ((scope_kind < 250 || !(current_scope->buckets[0]->type->flags & 0x200)) &&
            current_scope->buckets) {
            if (scope_kind < 250)
                first = current_scope->buckets[(unsigned)name & current_scope->bucket_mask];
            else
                first = current_scope->buckets[0]->auxiliary.scope_members;
        } else first = 0;
        if (first) symbol = find_tag_in_chain(name, first);
        if (symbol && (symbol->flags & 4)) {
            report_syntax_error(11, name, type_kind_name(symbol->type->flags));
            symbol->declaration_flags |= 2;
            return symbol;
        }
    }
    if (!symbol) {
        symbol = allocate_tag_symbol(name, kind, finalize_parameter_declaration, 1);
        if (!symbol) return 0;
        if (current_class_context >= 0)
            defer_nested_tag(class_contexts[current_class_context].symbol, symbol);
    }
    symbol = match_tag_symbol(symbol, kind);
    if (symbol->declaration_flags & 2) return symbol;
    description = copy_type_description(symbol->type);
    if (browser_filename && (!declaration_mode || options.option_FR) && !(symbol->flags & 0x80)) {
        browser_begin_tag();
        if (!browser_name_reference(0, symbol->name)) browser_declaration(symbol);
        browser_enter_tag(symbol);
    }
    description->flags |= 0x10;
    description->attributes = attributes;
    description->field08 = symbol->detail.class_info->type_index;
    type = intern_type(description);
    --type_description_depth;
    if (((token_allocation_flags & 0x80000) && !(token_allocation_flags & 0x40)) ||
        current_class_context >= 0)
        symbol->detail.class_info->flags |= 2;
    else
        symbol->detail.class_info->flags &= ~2;
    if (kind == 0x200) {
        if (current_class_context < 0)
            symbol->flags |= 3;
        else symbol->flags = (symbol->flags & ~3) | (class_contexts[current_class_context].flags & 3);
        enumerator_value = (unsigned)-1;
        symbol->detail.class_info->field00 = 4;
        symbol->flags |= 4;
        enumerator_context = type;
        current_enum_symbol = symbol;
        return symbol;
    }
    if (current_class_context >= 0)
        symbol->flags = (symbol->flags & ~3) | (class_contexts[current_class_context].flags & 3);
    if (++current_class_context == 16) fatal_error(46, type_kind_name(type->flags));
    context = &class_contexts[current_class_context];
    if ((kind & 0x408) == 0x408)
        context->flags = (context->flags & ~2) | 1;
    else context->flags |= 3;
    context->field10 = 0;
    symbol->detail.class_info->flags &= ~0x10;
    symbol->detail.class_info->flags &= ~0x40;
    enter_class_scope(symbol, -1);
    context = &class_contexts[current_class_context];
    context->flags &= ~0x0c;
    symbol->detail.class_info->flags2 |= 1;
    if ((kind & 0x408) == 0x408 || (kind & 0x402) == 0x402)
        context->flags |= 0x10;
    else context->flags &= ~0x10;
    context->symbol = symbol;
    context->field04 = 0;
    context->field14 = 0;
    context->field1c = 0;
    context->field18 = 0;
    context->type = type;
    context->alignment = 1;
    return symbol;
}
