/* C++ declaration lookup and scope selection. */
#include "lexer.h"
#include "class-lookup.h"
#include "cpp-context.h"
#include "global-ir.h"
#include "parameters.h"
#include "inline-capture.h"
#include "macros.h"
#include "parameter-scopes.h"
#include <recon.h>

int __stdcall has_class_initialization(CType *type);
CppSymbol *__stdcall bind_function_overload(CppSymbol *overloads, CppSymbol *declaration);
CppSymbol *__stdcall reconcile_typedef(CppSymbol *existing, CppSymbol *declaration);
void __stdcall merge_declaration_storage(CppSymbol *existing, CppSymbol *declaration);
void __stdcall merge_function_defaults(FunctionParameters *current, FunctionParameters *previous,
    CppSymbol *declaration, int inherit);
CppSymbol *__stdcall make_overload_symbol(CppSymbol *symbol);
void __stdcall check_function_defaults(FunctionParameters *parameters, CppSymbol *symbol);
void __stdcall initialize_scope_buckets(SymbolScope *scope, CppSymbol *symbol);
void __stdcall insert_class_member(CppSymbol *symbol);
unsigned current_qc_context; /* original 0x9fc40 */
int __stdcall find_intrinsic_index(char *name);
int __stdcall intrinsic_enabled(int index);
void __stdcall apply_intrinsic(CppSymbol *symbol, int index);
void __stdcall check_missing_defaults(DefaultArgument *defaults, unsigned count,
    FunctionParameters *parameters, unsigned first_parameter, CppSymbol *symbol);
void __stdcall report_parameter_error(unsigned code, CppSymbol *symbol, unsigned parameter);
SymbolScope *declaration_scope(void);
CppSymbol *__stdcall find_name_in_chain(CppSymbol *symbol, char *name);
CppSymbol *__stdcall reconcile_declaration(CppSymbol *existing, CppSymbol *declaration);
CppSymbol *__stdcall lookup_scope_declaration(CppSymbol *symbol, SymbolScope *scope);

CppSymbol *__stdcall find_c_linkage_function(CppSymbol *functions);
void __stdcall prepend_function_overload(CppSymbol *overloads, CppSymbol *declaration);
char *__stdcall make_local_static_name(CppSymbol *symbol);
void __stdcall record_function_symbol_alias(CppSymbol *symbol, CppSymbol *alias);

void __stdcall record_typedef_alias(CppSymbol *symbol, CppSymbol *alias);

/* Original 0x54710..0x54813. A typedef may name its own tag; mixed
   typedef/object declarations preserve the original chain through an alias. */
CppSymbol *__stdcall reconcile_typedef(CppSymbol *existing, CppSymbol *declaration)
{
    CppSymbol *alias;
    CType *type;
    if ((existing->flags & 0x10) && (declaration->flags & 0x10)) {
        if (existing->type == declaration->type) return existing;
        type = declaration->type;
        if ((existing->kind & 31) == 4 &&
            cpp_context_stack->items[type->field08] == existing && !type->related &&
            (existing->type->attributes == type->attributes || !type->attributes))
            return existing;
        return 0;
    }
    if (!(existing->flags & 0x10) && !(declaration->flags & 0x10)) return 0;
    if (existing->flags & 0x10) {
        if ((existing->storage[0] & 7) == 7)
            report_syntax_error(377, existing->name);
    } else if ((declaration->storage[0] & 7) == 7)
        report_syntax_error(378, existing->name);
    if (existing->kind & 0x80) return existing;
    existing->kind |= 0x80;
    alias = make_overload_symbol(declaration);
    alias->kind |= 0x40;
    record_typedef_alias(existing, alias);
    alias->next = existing->next;
    existing->next = alias;
    if (current_class_context >= 0 &&
        class_contexts[current_class_context].last_member == existing)
        class_contexts[current_class_context].last_member = alias;
    return declaration;
}

/* Original 0x535e0..0x535f3. */
CppSymbol *__stdcall lookup_declaration(CppSymbol *symbol)
{
    return lookup_scope_declaration(symbol, declaration_scope());
}

/* Original 0x54a00..0x54a18. Skip the temporary wrapper scopes. */
SymbolScope *declaration_scope(void)
{
    SymbolScope *scope = current_scope;
    while ((scope->flags & 0xff) >= 250) scope = scope->next;
    return scope;
}

/* Original 0x53620..0x53656. The third lookup argument records Pascal
   spelling mode; the original lookup routine retains but ignores that slot. */
CppSymbol *__stdcall lookup_scope_declaration(CppSymbol *symbol, SymbolScope *scope)
{
    CppSymbol *existing = find_scope_symbol(scope, symbol->name,
        (symbol->type->attributes & 0x70) == 0x20);
    return reconcile_declaration(existing, symbol);
}

/* Original 0x53570..0x535dc. Names are interned, so the pointer supplies
   both the bucket hash and the identity comparison. */
CppSymbol *__stdcall find_scope_symbol(SymbolScope *scope, char *name, int flags)
{
    CppSymbol *head;
    if (!scope) return 0;
    if (current_class_context < 0)
        while ((scope->flags & 0xff) >= 250) scope = scope->next;
    if (!scope->buckets) return 0;
    if ((scope->flags & 0xff) >= 250) {
        /* The wrapper-symbol variant uses its +0x24 word as the head of
           the scope's declaration chain. */
        head = scope->buckets[0]->auxiliary.scope_members;
    } else
        head = scope->buckets[(unsigned)name & scope->bucket_mask];
    return find_name_in_chain(head, name);
}

/* Original 0x54e50..0x54e75. */
CppSymbol *__stdcall find_name_in_chain(CppSymbol *symbol, char *name)
{
    while (symbol && symbol->name != name) symbol = symbol->next;
    return symbol;
}

/* Original 0x538a0..0x53a38. A local extern can reuse an outer index,
   after checking parameter conflicts, aliases and overloaded functions. */
CppSymbol *__stdcall lookup_outer_declaration(CppSymbol *declaration)
{
    CppSymbol *parameter, *existing;
    unsigned kind;
    if (declaration_mode == 2 && (current_scope->flags & 0xff) != 1) {
        for (parameter = active_parameter_symbols; parameter; parameter = parameter->next)
            if (parameter->name == declaration->name) {
                report_syntax_error(82, declaration->name);
                return declaration;
            }
    }
    if ((declaration->storage[0] & 7) != 1) return 0;
    existing = find_scope_symbol(global_scope, declaration->name,
        (declaration->type->attributes & 0x70) == 0x20);
    existing = resolve_declaration_alias(existing, 0);
    if (!existing) return 0;
    kind = existing->kind & 31;
    if (kind == 4 || (kind >= 1 && kind <= 13 && kind != 5 &&
        (existing->storage[0] & 7) == 7)) return 0;
    if (kind == 16 || kind == 17) {
        cpp_parser_flags |= 0x04000000UL;
        existing = find_matching_overload(existing, declaration->type, 0);
        cpp_parser_flags &= ~0x04000000UL;
        if (!existing) return 0;
    }
    cpp_parser_flags |= 0x04000000UL;
    if (merge_declared_symbol(existing, declaration)) {
        cpp_parser_flags &= ~0x04000000UL;
        return 0;
    }
    cpp_parser_flags &= ~0x04000000UL;
    declaration->index = existing->index;
    declaration->attributes = (declaration->attributes & ~0x400UL) |
        (existing->attributes & 0x400);
    declaration->attributes = (declaration->attributes & ~2UL) |
        (existing->attributes & 2);
    declaration->attributes = (declaration->attributes & ~0x100UL) |
        (existing->attributes & 0x100);
    insert_current_scope_symbol(declaration);
    if ((existing->kind & 31) == 6) {
        declaration->storage[0] = (declaration->storage[0] & 0xf9) | 1;
        declaration->kind = 6;
        declaration->detail.intrinsic.operation = existing->detail.intrinsic.operation;
        declaration->detail.intrinsic.flags = existing->detail.intrinsic.flags;
    }
    ++existing->references;
    declaration->flags |= 8;
    existing->flags &= ~8;
    return existing;
}

