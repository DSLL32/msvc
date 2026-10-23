/* C symbols appearing in assembler operand expressions. */
#include "asm.h"
#include "global-ir.h"
#include <recon.h>

extern FrontendSymbol *translation_sentinel, *translation_placeholder;

/* Original 0x50342..0x50540. A symbol can denote data, a label or a
   stack/register-resident object. Its operand descriptor retains the symbol
   while the expression carries the remaining displacement contribution. */
AsmExpression *__stdcall make_asm_symbol(char *name)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    AsmExpression *expression;
    FrontendSymbol *symbol = lookup_asm_symbol(name,1);
    unsigned kind = symbol->kind & 31, storage;
    short expression_kind;
    char *context;
    if (kind < 7 || kind > 10) {
        context = asm_error_context();
        report_syntax_error(420,name,context);
        expression_kind = 0;
    } else {
        asm_operand->address->address_flags &= 0xe8;
        asm_operand->address->address_flags |= 3;
        if (!symbol->type->related && (symbol->type->flags & 0x100))
            asm_operand->address->address_flags |= 0x20;
        ++symbol->references;
        if (!(asm_operand->flags & 1)) {
            if ((kind != 8 && kind != 7) ||
                ((asm_operand_class != 16 && asm_operand_class != 15 &&
                  asm_operand_class != 27 && asm_operand_class != 14) &&
                 symbol != translation_sentinel && symbol != translation_placeholder)) {
                asm_operand->address->address_flags &= 0xef;
                asm_operand->address->address_flags |= 0x10;
            }
            asm_operand->size = asm_size_encoding(asm_symbol_type_size(symbol));
        } else if (!asm_address_mode) {
            asm_operand->address->address_flags &= 0xef;
            asm_operand->address->address_flags |= 0x10;
        }
        storage = symbol->storage[0] & 7;
        if (storage > 3 && storage < 7) {
            if (asm_operand->address->symbol) compiler_warning(1,411,interned_name);
            asm_operand->address->address_flags &= 0xde;
            if (options.option_m && storage > 4 && storage < 7) {
                expression = make_asm_register(8);
                asm_operand->register_symbol = symbol;
                return expression;
            }
            expression = make_asm_address_register(6);
            expression->value.integer = 0;
            asm_operand->frame_symbol = symbol;
            return expression;
        }
        if (!asm_operand->address->symbol) asm_operand->address->symbol = symbol;
        expression_kind = 2;
    }
    if (pool->remaining < (int)sizeof(AsmExpression)) refill_arena(&compiler_arena,pool);
    expression = (AsmExpression *)pool->next;
    pool->remaining -= sizeof(AsmExpression);
    pool->next += sizeof(AsmExpression);
    expression->kind = expression_kind;
    expression->value.integer = 0;
    return expression;
}

/* Original 0x4e290..0x4e323. '$' and __LOCAL_SIZE are persistent special
   symbols. Other unresolved names either recover a C declaration or create
   an assembler-visible label, according to the caller's lookup mode. */
FrontendSymbol *__stdcall lookup_asm_symbol(char *name, int allow_label)
{
    FrontendSymbol *symbol;
    char *context;
    if (name == name_dollar) return translation_sentinel;
    if (name == anonymous_symbol_name) {
        if (!(function_symbol->storage[2] & 1)) report_syntax_error(486);
        return translation_placeholder;
    }
    symbol = find_identifier_symbol(name);
    if (!symbol) {
        if (!allow_label) {
            context = asm_error_context();
            report_syntax_error(406,name,context);
            symbol = make_implicit_declaration(name,parameter_nesting);
        } else {
            symbol = find_label(name);
            if (!symbol) {
                symbol = declare_label(name);
                reference_label(symbol);
            }
        }
    }
    return symbol;
}

/* Original 0x4df51..0x4df9e. Code labels and four-byte function pointers
   use the assembler's near-address sentinel instead of an ordinary size. */
unsigned __stdcall asm_symbol_type_size(FrontendSymbol *symbol)
{
    CType *type;
    if ((symbol->kind & 31) == 7 && type_size(symbol->type) == 4) return 65535;
    if ((symbol->kind & 31) == 8) return 65535;
    type = symbol->type;
    if (symbol->attributes & 0x400)
        type = drop_first_type_shape(remove_type_prefix(type));
    return type_size(type);
}

/* Original0x2caa0..0x2cb2c. An assembler label defines every matching
   unresolved spelling under case-insensitive lookup, or creates a new label. */
void __stdcall emit_asm_label(char *name)
{
    FrontendSymbol *label;
    int found = 0;
    for (label = label_symbols; label; label = label->next) {
        if (!label->detail.value && !compare_names_ignore_case(name,label->name)) {
            found = 1;
            label->attributes |= 0x1000;
            if (!(label->flags & 1)) emit_code_label(make_ir_leaf(2,label,0));
            else report_syntax_error(412,label->name);
        }
    }
    if (!found) {
        label = declare_label(name);
        label->attributes |= 0x1000;
        reference_label(label);
        emit_code_label(make_ir_leaf(2,label,0));
    }
}

