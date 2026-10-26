/* Declaration selection for symbol and global intermediate streams. */
#include "symbol-output.h"
#include "lexer.h"
#include "browser.h"
#include "cpp-context.h"
#include "type-diagnostics-internal.h"
#include <string.h>
#include <recon.h>

/* Original 0x56cb0..0x56cf7. Skip forward function declarations and
   unnamed/compiler-generated declarations except the actual this parameter. */
void __stdcall prepare_symbol_debug_type(CppSymbol *symbol)
{
    unsigned kind = symbol->kind & 31;
    if ((kind == 6 || kind == 7) && !(symbol->flags & 4)) return;
    if (!symbol->name || symbol->name == name_unnamed) return;
    if ((symbol->flags & 0x80) && symbol->name != name_this) return;
    reference_debug_type(symbol->type);
    flush_debug_types();
}

/* Original 0x56d20..0x57189. PCH/debug selection, class member passes,
   aliases, unused-declaration diagnostics and final record selection. */
void __stdcall emit_symbol_declaration(CppSymbol *symbol)
{
    CppSymbol *member, *member_next, *function, *next, *alias, *bound_value;
    unsigned kind, storage, record;
    unsigned short flags;
    int separate_debug, inherited_pch;

    kind = symbol->kind & 0x1f;
    separate_debug = pch_separate_debug && !global_record_kind &&
        (!kind || kind >= 14 || kind == 5 || !(symbol->attributes & 8));
    inherited_pch = (symbol->declaration_flags & 1) &&
        (options.pch_use || options.pch_use_file) &&
        !options.pch_create && !options.pch_create_file;
    if (error_count > 0 || kind == 16 || kind == 17 || (symbol->flags & 8)) return;
    if (kind == 13) {
        if (separate_debug) return;
        write_global_record(9, symbol);
        symbol->flags |= 8;
        return;
    }
    if (kind == 4) {
        if (options.configuration.debug_info && !(symbol->flags & 0x80)) {
            symbol->type->debug_index = debug_tag_type(symbol);
            flush_debug_types();
        }
        if (separate_debug) {
            symbol->declaration_flags |= 1;
            return;
        }
        flags = symbol->type->flags;
        if (flags == 0x402 || flags == 0x404 || flags == 0x408) {
            for (member = symbol->auxiliary.scope_members; member; member = member_next) {
                member_next = member->next;
                if ((member->kind & 0x1f) == 17 && !(member->flags & 0x80)) {
                    for (function = ((OverloadSet *)member)->functions; function; function = next) {
                        next = function->next;
                        emit_symbol_declaration(function);
                    }
                }
            }
            for (member = symbol->auxiliary.scope_members; member; member = next) {
                next = member->next;
                if ((member->kind & 0x1f) == 4 && !(member->flags & 0x80))
                    emit_symbol_declaration(member);
            }
        }
        if (!options.configuration.debug_info || (symbol->flags & 0x80) || inherited_pch) {
            symbol->flags |= 8;
            return;
        }
        record = 11;
        goto emit_record;
    }
    if (kind == 5) {
        if (!options.configuration.debug_info) {
            symbol->flags |= 8;
            return;
        }
        reference_debug_type(symbol->type);
        flush_debug_types();
        if (separate_debug) {
            symbol->declaration_flags |= 1;
            return;
        }
        if (inherited_pch) {
            symbol->flags |= 8;
            return;
        }
        record = 8;
        goto emit_record;
    }
    if (separate_debug) {
        prepare_symbol_debug_type(symbol);
        if ((symbol->storage[0] & 7) == 7) symbol->declaration_flags |= 1;
        return;
    }
    storage = symbol->storage[0] & 7;
    if (storage == 7 && (symbol->declaration_flags & 1)) {
        if (!inherited_pch) write_global_record(11, symbol);
        symbol->flags |= 8;
        return;
    }
    if (symbol->attributes & 0x40) {
        if (kind == 9) {
            alias = symbol_alias(symbol);
            alias->attributes = (alias->attributes & ~4U) | (symbol->attributes & 4);
            /* The native 0x5700c mask retains the alias's low seven bits
               of this byte, copying only the source's high bit. */
            alias->attributes = (alias->attributes & ~0x8000U) | (symbol->attributes & 0x8000);
            alias->auxiliary.value = symbol->auxiliary.value;
            alias->attributes = (alias->attributes & ~2U) | (symbol->attributes & 2);
            alias->type = symbol->type;
            if (!options.configuration.debug_info) return;
            ++alias->references;
            reference_debug_type(symbol->type);
            flush_debug_types();
            record = 7;
            goto emit_record;
        }
    } else if (kind == 9 && (symbol->attributes & 0x1000)) {
        if (!options.configuration.debug_info) return;
        prepare_symbol_debug_type(symbol);
        alias = lookup_symbol_binding(symbol, &bound_value);
        ++alias->references;
        emit_symbol_declaration(alias);
        record = 7;
        goto emit_record;
    }
    if (!symbol->references) {
        if (storage == 1 || kind == 6 || kind == 7) return;
        if (storage == 3 && (type_qualifiers(symbol->type) & 0x1000)) return;
        if ((symbol->storage[0] & 7) == 7) return;
        if ((symbol->kind & 0x1f) == 10) compiler_warning(4, 100, symbol->name);
        else if (global_record_kind) compiler_warning(3, 101, symbol->name);
    }
    kind = symbol->kind & 0x1f;
    if (kind == 6) record = 4;
    else if (kind == 7) {
        if (symbol->cpp.parts.member_qualifiers & 1) {
            alias = method_alias(symbol);
            if (!(alias->flags & 8)) emit_symbol_declaration(alias);
            record = 16;
        } else record = 4;
    } else {
        if ((kind == 10 || kind == 11) && (symbol->attributes & 0x400))
            symbol->type = symbol_reference(symbol)->type;
        record = 1;
    }
    if (options.configuration.debug_info) prepare_symbol_debug_type(symbol);
emit_record:
    write_global_record(record, symbol);
    symbol->flags |= 8;
}