/* Original 0x53a40..0x53b66. Local static storage owns a permanent global
   alias while the local declaration retains its scope and lookup identity. */
void __stdcall record_local_declaration(CppSymbol *declaration)
{
    CppSymbol *alias;
    if ((declaration->kind & 31) != 9 || (declaration->storage[0] & 7) != 3)
        return;
    alias = allocate_permanent(sizeof(CppSymbol));
    *alias = *declaration;
    if (function_symbol && (function_symbol->storage[1] & 1) &&
        ((function_symbol->storage[0] & 7) != 3 ||
         (function_symbol->declaration_flags & 0x400))) {
        if (function_symbol->storage[2] & 4) {
            alias->storage[2] |= 4;
            alias->storage[2] = (alias->storage[2] & ~8) |
                (function_symbol->storage[2] & 8);
            declaration->storage[0] = alias->storage[0];
            declaration->storage[1] = alias->storage[1];
            declaration->storage[2] = alias->storage[2];
        }
        alias->storage[0] = (alias->storage[0] & 0xfa) | 0x42;
    }
    declaration->detail.scope_number = current_declaration_scope;
    alias->detail.scope_number = current_declaration_scope;
    alias->name = make_local_static_name(declaration);
    alias->declaration_flags = (alias->declaration_flags & ~0x100) | 0x200;
    if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    alias->index = next_symbol_index++;
    alias->flags |= 4;
    declaration->attributes |= 0x40;
    record_function_symbol_alias(declaration, alias);
    insert_scope_symbol(global_scope, alias);
}

/* Original 0x53660..0x53899. Reconcile storage and prior declarations,
   including const objects, typedefs, aliases and overloaded functions. */
CppSymbol *__stdcall reconcile_declaration(CppSymbol *existing, CppSymbol *declaration)
{
    CppSymbol *resolved, *result, *tag;
    TypeShape *shape, *old_shape;
    unsigned kind = declaration->kind & 31;
    if (kind != 10 && kind != 11 && kind != 7 && (declaration->storage[0] & 0x20) &&
        (type_qualifiers(declaration->type) & 0x1000)) {
        if (!(declaration->attributes & 4) &&
            (!(classify_incomplete_type(declaration->type) & 0x82) ||
             !(tag = (CppSymbol *)cpp_context_stack->items[declaration->type->field08]) ||
             (tag->kind & 31) != 4 || !has_class_initialization(declaration->type)))
            report_syntax_error(734, declaration->name);
        else if (!declaration_mode) {
            if (!existing)
                declaration->storage[0] = (declaration->storage[0] & ~4) | 3;
            else if ((existing->kind & 31) != 16 && (existing->kind & 31) != 17 &&
                     !(existing->storage[0] & 0x20) && (existing->storage[0] & 7) == 1) {
                declaration->storage[0] = (declaration->storage[0] & ~5) | 2;
                declaration->storage[1] |= 0x10;
            }
            declaration->storage[0] &= ~0x60;
        }
    }
    if (!existing) return 0;
    resolved = resolve_declaration_alias(existing, 0);
    kind = resolved->kind & 31;
    if ((kind == 16 || kind == 17) && (declaration->kind & 31) == 7)
        return bind_function_overload(resolved, declaration);
    if (existing == resolved) {
        result = reconcile_typedef(resolved, declaration);
        if (result) return result;
    }
    if (!declaration_mode && ((resolved->storage[0] & 7) == 1 ||
        (declaration->storage[0] & 7) == 1 ||
        ((resolved->storage[2] & 4) && !(resolved->storage[2] & 8)))) {
        if (merge_declared_symbol(resolved, declaration)) {
            declaration->declaration_flags |= 2;
            return declaration;
        }
        merge_declaration_storage(resolved, declaration);
        return resolved;
    }
    if ((resolved->storage[0] & 7) == 1 && (declaration->storage[0] & 7) == 1) {
        if (merge_declared_symbol(resolved, declaration)) {
            declaration->declaration_flags |= 2;
            return declaration;
        }
        shape = declaration->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        kind = shape->flags & 0x77;
        if (kind == 3 || kind == 0x20 || kind == 0x44) {
            old_shape = resolved->type->related;
            if (old_shape && !(old_shape->flags & 0x77)) old_shape = old_shape->related;
            merge_function_defaults(shape->related->data.parameters,
                old_shape->related->data.parameters, declaration, 1);
        }
        return resolved;
    }
    if (!merge_declared_symbol(resolved, declaration))
        report_syntax_error(86, format_symbol_name(declaration));
    declaration->declaration_flags |= 2;
    return declaration;
}

/* Original 0x549e0..0x549f3. */
void __stdcall insert_symbol(CppSymbol *symbol)
{
    insert_scope_symbol(declaration_scope(), symbol);
}

/* Original 0x54ad0..0x54bb4. Function declarations enter an overload-set
   wrapper; other declarations enter the scope's interned-name bucket. */
void __stdcall insert_scope_symbol(SymbolScope *scope, CppSymbol *symbol)
{
    TypeShape *shape;
    unsigned kind;
    if (symbol->type && (shape = symbol->type->related) != 0) {
        if (!(shape->flags & 0x77)) shape = shape->related;
        if (shape) {
            kind = shape->flags & 0x77;
            if (kind == 3 || kind == 0x20 || kind == 0x44)
                check_function_defaults(shape->related->data.parameters, symbol);
            else if (kind == 4)
                symbol = make_overload_symbol(symbol);
        }
    }
    if (options.configuration.option_qc) scope->qc_context = current_qc_context;
    if (!scope->buckets)
        initialize_scope_buckets(scope, symbol);
    else if ((scope->flags & 0xff) >= 250) {
        if (current_class_context >= 0) insert_class_member(symbol);
        else insert_symbol(symbol);
        return;
    } else
        symbol->next = scope->buckets[(unsigned)symbol->name & scope->bucket_mask];
    if ((scope->flags & 0xff) >= 250)
        scope->buckets[0]->auxiliary.scope_members = symbol;
    else scope->buckets[(unsigned)symbol->name & scope->bucket_mask] = symbol;
}

