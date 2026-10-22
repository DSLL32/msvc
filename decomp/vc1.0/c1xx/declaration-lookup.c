/* C++ declaration lookup and scope selection. */
#include "declarations.h"
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

/* Overload sets share the ordinary symbol prefix through the name slot.
   Their final word holds functions rather than a CType pointer. */
typedef struct OverloadSet {
    unsigned char kind, flags;
    unsigned short declaration_flags;
    CppSymbol *next;
    char *name;
    CppSymbol *functions;
} OverloadSet;
typedef char OverloadSetSizeMustBe16[(sizeof(OverloadSet) == 16) ? 1 : -1];

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