/* Original0x4e151..0x4e273. Resolve the member in an explicit tag, the
   operand's symbol/frame/register type, or finally the active tag scopes. */
unsigned __stdcall asm_member_offset(char *name)
{
    FrontendSymbol *symbol, *member;
    unsigned kind;
    if (!asm_member_context || !asm_member_context->associated.members) {
        symbol = asm_operand->address->symbol;
        if (!symbol || !(asm_operand->address->address_flags & 1)) {
            symbol = asm_operand->frame_symbol;
            if (!symbol) symbol = asm_operand->register_symbol;
        }
        asm_member_context = symbol ? (FrontendSymbol *)symbol->type->field08 : 0;
    }
    if (asm_member_context && asm_member_context->associated.members)
        member = find_asm_member(asm_member_context->associated.members,name);
    else member = find_asm_scope_member(name);
    kind = member ? member->kind & 31 : 0;
    if (!member || kind < 1 || kind > 13 || kind == 5 ||
        (kind != 2 && kind != 3) || (member->storage[0] & 7) == 3) {
        char *context = asm_error_context();
        report_syntax_error(411,name,context);
        return 0;
    }
    asm_member_context = (FrontendSymbol *)member->type->field08;
    if (!asm_member_context) {
        if (kind == 2) compiler_warning(1,401,member->name);
        asm_operand->size = asm_size_encoding(asm_symbol_type_size(member));
    }
    return member->detail.value;
}

/* Original0x4e06c..0x4e150. Search every complete struct tag in active
   scopes. Later matches retain the legacy ambiguity diagnostic and depth. */
FrontendSymbol *__stdcall find_asm_scope_member(char *name)
{
    unsigned char depth = scope_depth;
    SymbolScope *scope;
    FrontendSymbol *tag, *member, *result = 0;
    unsigned bucket;
    for (;;) {
        scope = scope_directory[depth];
        for (bucket = 0; bucket <= scope->bucket_mask; ++bucket) {
            tag = scope->buckets ? scope->buckets[bucket & scope->bucket_mask] : 0;
            for (; tag; tag = tag->next) {
                if ((tag->kind & 31) != 4 || (tag->type->flags & 0x200)) continue;
                member = find_asm_member(tag->associated.members,name);
                if (!member) continue;
                if (result) {
                    char *context = asm_error_context();
                    report_syntax_error(410,name,context);
                }
                found_scope_depth = depth;
                result = member;
            }
        }
        if (!depth--) break;
    }
    return result;
}

/* Original0x4e047..0x4e06b. Interned member names are pointer keys;
   unnamed members do not satisfy a lookup. */
FrontendSymbol *__stdcall find_asm_member(FrontendSymbol *members, char *name)
{
    for (; members; members = members->next)
        if (members->name != name_unnamed && members->name == name) break;
    return members;
}

/* Original0x505ba..0x5061c. SEG keeps the grammar expression identity,
   while selecting the current operand's segment-address representation. */
AsmExpression *__stdcall make_asm_segment_value(AsmExpression *value)
{
    AsmAddress *address = asm_operand->address;
    unsigned storage;
    int invalid = !(address->address_flags & 1) || !address->symbol;
    if (!invalid) {
        storage = address->symbol->storage[0] & 7;
        invalid = storage >= 4 && storage <= 6;
    }
    if (invalid) {
        report_syntax_error(415);
        asm_operand->address->symbol = translation_sentinel;
    }
    asm_operand->address->address_flags &= 0xe0;
    asm_operand->address->address_flags |= 11;
    asm_operand->size = 1;
    return value;
}

/* Original0x5061c..0x50694. OFFSET produces an immediate symbol address,
   retaining a placeholder symbol after malformed/register expressions. */
AsmExpression *__stdcall make_asm_offset(AsmExpression *value)
{
    char *context;
    if (!(asm_operand->address->address_flags & 1)) {
        if (value->kind == 3) {
            context = asm_error_context();
            report_syntax_error(404,interned_name,context);
        } else report_syntax_error(415);
        asm_operand->address->symbol = translation_sentinel;
        value->kind = 2;
    }
    asm_operand->address->address_flags &= 0xe8;
    asm_operand->address->address_flags |= 3;
    asm_operand->size = 2;
    return value;
}

/* Original0x4df9f..0x4dfd7. LENGTH reports the outer array bound,
   stepping past the representation prefix used by qualified arrays. */
unsigned __stdcall asm_symbol_length(FrontendSymbol *symbol)
{
    TypeShape *shape;
    if ((symbol->kind & 31) == 8 || !(symbol->attributes & 0x400)) return 1;
    shape = symbol->type->related;
    if (shape && !(shape->flags & 7)) shape = shape->related;
    return shape->field08;
}