/* Original 0x54820..0x54914. */
CppSymbol *__stdcall make_overload_symbol(CppSymbol *symbol)
{
    OverloadSet *overloads;
    TypeShape *shape;
    int intrinsic;
    if ((symbol->kind & 31) != 6 && (symbol->kind & 31) != 7)
        return symbol;
    overloads = allocate_permanent(sizeof(OverloadSet));
    overloads->kind = 17;
    if (current_class_context < 0 || (symbol->storage[1] & 4)) overloads->kind = 16;
    overloads->name = symbol->name;
    if (symbol->declaration_flags & 0x200) {
        intrinsic = find_intrinsic_index(symbol->name);
        if (intrinsic != -1 && intrinsic_enabled(intrinsic)) apply_intrinsic(symbol, intrinsic);
    }
    overloads->functions = symbol;
    overloads->declaration_flags = (overloads->declaration_flags & ~0x4300) |
                                  (symbol->declaration_flags & 0x4300);
    shape = symbol->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    check_function_defaults(shape->data.parameters, symbol);
    if (!symbol->index) {
        if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
        if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_symbol_rollover();
        symbol->index = next_symbol_index++;
    }
    return (CppSymbol *)overloads;
}

/* Original 0x3a3b0..0x3a3d4. */
void __stdcall check_function_defaults(FunctionParameters *parameters, CppSymbol *symbol)
{
    unsigned count = parameters->default_count;
    check_missing_defaults(parameters->defaults, count, parameters,
        parameters->declarations->count - count, symbol);
}

/* Original 0x3a350..0x3a3ac. Default records retain both their declaration
   order and the following-parameter chain used by this completeness check. */
void __stdcall check_missing_defaults(DefaultArgument *defaults, unsigned count,
    FunctionParameters *parameters, unsigned first_parameter, CppSymbol *symbol)
{
    unsigned missing = 0;
    while (count--) {
        defaults = defaults->following;
        ++first_parameter;
        if (!defaults->expression) {
            report_parameter_error(548, symbol, first_parameter);
            ++missing;
        }
    }
    if (missing) {
        parameters->default_count = 0;
        parameters->defaults = 0;
    }
}

/* Original 0x53600..0x53613. */
CppSymbol *__stdcall lookup_current_scope_declaration(CppSymbol *declaration)
{
    return lookup_scope_declaration(declaration, current_scope);
}

/* Original 0x549c0..0x549d3. The same insertion is used for parameters
   and tags; it retains the current scope, including temporary wrappers. */
void __stdcall insert_current_scope_symbol(CppSymbol *symbol)
{
    insert_scope_symbol(current_scope, symbol);
}

/* Original 0x54be0..0x54bf1. Outer-name lookup supplies the original category mask 0x112. */
CppSymbol *__stdcall find_outer_name(char *name)
{
    return lookup_scoped_name(name, 0x112);
}

unsigned char class_lookup_operator = 1; /* original 0x9fc84 */
CppSymbol *class_lookup_context; /* original 0x914c0 */
int class_lookup_error; /* original 0x914c4 */

/* Original 0x54c00..0x54db4, including its internal alignment instruction.
   Class scopes return a qualified path; ordinary scopes resolve aliases before
   returning. The first-class marker tracks enclosing implicit-this eligibility. */
CppSymbol *__stdcall lookup_scoped_name(char *name, unsigned categories)
{
    CppSymbol *symbol;
    QualifiedDeclarator *qualified;
    SymbolScope *scope;
    unsigned char index;
    int first_class = 1;
    if (!name) return 0;
    if (class_lookup_operator != 1 && !(token_allocation_flags & 0x10)) {
        if (!class_lookup_context) {
            if (!class_lookup_error) {
                report_syntax_error(class_lookup_operator == 92 ? 227 : 228, name);
                class_lookup_error = 1;
            }
            return 0;
        }
        qualified = lookup_class_name(class_lookup_context, name, categories, 385);
        if (!qualified) report_syntax_error(39, name, class_lookup_context->name);
        return resolve_declaration_alias((CppSymbol *)qualified, 0);
    }
    index = scope_directory_index;
    for (;;) {
        scope = scope_directory[index];
        if ((scope->flags & 0xff) >= 250) {
            qualified = lookup_class_name(scope->buckets[0], name, categories, 385);
            if (qualified) {
                if (first_class && (scope->flags & 0x100) &&
                    ((qualified->resolution->binding->target->kind & 31) != 4))
                    qualified->declaration_flags |= 0x8000;
                symbol_scope_mode = (unsigned char)scope->flags;
                last_lookup_scope = scope;
                return (CppSymbol *)qualified;
            }
            if (!(scope->flags & 0x100)) first_class = 0;
        } else {
            first_class = 1;
            symbol = scope->buckets[(unsigned)name & scope->bucket_mask];
            while (symbol) {
                if (symbol->name == name && symbol_matches_categories(symbol, categories)) {
                    symbol_scope_mode = (unsigned char)scope->flags;
                    last_lookup_scope = scope;
                    return resolve_declaration_alias(symbol, 0);
                }
                symbol = symbol->next;
            }
        }
        if (index-- == 0) return 0;
    }
}

/* Original 0x78400..0x78416, stdcall 12. This entry exposes the path
   search without constructing a qualified declaration or diagnostics. */
QualifierResolution *__stdcall lookup_class_name_paths(CppSymbol *tag, char *name,
    unsigned categories)
{
    return find_class_name_paths(tag, name, categories);
}

/* Original 0x78420..0x7855e. Retain the selected lookup path even after
   ambiguity recovery: alternative paths are reported, then the list is reduced
   to its first entry before constructing the qualified declarator. */
QualifiedDeclarator *__stdcall lookup_class_name(CppSymbol *tag, char *name,
    unsigned categories, unsigned diagnostic)
{
    QualifierResolution *resolution;
    QualifierBinding *first, *binding, *root;
    QualifiedDeclarator *qualified;
    ArenaPool *pool;
    int ambiguous = 0;
    unsigned arena;
    if (tag->name == name && (categories & 1))
        resolution = append_qualifier_reference(0, 0, tag);
    else {
        resolution = find_class_name_paths(tag, name, categories);
        if (!resolution) return 0;
        first = resolution->binding;
        if (resolution->tail != first) {
            if (diagnostic) {
                ambiguous = 1;
                root = first;
                while (root->next) root = root->next;
                report_syntax_error(diagnostic, root->target->name, name);
                for (binding = first; binding; binding = binding->following) {
                    if (binding == first)
                        compiler_warning(1, diagnostic, "could be", name,
                            format_qualified_name(binding, 1));
                    else
                        compiler_warning(1, diagnostic, "or", name,
                            format_qualified_name(binding, 1));
                }
            }
            resolution->tail = resolution->binding;
            resolution->binding->following = 0;
        }
    }
    arena = declaration_mode != 0;
    if (!arena) qualified = allocate_permanent(sizeof(QualifiedDeclarator));
    else {
        if (arena == (unsigned)-1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(QualifiedDeclarator))
            refill_arena(&compiler_arena, pool);
        qualified = (QualifiedDeclarator *)pool->next;
        pool->remaining -= sizeof(QualifiedDeclarator);
        pool->next += sizeof(QualifiedDeclarator);
    }
    qualified->kind = 18;
    qualified->resolution = resolution;
    qualified->name = name;
    if (ambiguous) qualified->declaration_flags |= 2;
    return qualified;
}

/* Original 0x77ee0..0x77f5a. The interned-name prefix records whether the
   spelling can occur in a member list, avoiding the recursive search if not. */
