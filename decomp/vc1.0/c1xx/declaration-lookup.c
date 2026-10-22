/* C++ declaration lookup and scope selection. */
#include "lexer.h"
#include "class-lookup.h"
#include "cpp-context.h"
#include "global-ir.h"
#include "parameters.h"

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