/* Original 0x54940..0x549b8. Expand an overload set, stopping at the
   first non-function member. The original's null input leaves its cursor
   uninitialized; callers provide a symbol and the same condition is retained. */
void __stdcall emit_function_symbols(CppSymbol *symbols)
{
    CppSymbol *cursor, *function;
    unsigned kind;
    if (symbols) {
        kind = symbols->kind & 0x1f;
        cursor = kind == 16 || kind == 17 ? ((OverloadSet *)symbols)->functions : symbols;
    }
    while (cursor) {
        kind = cursor->kind & 0x1f;
        if (kind != 6 && kind != 7) break;
        function = cursor;
        cursor = cursor->next;
        emit_symbol_declaration(function);
    }
}

HashTable *method_aliases; /* original 0x9f334, distinct from per-function aliases */

/* Original 0x56180..0x561b2. Persist the vector-to-scalar deleting
   destructor association in a lazily created pointer-keyed table. */
void __stdcall record_method_alias(CppSymbol *symbol, CppSymbol *alias)
{
    if (!method_aliases) method_aliases = new_hash_table(31, 0, 0, 0);
    set_hash_value(method_aliases, symbol, alias);
}

/* Original 0x561c0..0x561d2, stdcall4. The table owns deleting-destructor
   substitutions; lookup keeps the generic hash routine's null-table behavior. */
CppSymbol *__stdcall method_alias(CppSymbol *symbol)
{
    return find_hash_value(method_aliases, symbol);
}

HashTable *static_member_aliases; /* original 0x9f3d4 */
HashTable *static_member_name_bindings; /* original 0x9f260 */

/* Original 0x55f90..0x55fa3. Keep lookup separate from registration,
   including the generic table helper's missing-table result. */
CppSymbol *__stdcall static_member_alias(CppSymbol *symbol)
{
    return find_hash_value(static_member_aliases, symbol);
}

/* Original 0x55f20..0x55f82. Keep both class-member to external-symbol
   and interned external-name to class-member associations. */
void __stdcall register_static_member_alias(CppSymbol *member, CppSymbol *alias)
{
    if (!static_member_aliases) static_member_aliases = new_hash_table(31, 0, 2, 1);
    set_hash_value(static_member_aliases, member, alias);
    if (!static_member_name_bindings)
        static_member_name_bindings = new_hash_table(31, 0, 1, 0);
    set_hash_value(static_member_name_bindings, alias->name, member);
}

/* Original 0x55fb0..0x55fc5, stdcall4. Resolve the external
   alias by its interned spelling to recover the owning static class member. */
CppSymbol *__stdcall static_member_from_alias(CppSymbol *symbol)
{
    return find_hash_value(static_member_name_bindings, symbol->name);
}