QualifierResolution *__stdcall find_class_name_paths(CppSymbol *tag, char *name,
    unsigned categories)
{
    ClassNameSearch search;
    QualifierResolution *paths;
    if (!(((unsigned char *)name)[-1] & 4)) return 0;
    begin_qualification_allocation(2);
    search.name = name;
    search.categories = categories;
    search.visited_virtual_bases = 0;
    search.shadowed_virtual_bases = 0;
    search.candidates = new_vector(1, 2);
    paths = search_class_name_paths(append_qualifier_reference(0, 0, tag), &search);
    prune_hidden_class_paths(paths, search.shadowed_virtual_bases);
    end_qualification_allocation();
    return paths;
}

/* Original 0x781d0..0x78238. Category tests return a complete integer in
   EAX; the original SBB/INC sequence is not an eight-bit bool return ABI. */
int __stdcall symbol_matches_categories(CppSymbol *symbol, unsigned categories)
{
    unsigned kind = symbol->kind & 31;
    if (kind == 4) return (categories & 0x100) != 0;
    if (kind == 15) return (categories & 1) != 0;
    if (kind == 3 || kind == 2 || kind == 17 || kind == 5 || (symbol->kind & 0x20))
        return (categories & 2) != 0;
    return (categories & 0x10) != 0;
}

/* Original 0x54420..0x544db. Preserve the qualified wrapper while replacing
   its selected target; ordinary aliases choose their visible or underlying
   record according to the lookup/declaration context. */
CppSymbol *__stdcall resolve_declaration_alias(CppSymbol *symbol, CppSymbol *declaration)
{
    CppSymbol *target, *underlying;
    QualifiedDeclarator *qualified = 0;
    unsigned kind;
    if (!symbol) return 0;
    target = symbol;
    if ((symbol->kind & 31) == 18) {
        qualified = (QualifiedDeclarator *)symbol;
        target = (CppSymbol *)qualified->resolution->binding->target;
    }
    underlying = target;
    if (target->kind & 0x80) {
        if (target->flags & 0x10) target = target->next;
        else underlying = target->next;
    }
    if (token_allocation_flags & 0x10) target = underlying;
    else if (declaration) {
        if ((declaration->kind & 31) == 7) {
            kind = target->kind & 31;
            if (kind == 16 || kind == 17) {
                target = find_matching_overload(target, declaration->type, 1);
                if (!target) return 0;
            } else report_syntax_error(63, target->name);
        } else if (declaration->flags & 0x10) target = underlying;
    }
    if (!qualified) return target;
    qualified->resolution->binding->target = (ClassReference *)target;
    return (CppSymbol *)qualified;
}

/* Original 0x55c80..0x55e41. Match an existing signature or introduce a
   distinct overload, retaining C linkage restrictions and global index reuse. */
CppSymbol *__stdcall bind_function_overload(CppSymbol *overloads, CppSymbol *declaration)
{
    CppSymbol *matched, *global;
    TypeShape *shape;
    int intrinsic;
    matched = find_matching_overload(overloads, declaration->type, 1);
    if (!matched) {
        if ((declaration->name == name_main || declaration->name == name_winmain ||
             declaration->name == name_dllmain) && !(declaration->declaration_flags & 0x400))
            report_syntax_error(731, format_symbol_name(declaration));
        if (declaration->declaration_flags & 0x200) {
            if (find_c_linkage_function(overloads)) report_syntax_error(733, declaration->name);
            else {
                intrinsic = find_intrinsic_index(declaration->name);
                if (intrinsic != -1 && intrinsic_enabled(intrinsic)) apply_intrinsic(declaration, intrinsic);
            }
        }
        shape = declaration->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        check_function_defaults(shape->data.parameters, declaration);
        prepend_function_overload(overloads, declaration);
        if (declaration_mode >= 2) {
            global = find_scope_symbol(global_scope, declaration->name,
                (declaration->type->attributes & 0x70) == 0x20);
            if (global && ((global->kind & 31) == 16 || (global->kind & 31) == 17)) {
                global = find_matching_overload(global, declaration->type, 0);
                if (global) {
                    declaration->index = global->index;
                    declaration->flags |= 8;
                    global->flags &= ~8;
                    declaration->attributes = (declaration->attributes & ~2) | (global->attributes & 2);
                    ++global->references;
                    return declaration;
                }
            }
        }
        if (!declaration->index) {
            if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
            if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_symbol_rollover();
            declaration->index = next_symbol_index++;
        }
        return declaration;
    }
    if ((matched->declaration_flags & 2) || (declaration->declaration_flags & 2)) return matched;
    if (merge_declared_symbol(matched, declaration)) return 0;
    merge_declaration_storage(matched, declaration);
    return matched;
}

int __stdcall compare_declaration_parameters(DeclarationParameterInfo *left,
    DeclarationParameterInfo *right);
unsigned __stdcall reconcile_declaration_type(CppSymbol *existing,
    CppSymbol *declaration, CType *type);
short __stdcall format_function_parameters(short text, FunctionParameters *parameters,
    unsigned flags);

/* Original 0x3a6e0..0x3a9ac. Function-shape identity provides the fast
   path; otherwise compare the implicit object and explicit parameters before
   merging the return type and defaults. Callers supply an existing symbol.
   Like the original, a null initial symbol leaves the iterator uninitialized. */
CppSymbol *__stdcall find_matching_overload(CppSymbol *functions, CType *type,
    int merge_defaults)
{
    CppSymbol *next, *candidate, *owner_function;
    CType *old_type, *object_type, *old_object;
    TypeShape *shape = type->related, *old_shape;
    FunctionParameters *parameters, *old_parameters;
    int result;
    short old_text, new_text;
    char *owner, *old_string, *new_string;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    parameters = shape->data.parameters;
    object_type = parameters->associated_type;
    if (functions) {
        if ((functions->kind & 31) == 16 || (functions->kind & 31) == 17)
            next = ((OverloadSet *)functions)->functions;
        else next = functions;
    }
    for (;;) {
        candidate = next;
        if (!candidate || ((candidate->kind & 31) != 6 &&
                           (candidate->kind & 31) != 7)) return 0;
        next = candidate->next;
        old_shape = candidate->type->related;
        if (old_shape && !(old_shape->flags & 0x77)) old_shape = old_shape->related;
        old_parameters = old_shape->data.parameters;
        old_object = old_parameters->associated_type;
        if (shape != old_shape) {
            if (object_type != old_object) {
                if (!object_type) {
                    if (!(old_parameters->flags & 1)) continue;
                } else if (!old_object ||
                    ((object_type->attributes ^ old_object->attributes) & 0x3000) ||
                    ((object_type->related->attributes ^ old_object->related->attributes) & 7))
                    continue;
            }
            if (parameters->declarations != old_parameters->declarations ||
                (parameters->default_count && old_parameters->default_count &&
                 old_parameters->default_count >= parameters->default_count)) {
                result = compare_declaration_parameters(parameters->declarations,
                    old_parameters->declarations);
                if (result == 1) {
                    begin_type_diagnostic();
                    old_text = reserve_type_diagnostic();
                    new_text = reserve_type_diagnostic();
                    owner = "";
                    owner_function = ((OverloadSet *)functions)->functions;
                    if (owner_function->declaration_flags & 0x400)
                        owner = ((CppSymbol *)cpp_context_stack->items[
                            owner_function->auxiliary.declaring_class])->name;
                    format_function_parameters(old_text, old_parameters, 0);
                    format_function_parameters(new_text, parameters, 0);
                    new_string = type_diagnostic_text(new_text);
                    old_string = type_diagnostic_text(old_text);
                    report_syntax_error(579, owner, candidate->name, new_string,
                        owner, candidate->name, old_string);
                    candidate->declaration_flags |= 2;
                    end_type_diagnostic();
                    return candidate;
                }
                if (result == 2) {
                    if (candidate->name == name_delete &&
                        (candidate->declaration_flags & 0x400) &&
                        parameters->declarations->count != old_parameters->declarations->count) {
                        report_syntax_error(669);
                        candidate->declaration_flags |= 2;
                        return candidate;
                    }
                    continue;
                }
            }
        }
        old_type = candidate->type;
        old_shape = old_type->related;
        if (old_shape && !(old_shape->flags & 0x77)) old_shape = old_shape->related;
        if (!((type->flags ^ old_type->flags) & 0xefff) &&
            type->field08 == old_type->field08) {
            if (old_shape->related == shape->related) {
                merge_function_defaults(parameters, old_parameters, candidate, merge_defaults);
                return candidate;
            }
            if (!reconcile_declaration_type(candidate, 0, type)) return candidate;
        }
        if (candidate->name != name_new) {
            report_symbol_declaration_error(556, candidate);
            return candidate;
        }
    }
}

