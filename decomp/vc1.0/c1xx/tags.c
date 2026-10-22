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
        symbol = allocate_tag_symbol(name, kind, insert_current_scope_symbol, 1);
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

#include "linkage.h"
int tag_declaration_seen; /* original 0x9187c */

/* Original 0x2d8d0..0x2db11. Assign a tag's identity/type, merge a visible
   declaration when appropriate, and pass new entries to the caller's inserter. */
CppSymbol *__stdcall allocate_tag_symbol(char *name, unsigned short kind,
                                        void (__stdcall *insert)(CppSymbol *), int definition)
{
    CppSymbol *symbol = (CppSymbol *)new_tag_record(0), *existing;
    TypeDescription *description;
    ArenaPool *pool;
    symbol->kind = 4;
    if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_symbol_rollover();
    symbol->index = next_symbol_index++;
    symbol->flags |= 0x10;
    symbol->declaration_flags = (symbol->declaration_flags & ~0x300) |
        ((linkage_stack[linkage_depth] & 3) << 8);
    symbol->detail.class_info->inheritance_model &= ~7;
    register_tag_context(symbol);
    pool = &compiler_arena.pools[2];
    if (pool->remaining < (int)sizeof(TypeDescription)) refill_arena(&compiler_arena, pool);
    description = (TypeDescription *)pool->next;
    pool->remaining -= sizeof(TypeDescription);
    pool->next += sizeof(TypeDescription);
    description->flags = kind;
    if (kind & 0x200) {
        description->flags |= 4;
        symbol->auxiliary.enumerators = new_hash_table(4,
            (short)((token_allocation_flags & 0x80000) != 0), 0, 0);
    }
    description->field08 = symbol->detail.class_info->type_index;
    symbol->type = intern_type(description);
    if (!name) {
        if (declaration_depth > 0) {
            if (definition) report_syntax_error(465);
            else compiler_warning(1, 116);
        }
        if (cpp_parser_flags & 0x40) report_syntax_error(463);
        symbol->name = name_unnamed;
        SYMBOL_FROM_NAME(name_unnamed)->flags |= 2;
        symbol->flags |= 0x80;
        insert(symbol);
    } else {
        symbol->name = name;
        SYMBOL_FROM_NAME(name)->flags |= 2;
        if (declaration_depth > 0 && definition) report_syntax_error(204, symbol->name);
        if (cpp_parser_flags & 0x400) {
            begin_compound_statement();
            cpp_parser_flags |= 0x800;
        }
        existing = lookup_current_scope_declaration(symbol);
        if (!existing) {
            if (definition && is_current_class_name(name)) {
                report_syntax_error(580, name);
                return 0;
            }
            if (cpp_parser_flags & 0x40) report_syntax_error(462, name);
            else insert(symbol);
            apply_inheritance_model(symbol);
        } else {
            apply_inheritance_model(existing);
            symbol = existing;
        }
    }
    if (browser_filename && (!declaration_mode || options.option_FR)) {
        browser_declaration(symbol);
        browser_name_reference(1, symbol->name);
    }
    tag_declaration_seen = 1;
    return symbol;
}

/* Original 0x2e500..0x2e5b0. Tags use the common 40-byte symbol record,
   followed by a separately allocated 60-byte class/enum descriptor. */
TagSymbol *__stdcall new_tag_record(int arena)
{
    TagSymbol *tag;
    ClassInfo *info;
    ArenaPool *pool;
    if (!arena) {
        tag = allocate_permanent(sizeof(TagSymbol));
        info = allocate_permanent(sizeof(ClassInfo));
    } else {
        pool = &compiler_arena.pools[arena == -1 ? 0 : arena];
        if (pool->remaining < (int)sizeof(TagSymbol)) refill_arena(&compiler_arena, pool);
        tag = (TagSymbol *)pool->next;
        pool->remaining -= sizeof(TagSymbol);
        pool->next += sizeof(TagSymbol);
        if (pool->remaining < (int)sizeof(ClassInfo)) refill_arena(&compiler_arena, pool);
        info = (ClassInfo *)pool->next;
        pool->remaining -= sizeof(ClassInfo);
        pool->next += sizeof(ClassInfo);
    }
    tag->detail.class_info = info;
    if (!(token_allocation_flags & 0x80000) && !pch_separate_debug)
        tag->flags |= 8;
    else tag->flags &= ~8;
    return tag;
}

/* Original 0x2e5b0..0x2e612. Index zero is the null context sentinel. */
void __stdcall register_tag_context(CppSymbol *tag)
{
    InlineVector *stack = cpp_context_stack;
    short index = stack ? stack->count : 0;
    tag->detail.class_info->type_index = index;
    if (!index || index == -1) fatal_error(65);
    if (stack->count == stack->capacity) stack = grow_inline_vector(stack);
    stack->items[stack->count++] = tag;
    cpp_context_stack = stack;
}

/* Original 0x330d0..0x330ff. */
int __stdcall is_current_class_name(char *name)
{
    if (current_class_context < 0) return 0;
    return class_contexts[current_class_context].symbol->name == name;
}

/* Original 0x49d90..0x49e08. The class keeps its first explicit inheritance
   model; later incompatible specifications name that previous model. */
void __stdcall apply_inheritance_model(CppSymbol *tag)
{
    static char *model_names[] = { "unknown", "single", "multiple", "virtual", "virtual", 0 };
    unsigned model;
    unsigned char prior;
    if (!(cpp_parser_flags & 0x00e00000UL)) return;
    model = (cpp_parser_flags >> 21) & 7;
    prior = tag->detail.class_info->inheritance_model;
    if (!(prior & 8)) {
        tag->detail.class_info->inheritance_model = (prior & ~7) | model;
        tag->detail.class_info->inheritance_model |= 8;
    } else if (model != (unsigned)(prior & 7))
        report_syntax_error(286, tag->name, model_names[prior & 7]);
    cpp_parser_flags &= ~0x00e00000UL;
}

/* Original 0x2dc30..0x2dd4d. A qualified wrapper resolves to its tag first.
   Class/struct keyword changes warn and return before consuming model flags. */
CppSymbol *__stdcall match_tag_symbol(CppSymbol *symbol, unsigned short kind)
{
    unsigned short prior;
    QualifiedDeclarator *qualified;
    if ((symbol->kind & 31) == 18) {
        qualified = (QualifiedDeclarator *)symbol;
        if (!qualified->resolution) return 0;
        symbol = (CppSymbol *)qualified->resolution->binding->target;
        if (!symbol) return 0;
    }
    if (!(symbol->flags & 0x10)) {
        symbol = find_tag_declaration(symbol);
        if (!symbol) return 0;
    }
    prior = symbol->type->flags;
    if (((prior ^ kind) & 0x2fcf) && (kind != 0x200 || !(prior & 0x200))) {
        if (((prior & 0x408) == 0x408 || (prior & 0x402) == 0x402) &&
            ((kind & 0x408) == 0x408 || (kind & 0x402) == 0x402)) {
            if ((prior & 0x408) == 0x408)
                compiler_warning(2, 99, symbol->name, token_spellings[TOK_STRUCT], token_spellings[TOK_CLASS]);
            else
                compiler_warning(2, 99, symbol->name, token_spellings[TOK_CLASS], token_spellings[TOK_STRUCT]);
            return symbol;
        }
        report_syntax_error(11, symbol->name, type_kind_name(prior));
        symbol->declaration_flags |= 2;
    }
    apply_inheritance_model(symbol);
    return symbol;
}
