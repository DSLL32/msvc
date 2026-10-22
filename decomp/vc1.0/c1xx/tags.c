/* C++ class/enum tag definitions and their surrounding symbol context. */
#include "lexer.h"

CppSymbol *current_enum_symbol; /* original 0x948f8 */

/* Original 0xa0b20: shared by inheritance-model diagnostics. */
static char *inheritance_model_names[] = {
    "unknown", "single", "multiple", "virtual", "virtual", 0
};

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
        symbol->detail.class_info->size = 4;
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
    context->size = 0;
    context->bit_field_bits = 0;
    context->bit_field_type = 0;
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
    unsigned model;
    unsigned char prior;
    if (!(cpp_parser_flags & 0x00e00000UL)) return;
    model = (cpp_parser_flags >> 21) & 7;
    prior = tag->detail.class_info->inheritance_model;
    if (!(prior & 8)) {
        tag->detail.class_info->inheritance_model = (prior & ~7) | model;
        tag->detail.class_info->inheritance_model |= 8;
    } else if (model != (unsigned)(prior & 7))
        report_syntax_error(286, tag->name, inheritance_model_names[prior & 7]);
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

/* Original 0x33420..0x33532. A tag scope is a wrapper frame with one tag
   pointer in place of ordinary name buckets. Mode -3 gives it permanent life. */
SymbolScope *__stdcall enter_class_scope(CppSymbol *tag, signed char mode)
{
    SymbolScope *scope;
    CppSymbol **cell;
    ArenaPool *pool;
    QualifiedDeclarator *qualified;
    if (token_allocation_flags & 0x20000000UL) return 0;
    if ((token_allocation_flags & 0x10000000UL) && !(token_allocation_flags & 0x80000000UL))
        return 0;
    if (!tag) return 0;
    if ((tag->kind & 31) == 18) {
        qualified = (QualifiedDeclarator *)tag;
        tag = (CppSymbol *)qualified->resolution->binding->target;
    }
    if (!symbol_has_tag_scope(tag)) return 0;
    if (!current_scope->buckets && (current_scope->flags & 0xff) < 250)
        initialize_scope_buckets(current_scope, 0);
    if (mode == -3) {
        scope = allocate_permanent(sizeof(SymbolScope));
        cell = allocate_permanent(sizeof(CppSymbol *));
    } else {
        pool = &compiler_arena.pools[1];
        if (pool->remaining < (int)sizeof(SymbolScope)) refill_arena(&compiler_arena, pool);
        scope = (SymbolScope *)pool->next;
        pool->remaining -= sizeof(SymbolScope);
        pool->next += sizeof(SymbolScope);
        if (pool->remaining < (int)sizeof(CppSymbol *)) refill_arena(&compiler_arena, pool);
        cell = (CppSymbol **)pool->next;
        pool->remaining -= sizeof(CppSymbol *);
        pool->next += sizeof(CppSymbol *);
    }
    scope->buckets = cell;
    scope->bucket_mask = 0;
    scope->flags = (scope->flags & 0xff00) | (unsigned char)mode;
    *cell = tag;
    push_symbol_scope(scope);
    record_scope_directory_entry(scope);
    return scope;
}

/* Original 0x33000..0x3303b. */
int __stdcall symbol_has_tag_scope(CppSymbol *symbol)
{
    if ((symbol->kind & 31) == 4) return 1;
    if (!symbol->type) return 0;
    return (symbol->flags & 0x10) && (symbol->type->flags & 0x200) ? 1 : 0;
}

/* Original 0x2ddf0..0x2de56. Re-read the active class after each child,
   retaining the original global-context calling pattern. */
void begin_class_body(void)
{
    class_contexts[current_class_context].symbol->detail.class_info->flags2 |= 0x10;
    prepare_base_layout(class_contexts[current_class_context].symbol);
    inherit_class_pointer_attributes(class_contexts[current_class_context].symbol);
    apply_default_vtordisp(class_contexts[current_class_context].symbol);
    check_inheritance_model(class_contexts[current_class_context].symbol);
}