int __stdcall compare_overload_parameter(int previous, CType *left, CType *right);
/* Original 0x3a2d0..0x3a32c. Retain the accumulated ambiguity classification
   across corresponding parameter types; different arity cannot match. */
int __stdcall compare_declaration_parameters(DeclarationParameterInfo *left,
    DeclarationParameterInfo *right)
{
    int count = left->count, index, result = 0;
    if (right->count != count) return 2;
    for (index = 0; index < count; ++index) {
        result = compare_overload_parameter(result, left->types[index], right->types[index]);
        if (result == 2) break;
    }
    return result;
}


/* Original 0x3a070..0x3a2cf. Compare the canonical parameter and its first
   derived shape. Results are exact/previous (0), ambiguous (1), distinct (2).
   In this classification, reference qualification may erase a difference;
   pointer/array pairs retain the incoming ambiguity when one already exists. */
int __stdcall compare_overload_parameter(int previous, CType *left, CType *right)
{
    TypeShape *left_shape, *right_shape;
    unsigned left_kind, right_kind;
    if (left == right) return previous;
    if (!left || !right || ((left->flags ^ right->flags) & 0xefff) ||
        left->field08 != right->field08) return 2;
    left_shape = left->related;
    right_shape = right->related;
    if (!left_shape) {
        if (right_shape && (!(right_shape->flags & 0x40) || right_shape->related ||
            ((left->attributes ^ right_shape->attributes) & 0x3000))) return 2;
    } else if (!right_shape) {
        if (!(left_shape->flags & 0x40) || left_shape->related ||
            ((left_shape->attributes ^ right->attributes) & 0x3000)) return 2;
    } else if (left_shape == right_shape) {
        if ((left->attributes ^ right->attributes) & 0x3000) return 2;
    } else {
        if (left_shape->related != right_shape->related ||
            ((left_shape->flags ^ right_shape->flags) & 0x40) ||
            (!!(left_shape->attributes & 12) != !!(right_shape->attributes & 12)) ||
            (!(right_shape->attributes & 12) &&
             ((left_shape->attributes ^ right_shape->attributes) & 7)) ||
            left_shape->flags == right_shape->flags) return 2;
        left_kind = left_shape->flags & 0x77;
        right_kind = right_shape->flags & 0x77;
        if ((left_kind == 1 && right_kind == 2) ||
            (left_kind == 2 && right_kind == 1)) {
            if ((left->attributes ^ right->attributes) & 0x3000) return 2;
            if (previous != 1) return 0;
        }
    }
    return 1;
}

/* Original 0x55e50..0x55e64. Keep newest overload first, sharing the
   existing symbol-chain link rather than allocating a second list node. */
void __stdcall prepend_function_overload(CppSymbol *overloads, CppSymbol *declaration)
{
    declaration->next = ((OverloadSet *)overloads)->functions;
    ((OverloadSet *)overloads)->functions = declaration;
}

/* Original 0x37dc0..0x37df5. Look only for a function-overload wrapper;
   data/tag declarations with the same interned spelling do not qualify. */
CppSymbol *__stdcall find_class_special_member(CppSymbol *tag, char *name)
{
    ClassMemberSearch search;
    CppSymbol *member;
    begin_class_member_search(&search, name, tag);
    do {
        member = next_class_member_search(&search);
    } while (member && (member->kind & 31) != 17);
    return member;
}


/* Original 0x337f0..0x33863. Before completion use the declaration list;
   afterwards inspect only the bucket for this interned name. */
void __stdcall begin_class_member_search(ClassMemberSearch *search, char *name, CppSymbol *tag)
{
    ClassInfo *info = tag->detail.class_info;
    ClassMemberBucket *bucket;
    if (!info->member_index) {
        search->indexed = 0;
        search->position.next = tag->auxiliary.scope_members;
    } else {
        bucket = &info->member_index[(unsigned)name & info->member_index_mask];
        search->position.items = bucket->symbols;
        search->remaining = bucket->count;
        search->indexed = 1;
    }
    search->name = name;
}


/* Original 0x33870..0x338ea. The linked-list path advances its saved
   cursor only on a match; the indexed path consumes each inspected entry. */
CppSymbol *__stdcall next_class_member_search(ClassMemberSearch *search)
{
    CppSymbol *member;
    if (search->indexed != 1) {
        member = search->position.next;
        while (member) {
            if (member->name == search->name) {
                search->position.next = member->next;
                return member;
            }
            member = member->next;
        }
        return 0;
    }
    if (search->position.items && search->remaining) {
        do {
            member = *search->position.items++;
            --search->remaining;
            if (member->name == search->name) return member;
        } while (search->remaining);
    }
    return 0;
}

/* Original 0x54bc0..0x54bd2. A qualifier searches the class/tag categories
   through the shared scoped-name resolver, retaining its native wrapper ABI. */
CppSymbol *__stdcall find_qualifier_name(char *name)
{
    return lookup_scoped_name(name, 0x101);
}

/* Original 0x77f60..0x781ca. Search the current class first, retaining all
   qualifying base paths unless the caller requests the first result. The
   two pointer-keyed tables distinguish visited from hidden virtual bases. */