/* Original0x56240..0x56490, stdcall4. Diagnostic names unwrap qualified
   and overload references, then spell conversion/constructor/destructor names
   into arena-two storage while ordinary declarations retain their name owner. */
char *__stdcall format_symbol_name(CppSymbol *symbol)
{
    QualifiedDeclarator *qualified = 0;
    CppSymbol *owner, *alias;
    CType *type;
    ArenaPool *pool = &compiler_arena.pools[2];
    char *text, *result, *cursor;
    unsigned length, index;
    int bytes;
    short diagnostic;
    if ((symbol->kind & 31) == 18) {
        qualified = (QualifiedDeclarator *)symbol;
        symbol = (CppSymbol *)qualified->resolution->binding->target;
    }
    if ((symbol->kind & 31) == 16 || (symbol->kind & 31) == 17)
        symbol = ((OverloadSet *)symbol)->functions;
    if (symbol->declaration_flags & 0x4000) {
        type = drop_first_type_shape(strip_based_shape(symbol->type));
        begin_type_diagnostic();
        diagnostic = format_declarator(0,type);
        text = type_diagnostic_text(diagnostic);
        for (length = 0; text[length]; ++length) ;
        bytes = (length + 14) & ~3U;
        if (bytes > pool->remaining) refill_arena(&compiler_arena,pool);
        result = (char *)pool->next;
        pool->remaining -= bytes;
        pool->next += bytes;
        strncpy(result,"operator`",9);
        strncpy(result + 9,text,length);
        result[length + 9] = '\'';
        result[length + 10] = 0;
        release_type_diagnostic(diagnostic);
        end_type_diagnostic();
        return result;
    }
    if ((symbol->kind & 31) == 7 &&
        ((symbol->declaration_flags & 0x3000) ||
         symbol->name == name_delete_dtor || symbol->name == name_vector_delete_dtor)) {
        owner = cpp_context_stack->items[symbol->auxiliary.declaring_class];
        if (!owner) {
            if (!qualified) fatal_error(507);
            owner = (CppSymbol *)qualified->resolution->binding->next->target;
        }
        for (length = 0; owner->name[length]; ++length) ;
        bytes = (length * 2 + 7) & ~3U;
        if (bytes > pool->remaining) refill_arena(&compiler_arena,pool);
        result = (char *)pool->next;
        pool->remaining -= bytes;
        pool->next += bytes;
        for (index = 0; index <= length; ++index) result[index] = owner->name[index];
        cursor = result + length;
        *cursor++ = ':';
        *cursor++ = ':';
        if ((symbol->declaration_flags & 0x2000) ||
            symbol->name == name_delete_dtor || symbol->name == name_vector_delete_dtor)
            *cursor++ = '~';
        for (index = 0; index <= length; ++index) cursor[index] = owner->name[index];
        return result;
    }
    text = symbol->name;
    if (*text == '?' || (*text == ' ' && error_count)) {
        alias = static_member_from_alias(symbol);
        if (!alias) fatal_error(1,pch_diagnostic_name,0x1a4e);
        text = alias->name;
    }
    return text;
}

/* Original0x54920..0x5493f, stdcall4. The common16 B overload
   prefix owns its name and function-chain slots; other flag bits survive. */
OverloadSet *__stdcall new_named_overload_set(char *name)
{
    OverloadSet *set = allocate_permanent(16);
    set->kind = 17;
    set->name = name;
    set->functions = 0;
    set->declaration_flags &= ~0x300U;
    return set;
}

/* Original0x54f50..0x54fce, stdcall8. Preserve the suffix while
   limiting the prefix to a combined246 characters. Original oversized
   suffixes retain their negative-prefix-count defect; no new guard is added. */
char *__stdcall append_symbol_suffix(char *prefix, char *suffix)
{
    int prefix_length = 0, suffix_length = 0;
    unsigned index;
    char *result;
    while (prefix[prefix_length]) ++prefix_length;
    while (suffix[suffix_length]) ++suffix_length;
    if (prefix_length + suffix_length > 246) prefix_length = 246 - suffix_length;
    result = allocate_permanent((prefix_length + suffix_length + 4) & ~3U);
    for (index = 0; index < (unsigned)prefix_length; ++index) result[index] = prefix[index];
    for (index = 0; index < (unsigned)suffix_length; ++index)
        result[prefix_length + index] = suffix[index];
    result[prefix_length + suffix_length] = 0;
    return result;
}