/* Original 0x79300..0x793b5. The previous-base pointer spans both passes.
   Save each successor before placing a base, since placement can alter state. */
void __stdcall prepare_base_layout(CppSymbol *tag)
{
    CppSymbol *previous_base = 0, *member, *next, *base_tag;
    BaseClassLink *base;
    int pass;
    for (pass = 1; pass >= 0; --pass) {
        for (member = tag->auxiliary.scope_members; member; member = next) {
            next = member->next;
            if ((member->kind & 31) != 15 || (member->flags & 0x80)) continue;
            base = (BaseClassLink *)member;
            base_tag = cpp_context_stack->items[base->context_index];
            if (((base_tag->detail.class_info->alignment_flags & 0x40) != 0) == pass &&
                !(base->state & 1))
                base->displacement = place_base_subobject(base_tag, &previous_base, tag);
        }
    }
}

/* Original 0x2de60..0x2df96. An unspecified model comes from the first
   visible base, or the default data-pointer kind when there are no bases. */
void __stdcall inherit_class_pointer_attributes(CppSymbol *tag)
{
    int inherited = !tag->detail.class_info->pointer_attributes;
    CppSymbol *member = tag->auxiliary.scope_members, *base_tag;
    BaseClassLink *base;
    if (inherited) {
        for (;;) {
            if (!member) {
                tag->detail.class_info->pointer_attributes = (unsigned short)data_pointer_kind;
                break;
            }
            base = (BaseClassLink *)member;
            member = member->next;
            if ((base->kind & 31) != 15 || (base->flags & 0x80)) continue;
            base_tag = cpp_context_stack->items[base->context_index];
            tag->detail.class_info->pointer_attributes = base_tag->detail.class_info->pointer_attributes;
            break;
        }
    }
    while (member) {
        base = (BaseClassLink *)member;
        member = member->next;
        if ((base->kind & 31) != 15 || (base->flags & 0x80)) continue;
        base_tag = cpp_context_stack->items[base->context_index];
        if (base_tag->detail.class_info->pointer_attributes == tag->detail.class_info->pointer_attributes)
            continue;
        if (inherited) report_syntax_error(269, tag->name, base_tag->name);
        else compiler_warning(3, 149, tag->name, base_tag->name);
        return;
    }
}

/* Original 0x2dfa0..0x2dfba. The original retains a tag argument but does
   not read it; #pragma vtordisp only contributes to this shared switch. */
void __stdcall apply_default_vtordisp(CppSymbol *unused_tag)
{
    suppress_vtordisp |= pragma_vtordisp == 0;
}

/* Original 0x49f00..0x49fb3. Explicit models are checked once the class
   body exists, either exactly or as a lower bound under general pointers. */
void __stdcall check_inheritance_model(CppSymbol *tag)
{
    unsigned char model;
    int configured_model, general, actual, mismatch;
    if (!(tag->detail.class_info->flags2 & 0x10)) return;
    model = tag->detail.class_info->inheritance_model;
    if (!(model & 8) || (model & 0x10)) return;
    general = query_inheritance_model(&configured_model);
    model = tag->detail.class_info->inheritance_model;
    actual = infer_inheritance_model(tag);
    mismatch = general ? (int)(model & 7) < actual : (int)(model & 7) != actual;
    if (mismatch) {
        tag->detail.class_info->inheritance_model |= 0x10;
        actual = infer_inheritance_model(tag);
        report_syntax_error(general ? 287 : 292, tag->name,
            inheritance_model_names[tag->detail.class_info->inheritance_model & 7],
            inheritance_model_names[actual]);
    }
}

/* Original 0x32bf0..0x32c0e. */
char *current_class_name(void)
{
    return current_class_context < 0 ? 0 : class_contexts[current_class_context].symbol->name;
}

/* Original 0x33140..0x33172. Class identity here is the interned name,
   rather than the address of a potentially separate declaration record. */
int __stdcall is_current_class(CppSymbol *symbol)
{
    return current_class_context >= 0 &&
        class_contexts[current_class_context].symbol->name == symbol->name;
}