QualifierResolution *__stdcall search_class_name_paths(QualifierResolution *path,
    ClassNameSearch *search)
{
    CppSymbol *tag = (CppSymbol *)path->binding->target, *member, *base_tag;
    BaseClassLink *base;
    ClassMemberSearch members;
    VirtualBaseSearch virtual_bases;
    QualifierResolution *result = 0, *found;
    if ((tag->flags & 0x10) && (tag->type->flags & 0x200)) {
        member = find_hash_value(tag->auxiliary.enumerators, search->name);
        return member ? append_qualifier_reference(0, path->binding, member) : 0;
    }
    if ((tag->kind & 31) != 4)
        tag = cpp_context_stack->items[((ClassReference *)tag)->context_index];
    begin_class_member_search(&members, search->name, tag);
    while ((member = next_class_member_search(&members)) != 0) {
        if (!symbol_matches_categories(member, search->categories) ||
            !class_candidate_is_distinct(member, search->candidates)) continue;
        append_vector(search->candidates, member);
        begin_virtual_base_search(&virtual_bases, tag);
        while (next_virtual_base_search(&virtual_bases)) {
            if (!search->visited_virtual_bases) {
                search->visited_virtual_bases = new_hash_table(4, 2, 0, 0);
                search->shadowed_virtual_bases = new_hash_table(4, 2, 0, 0);
            }
            set_hash_value(search->visited_virtual_bases, virtual_bases.tag, (void *)1);
            set_hash_value(search->shadowed_virtual_bases, virtual_bases.tag, (void *)1);
        }
        member = resolve_declaration_alias(member, 0);
        return append_qualifier_reference(0, path->binding, member);
    }
    if (search->name == name_assignment) return 0;
    if (!(tag->flags & 4) && (token_allocation_flags & 2)) return 0;
    for (base = (BaseClassLink *)tag->auxiliary.scope_members; base;
         base = (BaseClassLink *)base->next) {
        if ((base->kind & 31) != 15 || (base->flags & 0x80)) continue;
        base_tag = cpp_context_stack->items[base->context_index];
        if (base->state & 1) {
            if (search->categories & 4) continue;
            if (!search->visited_virtual_bases) {
                search->visited_virtual_bases = new_hash_table(4, 2, 0, 0);
                search->shadowed_virtual_bases = new_hash_table(4, 2, 0, 0);
            }
            if (find_hash_value(search->visited_virtual_bases, base_tag) == (void *)1)
                continue;
            set_hash_value(search->visited_virtual_bases, base_tag, (void *)1);
        }
        found = search_class_name_paths(append_qualifier_reference(0,
            path->binding, (CppSymbol *)base), search);
        if (found) {
            if (result) {
                result->tail->following = found->binding;
                result->tail = found->tail;
            } else result = found;
            if (search->categories & 8) return result;
        }
    }
    return result;
}

/* Original 0x78240..0x7834d. Virtual-base entries are represented only
   once. Static data and named types suppress only the identical symbol. */
int __stdcall class_candidate_is_distinct(CppSymbol *symbol, Vector *candidates)
{
    short index;
    CppSymbol *previous;
    if ((symbol->kind & 31) == 15 && (((BaseClassLink *)symbol)->state & 1)) {
        for (index = 0; index < candidates->count; ++index) {
            previous = candidates->items[index];
            if ((previous->kind & 31) == 15 &&
                (((BaseClassLink *)previous)->state & 1)) return 0;
        }
    } else if (is_static_data_member(symbol) || (symbol->flags & 0x10)) {
        for (index = 0; index < candidates->count; ++index)
            if (candidates->items[index] == symbol) return 0;
    }
    return 1;
}

/* Original 0x7c060..0x7c099. A present, even empty, group vector enables
   iteration. Leave the other cursor fields untouched when it is absent. */
void __stdcall begin_virtual_base_search(VirtualBaseSearch *search, CppSymbol *tag)
{
    if (tag->detail.class_info->virtual_table_groups) {
        search->active = 1;
        search->groups = tag->detail.class_info->virtual_table_groups;
        search->index = 0;
    } else search->active = 0;
}

/* Original 0x7c0a0..0x7c0dc. The current group and both of its leading
   words are observable cursor state; advancing consumes one short index. */
int __stdcall next_virtual_base_search(VirtualBaseSearch *search)
{
    if (search->active && search->index < search->groups->count) {
        search->group = search->groups->items[search->index++];
        search->tag = search->group->tag;
        search->field14 = search->group->object_offset;
        return 1;
    }
    return 0;
}

/* Original 0x78350..0x78398. Remove hidden alternatives in place, updating
   the tail only for retained entries, as in the native linked-path walker. */
void __stdcall prune_hidden_class_paths(QualifierResolution *paths, HashTable *hidden)
{
    QualifierBinding **link;
    if (!paths || paths->binding == paths->tail || !hidden) return;
    link = &paths->binding;
    while (*link) {
        if (qualifier_path_is_hidden(*link, hidden))
            *link = (*link)->following;
        else {
            paths->tail = *link;
            link = &(*link)->following;
        }
    }
}

/* Original 0x783a0..0x783fb, stdcall8. Only virtual-base links
   can be hidden by the search's already-visited class set. */
int __stdcall qualifier_path_is_hidden(QualifierBinding *path, HashTable *hidden)
{
    BaseClassLink *base;
    for (path = path->next; path; path = path->next) {
        base = (BaseClassLink *)path->target;
        if ((base->kind & 31) == 15 && (base->state & 1) &&
            find_hash_value(hidden, cpp_context_stack->items[base->context_index]) ==
                (void *)1)
            return 1;
    }
    return 0;
}

/* Original 0x53f40..0x5407b. Repeated declarations are checked against
   their canonical type, storage, language and definition status. Return
   nonzero on conflict, marking both declarations before reporting it. */
int __stdcall merge_declared_symbol(CppSymbol *existing, CppSymbol *declaration)
{
    unsigned code;
    if (declaration->declaration_flags & 2) return 1;
    if (!equivalent_base_type_flags(existing->type->flags, declaration->type->flags) ||
        cpp_context_stack->items[existing->type->field08] !=
        cpp_context_stack->items[declaration->type->field08]) code = 371;
    else {
        code = reconcile_declaration_type(existing, declaration, 0);
        if (code == 40) {
            declaration->declaration_flags |= 2;
            existing->declaration_flags |= 2;
            report_named_type_difference(40, 0, format_symbol_name(existing),
                declaration->type, existing->type);
            return 1;
        }
        if (!code) {
            if (!compatible_declaration_storage(existing, declaration))
                code = (existing->kind & 31) == 7 ? 375 : 370;
            else if (!compatible_declaration_attributes(existing->type->attributes,
                declaration->type->attributes, (existing->kind & 31) == 7)) code = 373;
            else if (!(existing->attributes & 4) || !(declaration->attributes & 4))
                return 0;
            else code = 374;
        }
    }
    declaration->declaration_flags |= 2;
    existing->declaration_flags |= 2;
    report_syntax_error(code, format_symbol_name(existing));
    return 1;
}

/* Original 0x54080..0x5409d. Base-type equivalence ignores only the
   qualifier bit 0x1000; the two stack arguments remain sixteen-bit values. */
int __stdcall equivalent_base_type_flags(unsigned short left, unsigned short right)
{
    return !((left ^ right) & 0xefff);
}

int __stdcall equivalent_shape_attributes(unsigned short left, unsigned short right);
/* Original 0x541d0..0x5428f. Report arity and individual parameter
   differences, keeping extension-mode warnings distinct from errors. Parameter
   types have already been interned, so their pointer identities are compared. */
void __stdcall diagnose_parameter_list_difference(FunctionParameters *existing,
    FunctionParameters *declaration)
{
    DeclarationParameterInfo *old_parameters = existing->declarations;
    DeclarationParameterInfo *new_parameters = declaration->declarations;
    int count = old_parameters->count, parameter;
    if (new_parameters->count < count) {
        count = new_parameters->count;
        if (options.configuration.extensions_enabled) compiler_warning(1, 30);
        else report_syntax_error(190);
    } else if (new_parameters->count > count) {
        if (options.configuration.extensions_enabled) compiler_warning(1, 31);
        else report_syntax_error(191);
    }
    for (parameter = 1; parameter <= count; ++parameter) {
        if (old_parameters->types[parameter - 1] !=
            new_parameters->types[parameter - 1]) {
            if (options.configuration.extensions_enabled)
                compiler_warning(1, 28, parameter);
            else report_syntax_error(192, parameter);
        }
    }
}

/* Original 0x54290..0x5441c. Reconcile shape chains after the caller has
   checked the base type. A leading based/segment modifier has its own
   adoption rules; array bounds and function parameters are then compared
   at their original nesting depths. */
unsigned __stdcall reconcile_declaration_type(CppSymbol *existing,
    CppSymbol *declaration, CType *type)
{
    TypeShape *old_shape = existing->type->related, *new_shape;
    CppSymbol *old_segment;
    int old_based, new_based;
    unsigned depth = 0, kind;
    if (declaration) type = declaration->type;
    new_shape = type->related;
    old_based = old_shape && !(old_shape->flags & 0x77);
    new_based = new_shape && !(new_shape->flags & 0x77);
    if (new_based) {
        if (old_based) {
            if (new_shape->data.based_symbol != old_shape->data.based_symbol) {
                /* Segment records share the ordinary 52-byte symbol layout.
                   Predefined segment identities 5 and 2 permit this change;
                   this is their symbol index at +16, not a segment-kind field. */
                old_segment = old_shape->data.based_symbol;
                if (old_segment->index != 5 && old_segment->index != 2) return 376;
            }
        } else {
            existing->type = make_qualified_type(existing->type, new_shape->flags,
                new_shape->attributes, &new_shape->data.word);
            existing->attributes |= 2;
        }
        if (new_shape && !(new_shape->flags & 0x77)) new_shape = new_shape->related;
    }
    if (old_based && old_shape && !(old_shape->flags & 0x77))
        old_shape = old_shape->related;
    for (;;) {
        if (new_shape == old_shape) return 0;
        if (!old_shape || !new_shape) return 40;
        ++depth;
        kind = old_shape->flags & 0x77;
        if ((new_shape->flags ^ old_shape->flags) & 0x77) return 372;
        if (!equivalent_shape_attributes(old_shape->attributes, new_shape->attributes))
            return 373;
        if (kind == 2) {
            if (new_shape->data.array_bound != old_shape->data.array_bound &&
                (depth != 1 || (old_shape->data.array_bound && new_shape->data.array_bound)))
                return 369;
        } else if (kind == 4) {
            if (new_shape->data.parameters != old_shape->data.parameters)
                diagnose_parameter_list_difference(old_shape->data.parameters,
                    new_shape->data.parameters);
            merge_function_defaults(new_shape->data.parameters, old_shape->data.parameters,
                existing, 2);
        }
        old_shape = old_shape->related;
        new_shape = new_shape->related;
    }
}

/* Original 0x540a0..0x541cb. Compare storage classes and DLL modifiers;
   the extension mode permits the native extern-to-static redeclaration
   warning only while the earlier declaration is still undefined. */
int __stdcall compatible_declaration_storage(CppSymbol *existing, CppSymbol *declaration)
{
    StorageSpec old_storage, new_storage;
    unsigned old_kind, new_kind;
    old_storage.flags = existing->storage[0] | (existing->storage[1] << 8);
    old_storage.extended = existing->storage[2];
    new_storage.flags = declaration->storage[0] | (declaration->storage[1] << 8);
    new_storage.extended = declaration->storage[2];
    old_kind = old_storage.flags & 7;
    new_kind = new_storage.flags & 7;
    if (((existing->kind & 31) == 7 || !(existing->attributes & 4)) &&
        (old_kind == new_kind || (old_kind == 1 && new_kind == 2)) &&
        equivalent_storage_extensions(old_storage, new_storage)) return 1;
    if (new_kind == 3) {
        if (options.configuration.extensions_enabled && old_kind == 1 &&
            !(existing->flags & 4)) {
            compiler_warning(4, 211);
            return 1;
        }
        return 0;
    }
    if ((old_kind == 7 || new_kind == 7) && !declaration_mode) return 0;
    if (!(old_storage.extended & 4) && (new_storage.extended & 4)) return 0;
    if ((old_storage.extended & 4) && new_kind == 3) return 0;
    return 1;
}

int __stdcall inherit_storage_modifiers(StorageSpec *destination, StorageSpec source);

/* Original 0x53b70..0x53ed1. Adopt completed array/based types and merge
   the accepted declaration's linkage, definition and DLL storage state.
   Both symbol records remain mutable during this reconciliation. */
void __stdcall merge_declaration_storage(CppSymbol *existing, CppSymbol *declaration)
{
    TypeShape *old_shape = existing->type->related;
    TypeShape *new_shape = declaration->type->related;
    int old_based = old_shape && !(old_shape->flags & 0x77);
    int new_based = new_shape && !(new_shape->flags & 0x77);
    unsigned kind;
    StorageSpec storage;

    if ((old_based && new_based && old_shape->data.word != new_shape->data.word) ||
        (!old_based && new_based)) {
        if (old_based) existing->type = strip_based_shape(existing->type);
        else existing->attributes |= 2;
        new_shape = declaration->type->related;
        existing->type = make_qualified_type(existing->type, new_shape->flags,
            new_shape->attributes, &new_shape->data.word);
    }
    new_shape = declaration->type->related;
    if (new_shape && !(new_shape->flags & 0x77)) new_shape = new_shape->related;
    old_shape = existing->type->related;
    if (old_shape && !(old_shape->flags & 0x77)) old_shape = old_shape->related;
    if (new_shape) {
        kind = new_shape->flags & 0x77;
        if (kind == 2) {
            if (new_shape->data.array_bound && !old_shape->data.array_bound) {
                existing->type = declaration->type;
                existing->attributes &= ~0x100;
            }
        } else if (kind == 3 || kind == 0x20 || kind == 0x44)
            merge_function_defaults(new_shape->related->data.parameters,
                old_shape->related->data.parameters, existing, 1);
        else if (kind == 4) {
            if ((existing->declaration_flags & 0x400) && current_class_context >= 0 &&
                class_contexts[current_class_context].symbol ==
                cpp_context_stack->items[existing->auxiliary.declaring_class])
                report_syntax_error(535, format_symbol_name(existing));
            if ((existing->declaration_flags & 0x400) && (existing->flags & 0x80) &&
                !(cpp_parser_flags & 0x200) &&
                ((existing->declaration_flags & 0x3000) ||
                 (existing->cpp.parts.member_flags & 0x40)))
                report_syntax_error(251, format_symbol_name(declaration));
            if (declaration->flags & 4) {
                if (!(existing->flags & 4)) existing->flags |= 4;
                else report_symbol_declaration_error(84, existing);
            }
        }
    }
    if (current_class_context < 0 && !(token_allocation_flags & 0x400)) {
        if (existing->declaration_flags & 0x200) {
            if (declaration->declaration_flags & 0x100)
                report_syntax_error(732, declaration->name);
            else declaration->declaration_flags = (declaration->declaration_flags & ~0x300) |
                (existing->declaration_flags & 0x300);
        } else if (declaration->declaration_flags & 0x200)
            report_syntax_error(732, declaration->name);
        else if (!(existing->declaration_flags & 0x100) &&
            (declaration->declaration_flags & 0x100)) compiler_warning(4, 507);
    }
    if (((existing->storage[2] & 12) == 12 && (declaration->storage[2] & 12) == 4) ||
        ((existing->storage[2] & 12) == 4 &&
         (!(declaration->storage[2] & 4) || (declaration->storage[2] & 8)) &&
         (declaration->storage[0] & 7) != 3 && !(declaration->storage[1] & 1) &&
         !(token_allocation_flags & 0x400) && !(declaration->declaration_flags & 2) &&
         !(existing->declaration_flags & 2))) {
        compiler_warning(1, 273, format_symbol_name(existing));
        if (!(existing->declaration_flags & 0x400))
            existing->storage[0] = (existing->storage[0] & 0xf9) | 1;
        existing->storage[2] |= 12;
    }
    if ((existing->storage[0] & 7) == 1 && (declaration->storage[0] & 7) != 1) {
        if ((existing->attributes & 2) && (existing->type->related->flags & 0x77))
            report_syntax_error(179, existing->name);
        if (existing->storage[1] & 0x10) declaration->storage[1] |= 0x10;
        if (existing->storage[2] & 4) declaration->storage[2] |= 4;
        if (existing->storage[2] & 8) declaration->storage[2] |= 8;
        if (type_qualifiers(declaration->type) & 0x1000) {
            declaration->storage[0] = (declaration->storage[0] & 0xfa) | 2;
            declaration->storage[1] |= 0x10;
            declaration->storage[0] &= 0x9f;
        }
        existing->storage[0] = declaration->storage[0];
        existing->storage[1] = declaration->storage[1];
        existing->storage[2] = declaration->storage[2];
    }
    if (declaration->flags & 4) existing->flags |= 4;
    if (declaration->attributes & 4) existing->attributes |= 4;
    storage.flags = declaration->storage[0] | (declaration->storage[1] << 8);
    storage.extended = declaration->storage[2];
    if (inherit_storage_modifiers((StorageSpec *)existing->storage, storage))
        existing->flags &= ~8;
}

/* Original0x55c00..0x55c7d, stdcall4. Find the first C-linkage
   function in a declaration/overload chain. NULL skips the original cursor
   store at55c23, leaving entryESP-8 unwritten at its55c30 read. */
CppSymbol *__stdcall find_c_linkage_function(CppSymbol *functions)
{
    CppSymbol *next, *function;
    unsigned kind;
    if (functions) {
        kind = functions->kind & 31;
        if (kind == 16 || kind == 17)
            next = ((OverloadSet *)functions)->functions;
        else next = functions;
    }
#ifdef RECON_NATIVE_TEST
    /* Preserve the Win32 source bug; never read an unrelated native cursor. */
    if (!functions) { __asm { int 3 } }
#endif
    while (next) {
        function = next;
        kind = function->kind & 31;
        if (kind != 6 && kind != 7) break;
        next = function->next;
        if (function->declaration_flags & 0x200) return function;
    }
    return 0;
}

char *__stdcall format_qualified_path(QualifierBinding *binding);

/* Original 0x78710..0x7879d. Verbose ambiguity notes omit the leaf,
   then describe each enclosing base/class in the shared literal workspace.
   An empty chain retains the workspace contents, as in the original. */
char *__stdcall format_qualified_name(QualifierBinding *binding, int verbose)
{
    char *cursor = (char *)expression_storage;
    char *end = (char *)expression_storage + 2047;
    char *text;
    if (!verbose)
        return format_qualified_path(binding);
    binding = binding->next;
    while (binding && cursor != end) {
        text = binding->target->name;
        while (*text && cursor != end)
            *cursor++ = *text++;
        binding = binding->next;
        if (binding) {
            text = binding->next ? "' of base '" : "' of class '";
            while (*text && cursor != end)
                *cursor++ = *text++;
        }
        *cursor = 0;
    }
    return (char *)expression_storage;
}

HashTable *typedef_aliases; /* original 0x9f348; shared PCH root98 */

/* Original 0x54680..0x546b3. Resolve declarations before hashing;
   the alias itself is the table key and its original symbol is the value. */
void __stdcall record_typedef_alias(CppSymbol *symbol, CppSymbol *alias)
{
    if (!typedef_aliases)
        typedef_aliases = new_hash_table(16, 0, 7, 6);
    set_hash_value(typedef_aliases, alias, symbol);
}

unsigned resolved_declaration_index(CppSymbol *symbol);

/* Original 0x54640..0x5464e. Hash aliases by the resolved declaration's
   compiler index rather than the transient key address. */
unsigned __cdecl hash_resolved_declaration(void *key)
{
    return resolved_declaration_index((CppSymbol *)key);
}

CppSymbol *__stdcall last_overload_declaration(OverloadSet *overloads);

/* Original 0x54590..0x54612. Base paths and overload sets resolve to
   an actual declaration before lazily assigning its shared compiler index. */
unsigned resolved_declaration_index(CppSymbol *symbol)
{
    unsigned kind = symbol->kind & 31;
    if (kind == 15)
        symbol = cpp_context_stack->items[((BaseClassLink *)symbol)->context_index];
    else if (kind == 16 || kind == 17)
        symbol = last_overload_declaration((OverloadSet *)symbol);
    if (!symbol->index) {
        if (next_symbol_index >= 0x7fffffffUL)
            fatal_error(55);
        if (next_symbol_index >= browser_symbol_limit && browser_filename)
            browser_symbol_rollover();
        symbol->index = next_symbol_index++;
    }
    return symbol->index;
}

/* Original0x78680..0x78703, stdcall4. Build the qualified path backwards
   at the end of the shared literal workspace, retaining the original budget. */
char *__stdcall format_qualified_path(QualifierBinding *binding)
{
    unsigned char *destination = expression_storage + 2048;
    char *begin, *end;
    int remaining = (int)((expression_storage + 2047) - text_work_cursor);
    *destination = 0;
    while (binding) {
        begin = binding->target->name;
        end = begin;
        while (*end) ++end;
        remaining -= (int)(end - begin);
        if (remaining <= 0) fatal_error(5);
        while (end != begin) *--destination = (unsigned char)*--end;
        binding = binding->next;
        if (binding) {
            *--destination = ':';
            *--destination = ':';
        }
    }
    return (char *)destination;
}

/* Original0x50ea0..0x50ec7, stdcall8. A declaration's shared
   diagnostic bit suppresses repeated reports for the same symbol. */
void __stdcall report_symbol_declaration_error(unsigned code, CppSymbol *symbol)
{
    if (!(symbol->declaration_flags & 2)) {
        report_syntax_error(code, format_symbol_name(symbol));
        symbol->declaration_flags |= 2;
    }
}
