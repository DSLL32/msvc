/* Symbol declaration and redeclaration processing. */
#include "parser.h"
#include "global-ir.h"
#include "pragma.h"

int default_thread_storage; /* original 0x6bc44 */

/* Original 0x339f0..0x33cdb. */
FrontendSymbol *__stdcall declare_symbol(DeclarationSpec *declaration, FrontendSymbol *symbol)
{
    unsigned char storage[3];
    unsigned kind;
    int member;
    FrontendSymbol *existing, *tag;
    if (!symbol || !declaration->type) return 0;
    storage[0] = declaration->storage[0];
    storage[1] = declaration->storage[1];
    storage[2] = declaration->storage[2];
    if ((storage[0] & 7) == 1) storage[1] |= 0x10;
    kind = symbol->kind & 0x1f;
    member = kind == 2 || kind == 3;
    if (kind == 1) symbol->kind = 9;
    symbol->storage[0] = storage[0];
    symbol->storage[1] = storage[1];
    symbol->storage[2] = storage[2];
    finalize_symbol_type(declaration->type, symbol);
    if ((declaration->storage[0] & 0x30) == 0x30) {
        if ((symbol->kind & 0x1f) != 7 || function_result_type(symbol)->related)
            report_syntax_error(501, symbol->name);
    }
    if (symbol->storage[2] & 1) {
        if ((symbol->storage[1] & 1) && !(symbol->flags & 0x80))
            compiler_warning(1, 271, symbol->name);
        else if (member || (symbol->kind & 0x1f) != 7)
            report_symbol_error(488, symbol);
    } else if (parameter_nesting == 2 && function_symbol && (function_symbol->storage[2] & 1)) {
        kind = symbol->storage[0] & 7;
        if (kind >= 4 && kind <= 6 && (symbol->attributes & 4))
            report_syntax_error(489, symbol->name);
    }
    kind = symbol->storage[0] & 7;
    if (default_thread_storage && (symbol->kind & 0x1f) == 9 && kind >= 1 && kind <= 3)
        symbol->storage[2] |= 2;
    if (symbol->storage[2] & 2) {
        kind = symbol->storage[0] & 7;
        if ((symbol->kind & 0x1f) != 9 || kind < 1 || kind > 3) {
            report_syntax_error(480, symbol->name);
            symbol->storage[2] &= ~2;
        }
        if (symbol->storage[2] & 4) report_syntax_error(492, symbol->name);
    }
    if ((symbol->storage[0] & 7) == 7) {
        symbol->flags |= 4;
        if (symbol->name) SYMBOL_FROM_NAME(symbol->name)->flags |= 2;
        if (!options.prototypes_only && is_record_type(symbol->type)) {
            tag = (FrontendSymbol *)symbol->type->field08;
            if (tag->name == name_unnamed) tag->name = symbol->name;
        }
    }
    if (member) return symbol;
    existing = merge_global_declaration(symbol);
    if (!existing) {
        if (parameter_nesting >= 2) existing = merge_local_declaration(symbol);
        else if (!options.extensions_enabled && (current_scope->flags & 0xff) == 1) {
            existing = find_identifier_symbol(symbol->name);
            if (existing && (existing->flags & 4)) compiler_warning(1, 224, symbol->name);
            existing = 0;
        }
    }
    if (existing) {
        if (symbol->attributes & 0x80) existing->attributes |= 0x80;
        return existing;
    }
    if (!symbol->index) {
        if (next_symbol_index >= 0x7fffffff) fatal_error(55);
        if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_index_overflow();
        symbol->index = next_symbol_index++;
    }
    if (parameter_nesting >= 2) record_local_declaration(symbol);
    insert_current_scope_symbol(symbol);
    return symbol;
}

/* Original 0x34060..0x3460c; storage dispatch tables 0x3460c..0x34648. */
void __stdcall finalize_symbol_type(CType *base, FrontendSymbol *symbol)
{
    unsigned char storage[3];
    unsigned kind, storage_kind;
    unsigned short attributes;
    int void_declaration = 0;
    CType *description = symbol->type;
    TypeShape *shape;
    FrontendSymbol *tag, *based;
    char *name = symbol->name;
    storage[0] = symbol->storage[0];
    storage[1] = symbol->storage[1];
    storage[2] = symbol->storage[2];
    kind = symbol->kind & 0x1f;
    storage_kind = storage[0] & 7;
    if (kind == 2 || kind == 3) {
        if ((storage_kind >= 4 && storage_kind <= 7) || storage_kind == 3 ||
            storage_kind == 1 || (storage[2] & 4)) {
            report_syntax_error(71, symbol->name);
            storage[0] |= 8;
            symbol->flags |= 0x80;
        }
    }
    if (base) {
        description->field08 = base->field08;
        description->flags = base->flags;
        merge_declarator_type(symbol, base, storage_kind == 7);
        shape = symbol->type->related;
        if (shape && !(shape->flags & 7)) shape = shape->related;
        if (!shape && storage_kind != 7) {
            description = symbol->type;
            kind = symbol->kind & 0x1f;
            if (description->flags == 0x80) {
                if (kind != 12 && kind != 11) {
                    report_syntax_error(182, name);
                    symbol->type->flags = 0x64;
                }
                void_declaration = 1;
            } else if (storage_kind != 1 && (description->flags & 0x400)) {
                tag = (FrontendSymbol *)description->field08;
                if (!(tag->flags & 1) && kind != 12 && kind != 11) {
                    report_syntax_error(79, name, tag->name);
                    symbol->type = type_0064;
                }
            }
        }
    }
    description = symbol->type;
    attributes = description->attributes;
    if (symbol->flags & 0x80) storage[0] |= 8;
    if ((symbol->kind & 0x1f) == 7) {
        shape = description->related;
        if (shape && !(shape->flags & 7)) shape = shape->related;
        attributes = (attributes & 0xf488) | (shape->attributes & 0xb77);
        switch (storage[0] & 7) {
        case 0:
        case 2:
            storage[0] = (storage[0] & ~7) | 1 | 0x20;
            break;
        case 3:
            if (storage[2] & 4) {
                report_syntax_error(201, name);
                storage[0] = (storage[0] & ~7) | 1 | 0x20;
            }
            if (parameter_nesting) {
                report_declaration_context_error(205, 201, name);
                storage[0] = (storage[0] & ~7) | 1 | 0x20;
            }
            break;
        case 4:
        case 5:
        case 6:
            compiler_warning(1, 42, name);
            storage[0] = (storage[0] & ~7) | 1 | 0x20;
            break;
        case 7:
            if (symbol->flags & 1) {
                report_syntax_error(71, name);
                storage[0] = (storage[0] & ~7) | 1;
            }
            break;
        }
        goto type_ready;
    }
    if (storage[1] & 1) {
        storage[1] &= ~1;
        kind = symbol->kind & 0x1f;
        if (kind == 11) report_syntax_error(735, "__inline");
        else if (kind == 12) report_syntax_error(736, "__inline");
        else report_syntax_error(433, symbol->name, "__inline");
    }
    if (!options.prototypes_only && !void_declaration && !(attributes & 0x70))
        attributes |= options.option_Gz ? 0x60 : (options.option_Gc ? 0x20 : 0x10);
    else if ((attributes & 0x70) == 0x40) {
        compiler_warning(2, 8, symbol->name, "_fastcall");
        attributes = (attributes & ~0x70) | 0x10;
    }
    if (attributes & 0xb00) {
        compiler_warning(1, 105, name);
        attributes &= ~0xb00;
    }
    if ((storage[0] & 7) == 5 && description->related && (description->related->attributes & 8)) {
        compiler_warning(1, 42, name);
        storage[0] &= ~7;
    }
    kind = symbol->kind & 0x1f;
    storage_kind = storage[0] & 7;
    if (kind == 10 || kind == 11) {
        if (storage_kind != 5) {
            if (storage_kind) compiler_warning(1, 42, name);
            storage[0] = (storage[0] & ~7) | 4;
        }
        if ((storage[1] & 7) || (storage[2] & 7)) {
            compiler_warning(1, 42, name);
            storage_kind = storage[0] & 7;
            storage[0] = (current_storage[0] & ~7) | storage_kind;
            storage[1] = current_storage[1];
            storage[2] = current_storage[2];
        }
    } else if (kind == 9) {
        if (!(scanner_flags & 0x80000)) {
            if (storage_kind >= 4 && storage_kind <= 6) {
                compiler_warning(1, 42, name);
                storage[0] = (storage[0] & ~7) | 2;
            } else if (!storage_kind) {
                if ((symbol->attributes & 0x400) && (symbol->attributes & 0x100) &&
                    !(symbol->attributes & 4)) storage[0] = (storage[0] & ~7) | 1;
                else storage[0] = (storage[0] & ~7) | 2;
            }
        } else if (!storage_kind) {
            if (scanner_flags & 0x40) storage[0] = (storage[0] & ~7) | 2;
            else if ((storage[2] & 4) && !(storage[2] & 8)) storage[0] = (storage[0] & ~7) | 1;
            else storage[0] = (storage[0] & ~7) | 4;
        }
    }
    switch (storage[0] & 7) {
    case 3:
        if (storage[2] & 4) {
            report_syntax_error(201, name);
            storage[2] &= ~12;
            storage[0] |= 8;
        }
        /* Static, external, and global objects share based-address checks. */
    case 1:
    case 2:
        shape = description->related;
        if (!shape || (shape->flags & 7)) break;
        attributes = (attributes & ~7) | 2;
        if ((shape->attributes & 7) >= 5) {
            based = (FrontendSymbol *)shape->field08;
            if ((based->kind & 0x1f) == 13) {
                if (based->index != 6) break;
                report_syntax_error(185, name);
            } else {
                if ((storage[0] & 7) == 1 && !(symbol->attributes & 4)) break;
                report_syntax_error(154, based->name);
            }
        } else report_syntax_error(185, name);
        /* An invalid based address is normalized below after its diagnostic. */
    case 0:
    case 4:
    case 5:
    case 6:
        if ((symbol->kind & 0x1f) != 12) attributes = normalize_symbol_storage(attributes, symbol);
        break;
    }
    if ((storage[2] & 4) && (scanner_flags & 0x80000) && (storage[0] & 7) != 1)
        report_syntax_error(71, symbol->name);
type_ready:
    if ((storage[0] & 7) != 7) description->attributes = attributes;
    symbol->storage[0] = storage[0];
    symbol->storage[1] = storage[1];
    symbol->storage[2] = storage[2];
    symbol->type = intern_type(description);
}

AggregateContext aggregate_contexts[16]; /* original 0x600a8 */
typedef char AggregateContext_must_be_24_bytes[(sizeof(AggregateContext) == 24) ? 1 : -1];
typedef char ShapeCursor_must_be_eight_bytes[(sizeof(ShapeCursor) == 8) ? 1 : -1];

/* Original 0x31520..0x31ad4. Fold the declarator's mutable shape chain
   into the base type from the inside out, retaining prefix qualifiers. */
void __stdcall merge_declarator_type(FrontendSymbol *symbol, CType *base, int is_typedef)
{
    TypeShape *head = symbol->type->related;
    TypeShape *work = head, *prefix = 0, *base_shape = base->related;
    TypeShape *next, *extra, *result;
    unsigned short qualifiers = symbol->type->attributes & 0x3000, pointer_attributes;
    ShapeCursor current, outer, inner, prepared_base;
    unsigned category;
    FrontendSymbol *tag;
    char *convention;
    prepared_base.shape = base_shape;
    if (!prepare_declarator_shapes(symbol, base, &work, &prepared_base, &prefix, &head, is_typedef))
        return;
    base_shape = prepared_base.shape;
    if (!work) {
        work = base_shape;
        current.flags = work ? work->flags : 0;
        current.attributes = work ? work->attributes : 0;
    } else {
        for (;;) {
            next = work->related;
            if (next) {
                outer.shape = next;
                outer.flags = next->flags;
                outer.attributes = next->attributes;
                next->attributes &= ~0x3000;
            } else {
                outer.shape = 0;
                outer.flags = 0;
                outer.attributes = symbol->type->attributes;
                symbol->type->attributes &= ~0x3000;
                symbol->type->attributes |= base->attributes & 0x3000;
            }
            inner.shape = base_shape;
            inner.flags = base_shape ? base_shape->flags : 0;
            inner.attributes = base_shape ? base_shape->attributes : 0;
            current.flags = work->flags;
            current.attributes = work->attributes;
            extra = 0;
            switch (current.flags & 7) {
            case 1:
                if ((inner.flags & 7) == 4) {
                    current.flags = (current.flags & ~4) | 3;
                    current.flags |= inner.flags & 0xc800;
                    if ((current.attributes & 0x70) == 0x60 && (current.flags & 0x8000))
                        current.attributes &= ~0x70;
                    current.attributes = merge_type_attributes(current.attributes, inner.attributes);
                } else {
                    convention = 0;
                    switch (current.attributes & 0x70) {
                    case 0x10: convention = "cdecl"; break;
                    case 0x20: convention = "pascal"; break;
                    case 0x40: convention = "_fastcall"; break;
                    case 0x60: convention = "_stdcall"; break;
                    }
                    if (convention) report_syntax_error(165, convention);
                    if (current.attributes & 0xf00) {
                        compiler_warning(1, 105, symbol->name);
                        current.attributes &= ~0xf00;
                    }
                    if ((current.attributes & 7) == 2) {
                        report_syntax_error(165, "_far");
                        current.attributes &= ~7;
                    }
                }
                current.attributes = (current.attributes & ~0x3000) | (outer.attributes & 0x3000);
                break;
            case 2:
                if ((inner.flags & 7) == 4) {
                    report_syntax_error(92);
                    extra = new_type_shape(3, 0, 0);
                }
                current.attributes = normalize_type_attributes(2,
                    (unsigned short)((symbol->type->attributes & 7) | (inner.attributes & 0x3000)));
                if (!inner.shape && (symbol->type->flags & 0x400)) {
                    tag = (FrontendSymbol *)symbol->type->field08;
                    if (tag->attributes & 0x100) report_syntax_error(233, symbol->name);
                }
                if ((outer.flags & 7) == 2 && !work->field08)
                    report_syntax_error(87, symbol->name);
                break;
            case 4:
                current.shape = work;
                extra = merge_function_shape(symbol, inner, &current, outer, is_typedef);
                work = current.shape;
                break;
            }
            if (extra) {
                extra->related = base_shape;
                base_shape = intern_type_shape(extra);
            }
            category = current.flags & 7;
            if (next || !is_typedef || (category != 4 && category != 2))
                current.attributes = normalize_type_attributes(current.flags, current.attributes);
            if (!next) {
                if (category == 4) current.attributes &= ~0x3000;
                else current.attributes |= qualifiers;
            }
            work->flags = current.flags;
            work->attributes = current.attributes;
            work->related = base_shape;
            work = intern_type_shape(work);
            if (!next) break;
            base_shape = work;
            work = next;
        }
    }
    current.shape = work;
    if (prefix) {
        if (!is_typedef) check_declarator_prefix(symbol, prefix);
        prefix->related = current.shape;
        current.shape = intern_type_shape(prefix);
    }
    result = current.shape;
    if ((symbol->attributes & 2) && current.shape->related) {
        current.shape = current.shape->related;
        current.flags = current.shape->flags;
        current.attributes = current.shape->attributes;
    }
    category = current.flags & 7;
    if (category == 4) {
        if ((symbol->kind & 0x1f) == 9) symbol->kind = 7;
        else {
            if (symbol->kind >= 2 && symbol->kind <= 3) {
                tag = aggregate_contexts[aggregate_depth].tag;
                report_syntax_error(32, symbol->name, type_specifier_name(tag->type->flags), tag->name);
            } else if (symbol->kind == 12) report_syntax_error(66);
            pointer_attributes = normalize_type_attributes(3,
                (unsigned short)(symbol->type->attributes & 15));
            symbol->type->attributes &= ~7;
            extra = new_type_shape(3, pointer_attributes, 0);
            extra->related = result;
            result = intern_type_shape(extra);
        }
    } else if (category == 2) finish_array_declarator(symbol, &result, current, is_typedef);
    if (symbol->type->flags == 0x80 && (head->flags & 7) == 2)
        report_syntax_error(182, symbol->name);
    symbol->type->related = result;
}

/* Original 0x32090..0x323a6. Separate prefix qualifiers from the two
   shape chains and reconcile their boundary before the folding pass. */
int __stdcall prepare_declarator_shapes(FrontendSymbol *symbol, CType *base,
    TypeShape **work, ShapeCursor *base_shape, TypeShape **prefix, TypeShape **tail, int is_typedef)
{
    TypeShape *node, *copy, *detached = 0;
    unsigned short *attributes;
    ArenaPool *pool;
    if (!base_shape->shape) {
        if (!*work) {
            symbol->type->attributes = merge_type_attributes(symbol->type->attributes, base->attributes);
            return 0;
        }
        node = extract_declarator_prefix(*work, prefix, symbol, is_typedef);
        *work = symbol->type->related;
        if (!node) symbol->type->attributes = merge_type_attributes(symbol->type->attributes, base->attributes);
        else node->attributes = merge_type_attributes(node->attributes, (unsigned short)(base->attributes & 0xcfff));
        *work = symbol->type->related;
        return 1;
    }
    if ((base_shape->shape->flags & 7) == 2 && !base_shape->shape->field08 &&
        *work && (((*work)->flags & 7) == 2)) {
        report_syntax_error(87, symbol->name);
        symbol->flags |= 0x80;
    }
    if ((base_shape->shape->flags & 7) == 4) {
        node = symbol->type->related;
        base_shape->attributes = merge_type_attributes(
            (unsigned short)(node ? node->attributes : symbol->type->attributes), base_shape->shape->attributes);
    } else base_shape->attributes = base_shape->shape->attributes;
    base_shape->attributes = normalize_type_attributes(base_shape->shape->flags, base_shape->attributes);
    if (base_shape->shape->attributes != base_shape->attributes) {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(TypeShape)) refill_arena(&compiler_arena, pool);
        copy = (TypeShape *)pool->next;
        pool->remaining -= sizeof(TypeShape);
        pool->next += sizeof(TypeShape);
        *copy = *base_shape->shape;
        copy->attributes = base_shape->attributes;
        base_shape->shape = intern_type_shape(copy);
    }
    *tail = base_shape->shape;
    while ((*tail)->related) *tail = (*tail)->related;
    if (!*work) {
        base_shape->shape = extract_base_prefix(base_shape->shape, prefix, symbol,
            is_typedef, &symbol->type->attributes);
        symbol->type->attributes = merge_type_attributes(symbol->type->attributes, base->attributes);
        return 1;
    }
    node = extract_declarator_prefix(*work, prefix, symbol, is_typedef);
    *work = symbol->type->related;
    if (!(base_shape->shape->flags & 7) && prefix && *prefix)
        append_type_shape(*prefix, base_shape->shape);
    attributes = node ? &node->attributes : &symbol->type->attributes;
    base_shape->shape = extract_base_prefix(base_shape->shape, &detached, symbol, is_typedef, attributes);
    if (node) {
        node->attributes = merge_type_attributes(node->attributes, (unsigned short)(base->attributes & 0xcfff));
        if (detached) append_type_shape(node, detached);
    } else {
        symbol->type->attributes = merge_type_attributes(symbol->type->attributes, base->attributes);
        if (!*prefix && detached)
            *prefix = new_type_shape(detached->flags, detached->attributes, &detached->field08);
    }
    return 1;
}

/* Original 0x31c00..0x31c72. */
TypeShape *__stdcall extract_declarator_prefix(TypeShape *shape, TypeShape **prefix,
    FrontendSymbol *symbol, int is_typedef)
{
    TypeShape *previous = 0;
    if (!shape) return 0;
    while (shape && (((shape->flags & 7) == 4) || ((shape->flags & 7) == 2))) {
        previous = shape;
        shape = shape->related;
    }
    if (shape && !(shape->flags & 7)) {
        copy_shape_prefix(symbol, shape, prefix, is_typedef);
        if (previous) previous->related = 0;
        else symbol->type->related = 0;
        return previous;
    }
    return shape;
}

/* Original 0x31e70..0x32089. Validate a function's result and combine
   attributes from its surrounding declarator before interning the shape. */
TypeShape *__stdcall merge_function_shape(FrontendSymbol *symbol, ShapeCursor base,
    ShapeCursor *current, ShapeCursor next, int is_typedef)
{
    TypeShape *extra = 0;
    char *convention = 0;
    switch (base.flags & 7) {
    case 2:
        report_syntax_error(90);
        extra = new_type_shape(1, 0, 0);
        break;
    case 4:
        report_syntax_error(91);
        extra = new_type_shape(3, 0, 0);
        break;
    case 1:
        if (base.attributes & 8) {
            report_syntax_error(263);
            extra = new_type_shape(1, 0, 0);
        }
        break;
    }
    if ((next.flags & 7) == 1) {
        current->attributes = merge_type_attributes(current->attributes,
            (unsigned short)(next.attributes & 0xf77));
        if (next.attributes & 12) next.shape->attributes &= ~15;
    } else if (!next.shape) {
        current->attributes = merge_type_attributes(current->attributes, next.attributes);
        if (!base.shape) current->attributes &= ~0x1000;
        if (symbol->flags & 1) {
            if (function_parameter_context == 1) current->flags |= 0x4000;
            if (is_typedef)
                current->attributes = normalize_type_attributes(current->flags, current->attributes);
        }
    }
    if (!(current->attributes & 0x70) && !(current->flags & 0x2000)) {
        if ((options.option_Gr || options.option_Gc || options.option_Gz) && symbol->name == name_main) {
            current->attributes = (current->attributes & ~0x70) | 0x10;
            convention = "cdecl";
        } else if ((options.option_Gr || options.option_Gc || options.option_Gd) &&
            (symbol->name == name_winmain || symbol->name == name_dllmain)) {
            current->attributes = (current->attributes & ~0x70) | 0x60;
            convention = "_stdcall";
        }
        if (convention) compiler_warning(3, 7, symbol->name, convention);
    }
    if (current->attributes & 12) {
        report_syntax_error(212, symbol->name);
        current->attributes &= ~15;
        if (next.shape && !(next.flags & 7)) {
            next.shape = 0;
            symbol->attributes &= ~2;
        }
    }
    if ((current->attributes & 0x70) == 0x60 && (current->flags & 0x8000))
        current->attributes &= ~0x70;
    return extra;
}

/* Original 0x34690..0x346a3. */
FrontendSymbol *__stdcall merge_global_declaration(FrontendSymbol *symbol)
{
    return merge_scoped_declaration(symbol, current_scope);
}

/* Original 0x346b0..0x346e6. */
FrontendSymbol *__stdcall merge_scoped_declaration(FrontendSymbol *symbol, SymbolScope *scope)
{
    FrontendSymbol *existing = find_scope_declaration(scope, symbol->name,
        (symbol->type->attributes & 0x70) == 0x20);
    return reconcile_declarations(existing, symbol);
}

/* Original 0x346f0..0x347c4. */
FrontendSymbol *__stdcall reconcile_declarations(FrontendSymbol *existing, FrontendSymbol *symbol)
{
    if (!existing) return 0;
    if (options.extensions_enabled && (existing->storage[0] & 7) == 7 &&
        (symbol->storage[0] & 7) == 7 && existing->type == symbol->type) {
        compiler_warning(4, 209);
        return existing;
    }
    if (!parameter_nesting) {
        if (declaration_conflict(existing, symbol)) {
            symbol->flags |= 0x80;
            return symbol;
        }
        commit_redeclaration(existing, symbol);
        return existing;
    }
    if ((existing->storage[0] & 7) == 1 && (symbol->storage[0] & 7) == 1) {
        if (!declaration_conflict(existing, symbol)) return existing;
    } else if (!declaration_conflict(existing, symbol)) report_syntax_error(86, symbol->name);
    symbol->flags |= 0x80;
    return symbol;
}

typedef char FrontendSymbol_must_be_forty_four_bytes[(sizeof(FrontendSymbol) == 44) ? 1 : -1];
typedef char CType_must_be_sixteen_bytes[(sizeof(CType) == 16) ? 1 : -1];

/* Original 0x2c600..0x2c684. Return an expression leaf for a new label,
   retaining its symbol as the payload consumed by the IR writer. */
IrToken *__stdcall make_internal_label(unsigned flags)
{
    FrontendSymbol *symbol;
    IrToken *node;
    ArenaPool *pool = &compiler_arena.pools[1];
    if (pool->remaining < (int)sizeof(FrontendSymbol)) refill_arena(&compiler_arena, pool);
    symbol = (FrontendSymbol *)pool->next;
    pool->remaining -= sizeof(FrontendSymbol);
    pool->next += sizeof(FrontendSymbol);
    symbol->kind = 8;
    symbol->detail.value = flags;
    symbol->type = type_0064;
    reference_label(symbol);
    if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
    node = (IrToken *)pool->next;
    pool->remaining -= sizeof(IrToken);
    pool->next += sizeof(IrToken);
    node->flags = 4;
    node->kind = 2;
    node->value.pointer = symbol;
    node->type = symbol->type;
    return node;
}

FrontendSymbol *label_symbols; /* original 0x6c0c0 */

/* Original 0x2c4a0..0x2c4ff. */
void __stdcall reference_label(FrontendSymbol *label)
{
    if (!label->index) {
        if (next_symbol_index >= 0x7fffffff) fatal_error(55);
        if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_index_overflow();
        label->index = next_symbol_index++;
    }
    label->next = label_symbols;
    label_symbols = label;
}

/* Original 0x2a830..0x2a87b. Canonicalize the result of a function type. */
CType *__stdcall function_result_type(FrontendSymbol *symbol)
{
    TypeDescription *description = copy_type_description(
        drop_first_type_shape(remove_type_prefix(symbol->type)));
    unsigned short attributes = description->attributes & ~7;
    CType *result;
    description->attributes = attributes;
    description->attributes = normalize_type_attributes(1, 0) | attributes;
    result = intern_type(description);
    --type_description_depth;
    return result;
}

/* Original 0x2c690..0x2c70c. Diagnose unresolved/unused named labels and
   serialize every label defined in the function. */
void finish_label_symbols(void)
{
    FrontendSymbol *label;
    if (!error_count) {
        for (label = label_symbols; label; label = label->next) {
            if (!(label->flags & 1) && !label->detail.value)
                report_syntax_error(94, label->name);
            else {
                if (!label->references && label->name) compiler_warning(3, 102, label->name);
                if (label->flags & 1) write_global_record(3, label);
            }
        }
    }
    label_symbols = 0;
}

FrontendSymbol *pending_global_symbols; /* original 0x6b6bc */

/* Original 0x362c0..0x3633c. Emit retained functions and global declarations,
   followed by any deferred initializer expressions and final pragma state. */
void finish_declarations(void)
{
    FrontendSymbol *symbol, *target;
    IrToken *initializer;
    global_record_kind = 0;
    if (error_count) return;
    for (symbol = pending_global_symbols; symbol; symbol = symbol->next)
        emit_symbol_declaration(symbol);
    write_function_directory();
    serialize_scope(global_scope);
    for (symbol = pending_global_symbols; symbol; symbol = symbol->next) {
        if (symbol->associated.initializer) {
            emit_symbol_declaration(symbol->associated.initializer);
            initializer = make_symbol_reference(symbol);
            target = prepare_initializer_symbol(symbol->associated.initializer);
            finish_initializer(target, initializer);
        }
    }
    write_global_record(10, 0);
}

/* Original 0x36b50..0x36df7. Emit each declaration once, separating
   ordinary records from declarations retained in precompiled debug data. */
void __stdcall emit_symbol_declaration(FrontendSymbol *symbol)
{
    unsigned kind = symbol->kind & 31;
    unsigned storage;
    int defer_debug = 0, imported_debug, tag;
    FrontendSymbol *alias;

    if (pch_separate_debug && !global_record_kind &&
        !(kind >= 1 && kind <= 13 && kind != 5 && (symbol->attributes & 8)))
        defer_debug = 1;
    imported_debug = (symbol->flags & 0x40) &&
        (options.pch_use || options.pch_use_file) &&
        !options.pch_create && !options.pch_create_file;
    if (error_count > 0 || (symbol->flags & 2)) return;
    if (kind == 13) {
        if (!defer_debug) {
            write_global_record(9, symbol);
            symbol->flags |= 2;
        }
        return;
    }
    if (kind == 4) {
        if (options.debug_info && !(symbol->flags & 0x20)) {
            symbol->type->field0c = (symbol->type->field0c & 0xffff0000UL) |
                                    debug_symbol_type_index(symbol);
            flush_debug_types();
        }
        if (defer_debug) { symbol->flags |= 0x40; return; }
        if (!symbol->name || symbol->name == name_unnamed) return;
        if (options.debug_info && !(symbol->flags & 0x20) && !imported_debug)
            tag = 11;
        else { symbol->flags |= 2; return; }
    } else if (kind == 5) {
        if (!options.debug_info) { symbol->flags |= 2; return; }
        emit_debug_type(symbol->type);
        flush_debug_types();
        if (defer_debug) { symbol->flags |= 0x40; return; }
        if (imported_debug) { symbol->flags |= 2; return; }
        tag = 8;
    } else {
        storage = symbol->storage[0] & 7;
        if (defer_debug) {
            prepare_symbol_debug_type(symbol);
            if (storage == 7) symbol->flags |= 0x40;
            return;
        }
        if (storage == 7 && (symbol->flags & 0x40)) {
            if (!imported_debug) write_global_record(11, symbol);
            symbol->flags |= 2;
            return;
        }
        if ((symbol->attributes & 0x40) && kind == 9) {
            alias = symbol_alias(symbol);
            alias->attributes = (alias->attributes & ~0x8004UL) |
                                (symbol->attributes & 0x8004);
            alias->detail.value = symbol->detail.value;
            alias->attributes = (alias->attributes & ~2UL) | (symbol->attributes & 2);
            alias->type = symbol->type;
            if (!options.debug_info) return;
            ++alias->references;
            emit_debug_type(symbol->type);
            flush_debug_types();
            tag = 7;
        } else {
            if (!symbol->references) {
                if (storage == 1 || kind == 6 || kind == 7 || storage == 7) return;
                if (kind == 10) compiler_warning(4, 100, symbol->name);
                else if (global_record_kind) compiler_warning(3, 101, symbol->name);
            }
            tag = (kind == 6 || kind == 7) ? 4 : 1;
            if (options.debug_info) prepare_symbol_debug_type(symbol);
        }
    }
    write_global_record(tag, symbol);
    symbol->flags |= 2;
}

/* Original 0x1ac90..0x1ad79; modifier table 0x1ad7c..0x1adac.
   The fourth ABI argument is retained although this function does not use it. */
void __stdcall merge_storage_class(unsigned char *storage, unsigned class,
                                  unsigned modifier, int replace)
{
    unsigned duplicate = 1;
    if (!(storage[0] & 7)) storage[0] = (storage[0] & ~7) | ((unsigned char)class & 7);
    else if ((unsigned char)class) report_syntax_error(159);
    switch ((unsigned char)modifier) {
    case 3: case 14: /* inline */
        if (storage[1] & 1) duplicate = 14;
        else storage[1] |= 1;
        break;
    case 4: /* naked */
        if (storage[2] & 1) duplicate = 2;
        else storage[2] |= 1;
        break;
    case 5: /* thread */
        if (storage[2] & 2) duplicate = 2;
        else storage[2] |= 2;
        break;
    case 6: /* dllimport */
        if (storage[2] & 4) duplicate = 2;
        else storage[2] = (storage[2] | 4) & ~8;
        break;
    case 7: /* dllexport */
        if (storage[2] & 4) duplicate = 2;
        else storage[2] |= 4 | 8;
        break;
    }
    if (duplicate != 1)
        compiler_warning(1, 141, duplicate == 2 ? interned_name : token_names[duplicate]);
}

/* Original 0x18d90..0x18e97. A name followed by an opening brace starts
   a tag definition; other named uses resolve or introduce a tag reference. */
unsigned char __stdcall read_tag_specifier(unsigned short kind, unsigned short *attributes)
{
    unsigned char token = scan_token();
    char *name;
    FrontendSymbol *tag;
    if (!token) fatal_error(4);
    else if (token == TOK_IDENTIFIER) {
        name = current_token_value.identifier;
        token = scan_token();
        unget_token(token);
        if (token != TOK_LEFT_BRACE) {
            current_token_value.symbol = lookup_or_declare_tag(name, kind, insert_current_scope_symbol);
            return TOK_TYPE;
        }
    } else if (token == TOK_LEFT_BRACE) {
        unget_token(token);
        name = 0;
    } else {
        report_syntax_error(237, type_specifier_name(kind), token_names[token]);
        unget_token(token);
        current_token_value.symbol = 0;
        return TOK_NEWLINE;
    }
    tag = begin_tag_definition(name, kind, *attributes);
    current_token_value.symbol = tag;
    if (tag->flags & 0x80) return TOK_NEWLINE;
    if (kind == 0x200) {
        scanner_flags |= 1;
        return TOK_ENUM_TAG;
    }
    return TOK_CLASS_HEAD;
}

FrontendSymbol *current_enum_tag; /* original 0x60830 */

/* Original 0x28650..0x2885c. Bind the tag, construct its incomplete
   definition type, then enter enum or aggregate layout state. */
FrontendSymbol *__stdcall begin_tag_definition(char *name, unsigned short kind,
                                              unsigned short attributes)
{
    FrontendSymbol *tag = 0, *head;
    TypeDescription *description;
    CType *type;
    AggregateContext *context;
    if (name) {
        head = current_scope->buckets ?
            current_scope->buckets[(unsigned)name & current_scope->bucket_mask] : 0;
        if (head) tag = lookup_tag_chain(name, head);
        if (tag && (tag->flags & 1)) {
            report_syntax_error(11, name, type_specifier_name(tag->type->flags));
            tag->flags |= 0x80;
            return tag;
        }
    }
    if (!tag) {
        tag = create_tag(name, kind, insert_current_scope_symbol, 1);
        if (!tag) return 0;
    }
    tag = check_tag_kind(tag, kind);
    if (tag->flags & 0x80) return tag;
    description = copy_type_description(tag->type);
    if (browser_filename && (!parameter_nesting || options.option_FR) && tag->name != name_unnamed) {
        browser_begin_function_scope();
        browser_symbol_definition(tag);
        browser_function_parameters(tag);
    }
    description->flags |= 0x10;
    description->attributes = attributes;
    description->field08 = (unsigned)tag;
    type = intern_type(description);
    --type_description_depth;
    if (((scanner_flags & 0x08000000) && !(scanner_flags & 0x40)) || aggregate_depth >= 0)
        tag->metadata.value |= 0x02000000;
    else tag->metadata.value &= ~0x02000000;
    if (kind == 0x200) {
        enumerator_value = (unsigned)-1;
        tag->detail.value = 4;
        tag->flags |= 1;
        enumerator_context = type;
        current_enum_tag = tag;
        return tag;
    }
    if (++aggregate_depth == 16) fatal_error(46, type_specifier_name(type->flags));
    context = &aggregate_contexts[aggregate_depth];
    if ((kind & 0x408) == 0x408 || (kind & 0x402) == 0x402) context->flags |= 1;
    else context->flags &= ~1;
    context->tag = tag;
    context->last_member = 0;
    context->size = 0;
    context->bitfield_used_bits = 0;
    context->bitfield_type = 0;
    context->type = type;
    context->alignment = 1;
    return tag;
}

int tag_in_current_scope; /* original 0x604a4 */

/* Original 0x28860..0x289bc. Allocate the tag's permanent identity and
   intern its incomplete type before publishing it in the selected scope. */
FrontendSymbol *__stdcall create_tag(char *name, unsigned short kind,
    void (__stdcall *insert)(FrontendSymbol *), int definition)
{
    FrontendSymbol *tag = allocate_frontend_symbol(0);
    TypeDescription *description;
    ArenaPool *pool = &compiler_arena.pools[2];
    tag->kind = 4;
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_index_overflow();
    tag->index = next_symbol_index++;
    if (pool->remaining < (int)sizeof(TypeDescription)) refill_arena(&compiler_arena, pool);
    description = (TypeDescription *)pool->next;
    pool->remaining -= sizeof(TypeDescription);
    pool->next += sizeof(TypeDescription);
    description->flags = kind;
    if (kind & 0x200) {
        description->flags |= 4;
        tag->associated.enumerators = new_hash_table(4, (short)((scanner_flags & 0x08000000) != 0), 0, 0);
    }
    description->field08 = (unsigned)tag;
    tag->type = intern_type(description);
    if (name) {
        tag->name = name;
        if (declaration_depth > 0) {
            compiler_warning(options.extensions_enabled ? 4 : 1, 115, name);
            if (!definition && options.extensions_enabled) {
                insert_scope_symbol(enclosing_declaration_scope(), tag);
                goto published;
            }
        }
    } else {
        if (declaration_depth > 0) compiler_warning(1, 116);
        tag->name = name_unnamed;
    }
    (*insert)(tag);
published:
    if (browser_filename && (!parameter_nesting || options.option_FR)) browser_symbol_definition(tag);
    tag_in_current_scope = 1;
    return tag;
}

/* Original 0x28c30..0x28c91. Storage is zeroed by the selected arena;
   bit1 distinguishes ordinary symbols from captured/separate-debug symbols. */
FrontendSymbol *__stdcall allocate_frontend_symbol(int arena)
{
    FrontendSymbol *symbol;
    ArenaPool *pool;
    if (!arena) symbol = allocate_permanent(sizeof(FrontendSymbol));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(FrontendSymbol)) refill_arena(&compiler_arena, pool);
        symbol = (FrontendSymbol *)pool->next;
        pool->remaining -= sizeof(FrontendSymbol);
        pool->next += sizeof(FrontendSymbol);
    }
    if (!(scanner_flags & 0x08000000) && !pch_separate_debug) symbol->flags |= 2;
    else symbol->flags &= ~2;
    return symbol;
}

/* Original 0x289c0..0x28a07. Enum references accept the completed enum's
   integer-width flags; other tag kinds must match their defining bits. */
FrontendSymbol *__stdcall check_tag_kind(FrontendSymbol *tag, unsigned short kind)
{
    unsigned short flags = tag->type->flags;
    if (((flags ^ kind) & 0x2fcf) && !(kind == 0x200 && (flags & 0x200))) {
        report_syntax_error(11, tag->name, type_specifier_name(flags));
        tag->flags |= 0x80;
    }
    return tag;
}

/* Original 0x18ea0..0x18f87. Merge typedef qualifiers and retain tag
   identity for aggregates; C enum specifiers have the ordinary int type. */
CType *__stdcall merge_named_type(FrontendSymbol *symbol, unsigned short *flags,
                                 unsigned short *attributes)
{
    CType *type = merge_type_specifier(symbol->type, flags, symbol->type->flags);
    TypeShape *shape;
    TypeDescription *description;
    if (!type) return 0;
    shape = type->related;
    if (shape && !(shape->flags & 7)) shape = shape->related;
    if (!shape) *attributes = merge_type_attributes(*attributes, type->attributes);
    else if (*attributes & 0x3000) {
        type = replace_outer_type_shape(type, shape->flags, (unsigned short)(shape->attributes | *attributes), &shape->field08);
        *attributes &= ~0x3000;
    }
    last_lookup_symbol = symbol;
    if ((symbol->kind & 0x1f) == 4) {
        if (type->flags & 0x200) return type_0064;
        description = copy_type_description(type);
        description->flags |= 0x10;
        description->attributes |= *attributes;
        description->field08 = (unsigned)symbol;
        type = intern_type(description);
        --type_description_depth;
    } else if (!symbol->references) symbol->references = 1;
    return type;
}

/* Original 0x29340..0x29368. Enumeration arithmetic wraps at 32 bits,
   but advancing past the signed-int maximum emits a warning. */
unsigned __stdcall new_enumerator(unsigned value, char *name)
{
    if (value == 0x7fffffffUL) compiler_warning(1, 340, name);
    return value + 1;
}

/* Original 0x29370..0x294fa. Enumerator symbols use the common symbol
   prefix followed by their value, occupying 28 bytes rather than 44. */
void __stdcall add_enumerator(char *name, CType *context, unsigned value)
{
    EnumeratorSymbol *enumerator;
    FrontendSymbol *symbol, *existing;
    ArenaPool *pool;
    HashTable *names;
    if (!context) return;
    if ((declaration_depth > 0 && !(scanner_flags & 0x08000000)) || !parameter_nesting)
        enumerator = allocate_permanent(sizeof(EnumeratorSymbol));
    else {
        pool = &compiler_arena.pools[1];
        if (pool->remaining < (int)sizeof(EnumeratorSymbol)) refill_arena(&compiler_arena, pool);
        enumerator = (EnumeratorSymbol *)pool->next;
        pool->remaining -= sizeof(EnumeratorSymbol);
        pool->next += sizeof(EnumeratorSymbol);
    }
    enumerator->kind = 5;
    enumerator->name = name;
    enumerator->type = context;
    if (!(scanner_flags & 0x08000000) && !pch_separate_debug && !(current_enum_tag->flags & 0x20))
        enumerator->flags |= 2;
    else enumerator->flags &= ~2;
    symbol = (FrontendSymbol *)enumerator;
    existing = merge_global_declaration(symbol);
    if (!existing) insert_current_scope_symbol(symbol);
    else {
        report_syntax_error(86, existing->name);
        symbol = existing;
    }
    names = current_enum_tag->associated.enumerators;
    if (!names) return;
    insert_hash_value(names, name, symbol);
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_index_overflow();
    symbol->index = next_symbol_index++;
    if (integer_conversion_overflow(4, 0, 0, value))
        compiler_warning(options.extensions_enabled ? 3 : 1, 341, symbol->name);
    ((EnumeratorSymbol *)symbol)->value = value;
    enumerator_value = value;
    if (browser_filename && (!parameter_nesting || options.option_FR)) browser_symbol_definition(symbol);
}

typedef char EnumeratorSymbol_must_be_28_bytes[(sizeof(EnumeratorSymbol) == 28) ? 1 : -1];

/* Original 0x28a70..0x28add. Finish a C enum definition while retaining
   its tag's browser scope and exposing ordinary int as its declaration type. */
CType *parse_enum_specifier(void)
{
    FrontendSymbol *tag = (FrontendSymbol *)enumerator_context->field08;
    if (!options.extensions_enabled && !hash_table_count(tag->associated.enumerators)) report_syntax_error(138);
    tag = (FrontendSymbol *)enumerator_context->field08;
    if (tag->name != name_unnamed && browser_filename && (!parameter_nesting || options.option_FR))
        browser_finish_scope();
    parser_flags |= 0x4000;
    return type_0064;
}

/* Original 0x33800..0x339e2. Commit a declarator, diagnose definitions of
   imported data, and apply storage handling to nonfunction declarations. */
FrontendSymbol *__stdcall apply_declarator_type(DeclarationSpec *declaration, FrontendSymbol *input)
{
    FrontendSymbol *symbol = declare_symbol(declaration, input);
    unsigned kind, storage;
    int external;
    if (!symbol) return 0;
    kind = symbol->kind & 0x1f;
    if (kind == 4) return symbol;
    if (browser_filename && (!parameter_nesting || options.option_FR)) {
        if (!symbol->index) {
            if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
            if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_index_overflow();
            symbol->index = next_symbol_index++;
        }
        browser_symbol_definition(symbol);
    }
    kind = symbol->kind & 0x1f;
    if (kind == 6 || kind == 7) {
        if (symbol->attributes & 4) {
            report_syntax_error(72, symbol->name);
            symbol->type = type_0064;
        }
        if (symbol->attributes & 0x10) report_syntax_error(136, symbol->name);
        if (declaration->storage[2] & 1) report_syntax_error(488, symbol->name);
        return symbol;
    }
    external = (symbol->storage[0] & 7) == 1 && !(symbol->attributes & 4) &&
               ((symbol->storage[1] & 0x10) ||
                ((symbol->attributes & 0x400) && (symbol->attributes & 0x100)));
    if (!external && !((symbol->storage[2] & 4) && !(symbol->storage[2] & 8))) symbol->flags |= 1;
    if ((symbol->storage[2] & 4) && !(symbol->storage[2] & 8) &&
        ((symbol->flags & 1) || (symbol->attributes & 4))) {
        if (!((scanner_flags & 0x08000000) && (function_symbol->storage[2] & 4) &&
              !(function_symbol->storage[2] & 8) && (function_symbol->storage[1] & 1)))
            report_syntax_error(491, symbol->name, "data");
    }
    storage = symbol->storage[0] & 7;
    if (storage && storage != 7 && storage != 1 && !(symbol->attributes & 4) &&
        (symbol->kind & 0x1f) == 9 && input == symbol) {
        if (effective_type_qualifiers(symbol->type) & 0x1000) compiler_warning(4, 132, symbol->name);
        if (symbol->storage[2] & 2) initialize_thread_symbol(symbol);
    }
    process_symbol_storage(symbol);
    global_expression_scope_active = 1;
    return symbol;
}

/* Original 0x33d00..0x33f18. Validate local storage, record cleanup/quick
   compilation work, apply the default data segment, and reject incomplete data. */
void __stdcall process_symbol_storage(FrontendSymbol *symbol)
{
    unsigned char storage[3];
    unsigned kind;
    TypeShape *shape;
    FrontendSymbol *tag;
    storage[0] = symbol->storage[0];
    storage[1] = symbol->storage[1];
    storage[2] = symbol->storage[2];
    kind = storage[0] & 7;
    if (kind == 1 || kind == 7) return;
    if (kind == 4 || kind == 5 || kind == 6) {
        shape = symbol->type->related;
        if (shape && !(shape->flags & 7)) {
            report_syntax_error(185, symbol->name);
            symbol->type = intern_type(remove_type_prefix(symbol->type));
        }
        if (kind != 4) {
            if (options.option_qc && normalized_Ol) symbol->references += 16;
            if (options.option_m) {
                write_ir_record(0x2a, make_symbol_reference(symbol));
                push_work_list((void **)&scope_cleanup_stack, 1);
                scope_cleanup_stack->value.symbol = symbol;
                push_work_list((void **)&scope_cleanup_stack, 1);
                scope_cleanup_stack->value.depth = parameter_nesting;
            }
        }
        if (options.option_qc && normalized_Ol && !scope_serial) record_inline_parameter(symbol);
    }
    if ((kind == 2 || kind == 3) && default_data_segment && (symbol->attributes & 4)) {
        if (symbol->attributes & 2) {
            if ((FrontendSymbol *)symbol->type->related->field08 != default_data_segment)
                report_syntax_error(193, symbol->name);
            return;
        }
        assign_symbol_segment(symbol, default_data_segment);
    }
    if (!(symbol->attributes & 4) && !type_size(symbol->type) && kind != 1) {
        if (symbol->type->flags & 0x400) {
            tag = (FrontendSymbol *)symbol->type->field08;
            if ((tag->kind & 0x1f) != 4) tag = (FrontendSymbol *)tag->type->field08;
            if (!(tag->flags & 1)) {
                report_syntax_error(79, symbol->name, tag->name);
                symbol->flags |= 0x80;
                return;
            }
        }
        report_syntax_error(133, symbol->name);
    }
}

/* Original 0x28a10..0x28a6c. Resolve the C tag namespace, introducing
   an incomplete tag when no enclosing scope already supplies one. */
FrontendSymbol *__stdcall lookup_or_declare_tag(char *name, unsigned short kind,
    void (__stdcall *insert)(FrontendSymbol *))
{
    FrontendSymbol *tag = lookup_tag(name);
    if (tag) tag = check_tag_kind(tag, kind);
    else tag = create_tag(name, kind, insert, 0);
    if (browser_filename && (!parameter_nesting || options.option_FR)) browser_symbol_reference(tag);
    return tag;
}

/* Original 0x2c980..0x2ca94. A declaration without a declarator may still
   introduce a tag in the current scope; scalar-only declarations are diagnosed. */
void __stdcall process_declaration(DeclarationSpec *declaration)
{
    CType *type = declaration->type;
    unsigned short flags = type->flags;
    char *name;
    switch (flags & 0x2fdf) {
    case 0x41: case 0x42: case 0x44: case 0x80:
    case 0x104: case 0x108: case 0x128:
        if (parser_flags & 0x4000) return;
        if (options.extensions_enabled) compiler_warning(2, 91);
        else report_syntax_error(59, "empty declaration");
        break;
    case 0x412: case 0x414: case 0x418:
        name = ((FrontendSymbol *)type->field08)->name;
        if (name == name_unnamed) {
            if (options.extensions_enabled) compiler_warning(2, 94, type_specifier_name(flags));
            else report_syntax_error(59, "empty declaration");
        } else if (!tag_in_current_scope)
            create_tag(name, (unsigned short)(flags ^ 0x10), insert_current_scope_symbol, 0);
        break;
    }
}

/* Original 0x2cf90..0x2d00e. Based/segment attributes require a retained
   prefix shape; ordinary attributes can be stored directly in the type. */
FrontendSymbol *__stdcall apply_declarator_prefix(FrontendSymbol *declarator, TypeShape *prefix)
{
    TypeDescription *description;
    ArenaPool *pool;
    if (prefix->attributes & 0xc) {
        declarator->attributes |= 2;
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(TypeDescription)) refill_arena(&compiler_arena, pool);
        description = (TypeDescription *)pool->next;
        pool->remaining -= sizeof(TypeDescription);
        pool->next += sizeof(TypeDescription);
        *description = *declarator->type;
        prefix->related = description->related;
        description->related = prefix;
        declarator->type = description;
    } else declarator->type->attributes = prefix->attributes;
    return declarator;
}

/* Original 0x33f20..0x33f5b. Automatic/register storage discards an
   explicit segment prefix and diagnoses unsupported memory-model attributes. */
unsigned short __stdcall normalize_symbol_storage(unsigned short attributes, FrontendSymbol *symbol)
{
    if (symbol->attributes & 2) {
        symbol->type = remove_type_prefix(symbol->type);
        symbol->attributes &= ~2;
    }
    if (attributes & 0xf) {
        warn_ignored_storage_model(symbol);
        attributes &= 0xfbf8;
    }
    return attributes;
}

/* Original 0x34ce0..0x34df9. Compare a repeated declaration's base type,
   shapes, storage and attributes, retaining one diagnostic for each failure. */
int __stdcall declaration_conflict(FrontendSymbol *existing, FrontendSymbol *symbol)
{
    int error;
    if (symbol->flags & 0x80) return 1;
    if (!compatible_base_type_flags(existing->type->flags, symbol->type->flags)) {
        error = 371;
        goto conflict;
    }
    if (existing->type->field08 != symbol->type->field08) {
        error = 371;
        goto conflict;
    }
    error = compare_declaration_types(existing, symbol, 0);
    if (error) {
        if (error != 40) goto conflict;
        existing->flags |= 0x80;
        symbol->flags |= 0x80;
        report_type_conflict(error, 0, existing->name, symbol->type, existing->type);
        return 1;
    }
    if (!compatible_storage_classes(existing, symbol)) {
        error = (existing->kind & 0x1f) == 7 ? 375 : 370;
        goto conflict;
    }
    if (!compatible_declaration_attributes(existing->type->attributes,
        symbol->type->attributes, (existing->kind & 0x1f) == 7)) {
        error = 373;
        goto conflict;
    }
    if ((existing->attributes & 4) && (symbol->attributes & 4)) {
        error = 374;
        goto conflict;
    }
    return 0;
conflict:
    existing->flags |= 0x80;
    symbol->flags |= 0x80;
    report_syntax_error(error, existing->name);
    return 1;
}

/* Original 0x353b0..0x3559d. Compare declaration shape chains, retaining
   an explicit segment prefix and allowing an incomplete outer array bound. */
int __stdcall compare_declaration_types(FrontendSymbol *left, FrontendSymbol *right, CType *right_type)
{
    CType *left_type = left->type, *merged;
    TypeShape *left_shape = left_type->related, *right_shape;
    FrontendSymbol *segment;
    unsigned depth = 0, kind;
    int left_prefix = left_shape && !(left_shape->flags & 7);
    right_type = right ? right->type : right_type;
    right_shape = right_type->related;
    if (right_shape && !(right_shape->flags & 7)) {
        if (left_prefix) {
            if (left_shape->field08 != right_shape->field08) {
                segment = (FrontendSymbol *)left_shape->field08;
                if (segment->index != 5 && segment->index != 2) return 376;
            }
        } else {
            left->type = make_qualified_type(left_type, right_shape->flags,
                right_shape->attributes, &right_shape->field08);
            left->attributes |= 2;
        }
        if (right_shape && !(right_shape->flags & 7)) right_shape = right_shape->related;
    }
    if (left_prefix && left_shape && !(left_shape->flags & 7)) left_shape = left_shape->related;
    for (;;) {
        if (left_shape == right_shape) return 0;
        if (!left_shape || !right_shape) return 40;
        ++depth;
        if ((left_shape->flags ^ right_shape->flags) & 7) return 372;
        if (!compatible_shape_attributes(left_shape->attributes, right_shape->attributes)) return 373;
        kind = left_shape->flags & 7;
        if (kind == 2) {
            if (left_shape->field08 != right_shape->field08 &&
                !(depth == 1 && (!left_shape->field08 || !right_shape->field08))) return 369;
        } else if (kind == 4 && right && !(right->flags & 1)) {
            if ((left_shape->flags ^ right_shape->flags) & 0x8000) {
                if (options.extensions_enabled) compiler_warning(4, 52);
                else report_syntax_error(142);
            }
            if (left_shape->field08 != right_shape->field08) {
                merged = merge_function_parameter_types(left->type, right->type, 1, depth == 1);
                if (depth == 1) left->type = merged;
            }
        }
        left_shape = left_shape->related;
        right_shape = right_shape->related;
    }
}

/* Original 0x34fb0..0x353b0. Reconcile prototypes, including nested
   function-pointer parameters and the default promotions of an old declaration. */
CType *__stdcall merge_function_parameter_types(CType *left, CType *right, int find_function, int outermost)
{
    TypeShape *left_shape, *right_shape, *prefix, *replacement;
    ParameterInfo *left_parameters, *right_parameters, *merged_parameters = 0;
    TypeDescription *description;
    CType *result = left, *parameter, *promoted;
    ArenaPool *pool;
    int left_count, right_count, count, index;
    unsigned bytes, offset;
    if (find_function) {
        left_shape = left->related;
        right_shape = right->related;
        if (left_shape && !(left_shape->flags & 7)) left_shape = left_shape->related;
        if (right_shape && !(right_shape->flags & 7)) right_shape = right_shape->related;
        while ((left_shape->flags & 7) != 4) left_shape = left_shape->related;
        while ((right_shape->flags & 7) != 4) right_shape = right_shape->related;
    } else {
        left_shape = left->related->related;
        right_shape = right->related->related;
        if (((left->flags ^ right->flags) & 0xefdf) || left->field08 != right->field08)
            return 0;
    }
    left_parameters = (ParameterInfo *)left_shape->field08;
    right_parameters = (ParameterInfo *)right_shape->field08;
    left_count = left_parameters->count;
    right_count = right_parameters->count;
    if (left_count && right_count) {
        if (!find_function && left_count != right_count) return 0;
        count = left_count;
        if (find_function && right_count < left_count) {
            count = right_count;
            if (options.extensions_enabled) compiler_warning(1, 30);
            else report_syntax_error(190);
        } else if (find_function && right_count > left_count) {
            if (options.extensions_enabled) compiler_warning(1, 31);
            else report_syntax_error(191);
        }
        for (index = 0; index < count; ++index) {
            if (left_parameters->types[index] == right_parameters->types[index]) continue;
            parameter = 0;
            if (outermost && is_function_pointer_type(left_parameters->types[index]) &&
                is_function_pointer_type(right_parameters->types[index]))
                parameter = merge_function_parameter_types(left_parameters->types[index],
                    right_parameters->types[index], 0, 1);
            if (parameter) {
                if (parameter == right_parameters->types[index]) continue;
                if (!merged_parameters) {
                    bytes = 8 + right_count * sizeof(CType *);
                    pool = &compiler_arena.pools[2];
                    if ((int)bytes > pool->remaining) refill_arena(&compiler_arena, pool);
                    merged_parameters = (ParameterInfo *)pool->next;
                    pool->remaining -= bytes;
                    pool->next += bytes;
                    /* The original copies the newly allocated workspace into
                       the existing descriptor, in this direction (351ce..351e2). */
                    for (offset = 0; offset < bytes; ++offset)
                        ((unsigned char *)right_parameters)[offset] = ((unsigned char *)merged_parameters)[offset];
                }
                merged_parameters->types[index] = parameter;
            } else {
                if (!find_function) return 0;
                if (options.extensions_enabled) compiler_warning(1, 28, index + 1);
                else report_syntax_error(192, index + 1);
            }
        }
        if (!merged_parameters) return right;
        replacement = intern_type_shape(new_type_shape(right_shape->flags,
            right_shape->attributes, &merged_parameters));
        if (!find_function) prefix = right->related;
        else prefix = right->related != right_shape ? right->related : 0;
        if (prefix) {
            prefix = new_type_shape(prefix->flags, prefix->attributes, &prefix->field08);
            prefix->related = replacement;
            replacement = intern_type_shape(prefix);
        }
        description = copy_type_description(right);
        description->related = replacement;
        result = intern_type(description);
        --type_description_depth;
        return result;
    }
    if (!left_count && !right_count) return right;
    if (!left_count) result = right;
    if (!find_function) return result;
    if (!left_count) left_parameters = right_parameters;
    count = left_count + right_count;
    for (index = 0; index < count; ++index) {
        parameter = left_parameters->types[index];
        if (!parameter) break;
        promoted = default_argument_promotion(parameter);
        if ((promoted && promoted != parameter) ||
            (!parameter->related && parameter->flags == 0x104)) {
            if (options.extensions_enabled) compiler_warning(4, 32, index + 1);
            else report_syntax_error(379, index + 1);
        }
    }
    return result;
}

/* Original 0x34e50..0x34f7c. A compatible redeclaration may supply an
   omitted storage class, but must retain DLL/thread storage modifiers. */
int __stdcall compatible_storage_classes(FrontendSymbol *left, FrontendSymbol *right)
{
    unsigned left_class = left->storage[0] & 7, right_class = right->storage[0] & 7;
    unsigned left_storage, right_storage;
    if ((left->kind & 0x1f) == 7 || !(left->attributes & 4)) {
        if (left_class == right_class || (left_class == 1 && right_class == 2)) {
            left_storage = left->storage[0] | ((unsigned)left->storage[1] << 8) |
                ((unsigned)left->storage[2] << 16);
            right_storage = right->storage[0] | ((unsigned)right->storage[1] << 8) |
                ((unsigned)right->storage[2] << 16);
            if (compatible_storage_modifiers(left_storage, right_storage)) return 1;
        }
    }
    if (right_class == 3) {
        if (options.extensions_enabled && left_class == 1 && !(left->flags & 1)) {
            compiler_warning(4, 211);
            return 1;
        }
        return 0;
    }
    if ((left_class == 7 || right_class == 7) && !parameter_nesting) return 0;
    if (!(left->storage[2] & 4) && (right->storage[2] & 4)) return 0;
    if ((left->storage[2] & 4) && right_class == 3) return 0;
    return 1;
}

/* Original 0x3fa40..0x3fa78. Storage is passed by value in the low three
   bytes; DLL import/export and thread-local modifiers must agree. */
int __stdcall compatible_storage_modifiers(unsigned left, unsigned right)
{
    return !((left ^ right) & 0x70000UL);
}

/* Original 0x33f60..0x34016. An omitted memory model may agree with an
   explicit model only when it denotes the compilation's default model. */
int __stdcall compatible_declaration_attributes(unsigned short left, unsigned short right, int function)
{
    unsigned model;
    if ((left ^ right) & 0xfff8) return 0;
    if (left == right) return 1;
    if ((left & 7) && (right & 7)) return 0;
    model = (right | (left & 7)) & 7;
    if (function) {
        if (model == 1) return !(options.option_M & 1);
        return (options.option_M & 1) == 1;
    }
    if (model == 1) return !(options.option_M & 2);
    if (model == 2) return (options.option_M & 2) == 2;
    return options.option_M & 4;
}

/* Original 0x34a80..0x34c7b. Commit a compatible repeated declaration,
   retaining segment/type completion and merging storage/definition flags. */
void __stdcall commit_redeclaration(FrontendSymbol *existing, FrontendSymbol *symbol)
{
    TypeShape *old_shape = existing->type->related, *new_shape = symbol->type->related;
    int old_prefix = old_shape && !(old_shape->flags & 7);
    int new_prefix = new_shape && !(new_shape->flags & 7);
    int replace_prefix = 0, storage_warning = 0;
    unsigned storage;
    if (old_prefix) {
        if (new_prefix && old_shape->field08 != new_shape->field08) {
            existing->type = remove_type_prefix(existing->type);
            replace_prefix = 1;
        }
    } else if (new_prefix) {
        existing->attributes |= 2;
        replace_prefix = 1;
    }
    if (replace_prefix) {
        new_shape = symbol->type->related;
        existing->type = make_qualified_type(existing->type, new_shape->flags,
            new_shape->attributes, &new_shape->field08);
    }
    new_shape = symbol->type->related;
    old_shape = existing->type->related;
    if (new_shape && !(new_shape->flags & 7)) new_shape = new_shape->related;
    if (old_shape && !(old_shape->flags & 7)) old_shape = old_shape->related;
    if (new_shape) {
        switch (new_shape->flags & 7) {
        case 2:
            if (new_shape->field08 && !old_shape->field08) {
                existing->type = symbol->type;
                existing->attributes &= ~0x100;
            }
            break;
        case 4:
            if (symbol->flags & 1) {
                if (existing->flags & 1) report_symbol_error(84, existing);
                else existing->flags |= 1;
            }
            break;
        }
    }
    if ((existing->storage[2] & 0xc) == 0xc &&
        (symbol->storage[2] & 0xc) == 4) storage_warning = 1;
    else if ((existing->storage[2] & 0xc) == 4 &&
        (symbol->storage[2] & 0xc) != 4 && (symbol->storage[0] & 7) != 3 &&
        !(symbol->storage[1] & 1) && !(symbol->flags & 0x80) && !(existing->flags & 0x80))
        storage_warning = 1;
    if (storage_warning) {
        compiler_warning(1, 273, existing->name);
        existing->storage[0] = (existing->storage[0] & 0xf9) | 1;
        existing->storage[2] |= 0xc;
    }
    if ((existing->storage[0] & 7) == 1 && (symbol->storage[0] & 7) != 1) {
        if ((existing->attributes & 2) && (existing->type->related->flags & 7))
            report_syntax_error(179, existing->name);
        symbol->storage[1] |= existing->storage[1] & 0x10;
        symbol->storage[2] |= existing->storage[2] & 0xc;
        existing->storage[0] = symbol->storage[0];
        existing->storage[1] = symbol->storage[1];
        existing->storage[2] = symbol->storage[2];
    }
    existing->flags |= symbol->flags & 1;
    existing->attributes |= symbol->attributes & 4;
    storage = symbol->storage[0] | ((unsigned)symbol->storage[1] << 8) |
        ((unsigned)symbol->storage[2] << 16);
    if (merge_storage_modifiers(existing->storage, storage)) existing->flags &= ~2;
}

/* Original 0x3fa80..0x3fb15. Add the declaration's inheritable storage
   modifiers, reporting whether the existing storage description changed. */
int __stdcall merge_storage_modifiers(unsigned char *target, unsigned source)
{
    int changed = 0;
    if ((source & 0x100) && !(target[1] & 1)) {
        target[1] |= 1;
        changed = 1;
    }
    if ((source & 0x40) && !(target[0] & 0x40)) {
        target[0] |= 0x40;
        changed = 1;
    }
    if ((source & 0x1000) && !(target[1] & 0x10)) {
        target[1] |= 0x10;
        changed = 1;
    }
    if ((source & 0x10000UL) && !(target[2] & 1)) {
        target[2] |= 1;
        changed = 1;
    }
    if ((source & 0x80000UL) && (target[2] & 4) &&
        (source & 0x40000UL) && !(target[2] & 8)) {
        target[2] |= 8;
        changed = 1;
    }
    return changed;
}

/* Original 0x31d50..0x31e69. Parameters adjust an outer array to a
   pointer, propagating element qualifiers; other arrays retain completion bits. */
void __stdcall finish_array_declarator(FrontendSymbol *symbol, TypeShape **result,
    ShapeCursor current, int is_typedef)
{
    unsigned kind = symbol->kind & 0x1f, based = 0;
    unsigned short attributes, qualifiers;
    TypeShape *shape;
    if (kind != 10 && kind != 11) {
        if (!is_typedef) {
            symbol->attributes |= 0x400;
            if (!current.shape->field08) {
                symbol->attributes |= 0x100;
                return;
            }
        }
        return;
    }
    if (symbol->attributes & 2) {
        symbol->attributes &= ~2;
        based = (*result)->field08;
        attributes = (*result)->attributes & 15;
    } else attributes = normalize_type_attributes(1, (unsigned short)(symbol->type->attributes & 15));
    if (current.flags & 0x1000) report_syntax_error(540);
    qualifiers = current.attributes & 0x3000;
    if (qualifiers) {
        shape = current.shape->related;
        if (shape) current.shape->related = intern_type_shape(new_type_shape(shape->flags,
            (unsigned short)(shape->attributes | qualifiers), &shape->field08));
        else symbol->type->attributes |= qualifiers;
    }
    symbol->type->attributes &= ~7;
    shape = new_type_shape(1, attributes, &based);
    shape->related = current.shape->related;
    *result = intern_type_shape(shape);
}

/* Original 0x31c80..0x31d50. Separate declaration prefixes and carry base
   qualifiers through arrays/functions to the first substantive inner shape. */
TypeShape *__stdcall extract_base_prefix(TypeShape *shape, TypeShape **prefix,
    FrontendSymbol *symbol, int is_typedef, unsigned short *attributes)
{
    unsigned short flags, kind, merged;
    TypeShape *result;
    while (shape && !(shape->flags & 7)) {
        if (!symbol->type->related) copy_shape_prefix(symbol, shape, prefix, is_typedef);
        else *prefix = shape;
        shape = shape->related;
    }
    if (!shape) return 0;
    flags = shape->flags;
    kind = flags & 7;
    if (kind != 4 && kind != 2) {
        merged = merge_type_attributes((unsigned short)(*attributes & 0x3000), shape->attributes);
        result = prepend_interned_type_shape(shape->related, flags, merged, &shape->field08);
        *attributes &= 0xcfff;
        return result;
    }
    if (shape->related) {
        if (kind == 4) flags |= 0x400;
        result = extract_base_prefix(shape->related, prefix, symbol, is_typedef, attributes);
        return prepend_interned_type_shape(result, flags, shape->attributes, &shape->field08);
    }
    return shape;
}

/* Original 0x29700..0x29918. Finish a member declaration, place its data
   or bitfield, and append it to the active aggregate's member sequence. */
void __stdcall add_struct_member(DeclarationSpec *declaration, FrontendSymbol *member)
{
    CType *base;
    FrontendSymbol *tag, *copy;
    AggregateContext *context;
    char *name;
    if (aggregate_depth < 0) {
        report_syntax_error(28);
        return;
    }
    base = declaration->type;
    if (!member) {
        if (!(base->flags & 0x400)) {
            report_type_syntax_error(208, base);
            return;
        }
        if (options.extensions_enabled) compiler_warning(4, 201);
        else report_syntax_error(467, (base->flags & 0x404) == 0x404 ? "union" :
            (base->flags & 0x402) == 0x402 ? "struct" : "enum");
        tag = (FrontendSymbol *)base->field08;
        if (!(tag->flags & 1)) {
            if (!options.prototypes_only) report_syntax_error(27, tag->name);
            return;
        }
        add_anonymous_members(base);
        return;
    }
    if (parameter_nesting > 0 && !(scanner_flags & 0x80000)) {
        copy = allocate_permanent(sizeof(FrontendSymbol));
        *copy = *member;
        member = copy;
    }
    member = declare_symbol(declaration, member);
    if (!member) return;
    name = member->name;
    if (check_member_name(name)) return;
    context = &aggregate_contexts[aggregate_depth];
    if (effective_type_qualifiers(member->type) & 0x1000)
        context->tag->metadata.value |= 0x04000000UL;
    if ((member->kind & 0x1f) == 2)
        layout_bitfield_member(member, name, member->type->flags);
    else {
        if (name != name_alignment) member->detail.value = layout_data_member(member);
        append_aggregate_member(member);
        context = &aggregate_contexts[aggregate_depth];
        context->tag->attributes = (context->tag->attributes & ~0x100UL) | (member->attributes & 0x100);
        context->bitfield_type = 0;
    }
    if (browser_filename && (!parameter_nesting || options.option_FR) &&
        aggregate_contexts[aggregate_depth].tag->name != name_unnamed)
        browser_symbol_definition(member);
}

/* Original 0x29500..0x2956e. The interned-name flag avoids a list walk
   for unused spellings. A repeated member is diagnosed; the routine returns 0. */
int __stdcall check_member_name(char *name)
{
    FrontendSymbol *tag, *member;
    if (!name || name == name_alignment || !(SYMBOL_FROM_NAME(name)->flags & 4)) return 0;
    tag = aggregate_contexts[aggregate_depth].tag;
    for (member = tag->associated.members; member; member = member->next) {
        if (member->name == name) {
            report_syntax_error(20, name, type_specifier_name(tag->type->flags));
            break;
        }
    }
    return 0;
}

/* Original 0x28f80..0x2921b. Complete an aggregate member's byte layout,
   flushing pending bitfields and honoring packing and incomplete-array rules. */
unsigned __stdcall layout_data_member(FrontendSymbol *member)
{
    AggregateContext *context = &aggregate_contexts[aggregate_depth];
    FrontendSymbol *tag;
    CType *type;
    TypeShape *shape;
    unsigned bytes, offset, scalar_bytes;
    if ((context->flags & 1) && (context->tag->attributes & 0x100)) {
        tag = context->tag;
        report_syntax_error(229, type_specifier_name(tag->type->flags), tag->name);
        aggregate_contexts[aggregate_depth].tag->attributes &= ~0x100UL;
    }
    bytes = type_size(member->type);
    if (!bytes) {
        tag = (FrontendSymbol *)member->type->field08;
        if (tag && !(tag->flags & 1)) {
            report_syntax_error(79, member->name, tag->name);
            return 0;
        }
        if (!options.extensions_enabled) {
            report_syntax_error(133, member->name);
            return 0;
        }
        compiler_warning(4, 200);
        bytes = type_size(drop_first_type_shape(member->type));
        member->attributes |= 0x100;
    }
    context = &aggregate_contexts[aggregate_depth];
    if ((context->flags & 1) && context->bitfield_type) {
        context->size += context->bitfield_type & 15;
        context->bitfield_used_bits = 0;
    }
    type = member->type;
    while (type->related && (type->related->flags & 7) == 2)
        type = drop_first_type_shape(type);
    shape = type->related;
    if (!shape) {
        if (type->flags & 0x140) {
            scalar_bytes = type->flags & 15;
            if ((unsigned)top_packing(packing_stack)->alignment > scalar_bytes) {
                align_aggregate_member(scalar_bytes);
                goto aligned;
            }
        }
        if ((type->flags & 0x410) == 0x410) {
            tag = (FrontendSymbol *)type->field08;
            align_aggregate_member((tag->metadata.value >> 16) & 0xff);
        } else align_aggregate_member(type_size(type));
    } else if (shape->flags & 1) align_aggregate_member(type_size(type));
aligned:
    context = &aggregate_contexts[aggregate_depth];
    if (context->flags & 1) {
        offset = context->size;
        if (!(member->attributes & 0x100)) {
            if (bytes && addition_overflows(offset, bytes - 1, 1, 4)) {
                tag = aggregate_contexts[aggregate_depth].tag;
                report_syntax_error(89, tag->name, type_specifier_name(tag->type->flags));
                return 0;
            }
            aggregate_contexts[aggregate_depth].size += bytes;
        }
    } else {
        offset = 0;
        if (context->size < bytes) context->size = bytes;
    }
    type = member->type;
    if (!type->related && (type->flags & 0x400)) {
        tag = (FrontendSymbol *)type->field08;
        member->attributes = (member->attributes & ~0x100UL) | (tag->attributes & 0x100);
        aggregate_contexts[aggregate_depth].tag->attributes |= tag->attributes & 0x200;
        if ((type->flags & 0x404) == 0x404 || (tag->metadata.value & 0x01000000UL))
            aggregate_contexts[aggregate_depth].tag->metadata.value |= 0x01000000UL;
    }
    return add_member_offset(offset, 0);
}

/* Original 0x2a300..0x2a43e. Record the aggregate's maximum effective
   alignment and represent inserted padding as an explicit alignment member. */
void __stdcall align_aggregate_member(unsigned bytes)
{
    unsigned effective = bytes, padding;
    FrontendSymbol *member;
    ArenaPool *pool;
    if ((unsigned)top_packing(packing_stack)->alignment < bytes)
        effective = top_packing(packing_stack)->alignment;
    if (aggregate_contexts[aggregate_depth].alignment <= (unsigned short)effective) {
        effective = bytes;
        if ((unsigned)top_packing(packing_stack)->alignment < bytes)
            effective = top_packing(packing_stack)->alignment;
        aggregate_contexts[aggregate_depth].alignment = (unsigned short)effective;
    }
    padding = aggregate_padding(bytes);
    if (!padding) return;
    effective = bytes;
    if ((unsigned)top_packing(packing_stack)->alignment < bytes)
        effective = top_packing(packing_stack)->alignment;
    if (effective != bytes)
        compiler_warning(4, 121, aggregate_contexts[aggregate_depth].tag->name);
    if (!parameter_nesting) member = allocate_permanent(sizeof(FrontendSymbol));
    else {
        pool = &compiler_arena.pools[1];
        if (pool->remaining < (int)sizeof(FrontendSymbol)) refill_arena(&compiler_arena, pool);
        member = (FrontendSymbol *)pool->next;
        pool->remaining -= sizeof(FrontendSymbol);
        pool->next += sizeof(FrontendSymbol);
    }
    member->kind = 3;
    member->flags |= 0x20;
    member->name = name_alignment;
    member->detail.value = padding;
    append_aggregate_member(member);
    aggregate_contexts[aggregate_depth].size += padding;
}

/* Original 0x2a270..0x2a28d. Query padding at the active aggregate's
   current byte position. */
unsigned __stdcall aggregate_padding(unsigned bytes)
{
    return packing_padding(bytes, aggregate_contexts[aggregate_depth].size);
}

/* Original 0x29920..0x2992b. Preserve the separate offset-addition entry. */
unsigned __stdcall add_member_offset(unsigned base, unsigned displacement)
{
    return base + displacement;
}

/* Original 0x2a4a0..0x2a4ae. Single-member wrapper around range insertion. */
void __stdcall append_aggregate_member(FrontendSymbol *member)
{
    append_aggregate_member_range(member, member);
}

/* Original 0x2a440..0x2a49c. Mark every spelling in the inserted range,
   then link the range after the aggregate's retained tail. */
void __stdcall append_aggregate_member_range(FrontendSymbol *first, FrontendSymbol *last)
{
    FrontendSymbol *member = first;
    AggregateContext *context;
    for (;;) {
        if (member->name) SYMBOL_FROM_NAME(member->name)->flags |= 4;
        if (member == last) break;
        member = member->next;
    }
    context = &aggregate_contexts[aggregate_depth];
    if (context->last_member) context->last_member->next = first;
    else context->tag->associated.members = first;
    aggregate_contexts[aggregate_depth].last_member = last;
}

/* Original 0x28ae0..0x28be3. Complete an aggregate's size/alignment,
   flush its final bitfield unit, and return the definition type to the grammar. */
CType *parse_struct_specifier(void)
{
    AggregateContext *context = &aggregate_contexts[aggregate_depth];
    FrontendSymbol *tag = context->tag;
    CType *type = context->type;
    unsigned short alignment;
    if (tag->name != name_unnamed && browser_filename &&
        (!parameter_nesting || options.option_FR)) browser_finish_scope();
    context = &aggregate_contexts[aggregate_depth];
    if (context->bitfield_type && context->bitfield_used_bits)
        context->size += context->bitfield_type & 15;
    align_aggregate_member(context->alignment);
    context = &aggregate_contexts[aggregate_depth];
    tag->detail.value = context->size;
    if (!tag->detail.value) tag->detail.value = 4;
    tag->metadata.value = (tag->metadata.value & 0xff00ffffUL) | ((unsigned)(context->alignment & 0xff) << 16);
    tag->flags |= 1;
    tag->type->field0c &= 0xffff0000UL;
    alignment = context->alignment;
    if (--aggregate_depth >= 0 && aggregate_contexts[aggregate_depth].alignment <= alignment)
        aggregate_contexts[aggregate_depth].alignment = alignment;
    tag_in_current_scope = 1;
    return type;
}

/* Original 0x29570..0x296fa. Surround an anonymous aggregate's copied
   members with explicit begin/end markers, adjusting offsets for a struct. */
void __stdcall add_anonymous_members(CType *type)
{
    FrontendSymbol *first, *last, *member, *copy, *tag;
    unsigned offset, kind;
    aggregate_contexts[aggregate_depth].tag->attributes |= 0x200;
    first = allocate_permanent(sizeof(FrontendSymbol));
    first->flags |= 0x20;
    first->kind = 3;
    first->type = type;
    first->name = name_alignment;
    first->attributes |= 0x800;
    offset = layout_data_member(first);
    first->detail.value = offset;
    tag = (FrontendSymbol *)type->field08;
    aggregate_contexts[aggregate_depth].tag->attributes =
        (aggregate_contexts[aggregate_depth].tag->attributes & ~0x100UL) | (tag->attributes & 0x100);
    if ((tag->metadata.value & 0x01000000UL) || (type->flags & 0x404) == 0x404)
        aggregate_contexts[aggregate_depth].tag->metadata.value |= 0x01000000UL;
    else aggregate_contexts[aggregate_depth].tag->metadata.value &= ~0x01000000UL;
    last = first;
    for (member = tag->associated.members; member; member = member->next) {
        kind = member->kind & 0x1f;
        if (kind == 15 || kind == 17 || kind == 4) continue;
        if (check_member_name(member->name)) continue;
        copy = allocate_permanent(sizeof(FrontendSymbol));
        *copy = *member;
        last->next = copy;
        last = copy;
        if (aggregate_contexts[aggregate_depth].flags & 1) copy->detail.value += offset;
    }
    member = allocate_permanent(sizeof(FrontendSymbol));
    member->flags |= 0x20;
    member->kind = 3;
    member->type = type;
    member->name = name_alignment;
    member->attributes |= 0x1000;
    last->next = member;
    if (first) {
        member->next = 0;
        append_aggregate_member_range(first, member);
    }
    aggregate_contexts[aggregate_depth].bitfield_type = 0;
}

/* Original 0x2c8f0..0x2c94f. Declaration bounds require an integer or
   enum constant; retain the original diagnostic-specific recovery values. */
unsigned __stdcall integer_constant_value(IrToken *expression)
{
    if (!expression) {
        report_syntax_error(56);
        return 0;
    }
    if ((expression->flags & 7) != 3) {
        report_syntax_error(57);
        return 1;
    }
    if (expression->type->flags & 0x240) return expression->value.integer;
    report_syntax_error(58);
    return 0;
}

/* Original 0x28ca0..0x28f71. Pack bitfields into compatible storage units,
   handling zero-width separators, extension types and anonymous first fields. */
void __stdcall layout_bitfield_member(FrontendSymbol *member, char *name, unsigned short flags)
{
    AggregateContext *context = &aggregate_contexts[aggregate_depth];
    FrontendSymbol *tag, *copy;
    unsigned width, bytes, capacity, previous_bytes;
    if ((context->flags & 1) && (context->tag->attributes & 0x100)) {
        tag = context->tag;
        report_syntax_error(229, type_specifier_name(tag->type->flags), tag->name);
        aggregate_contexts[aggregate_depth].tag->attributes &= ~0x100UL;
    }
    if (!(flags & 0x40) || !(flags & 0x20)) {
        if (!options.extensions_enabled && !options.z_E) {
            report_syntax_error(150, name);
            member->type = type_4064_first;
            flags = 0x4064;
        } else if (flags & 0x40) compiler_warning(4, 214);
        else {
            compiler_warning(1, 15, name);
            member->type = type_4064_first;
            flags = 0x4064;
        }
    }
    if (member->type->related) report_syntax_error(33, name);
    width = member->metadata.bitfield.width;
    if (!width) {
        if (member->name != name_alignment) report_syntax_error(149, name);
        if (!aggregate_contexts[aggregate_depth].bitfield_type) return;
    }
    bytes = flags & 15;
    capacity = bytes * 8;
    if (width > capacity) {
        report_syntax_error(34, name);
        width = capacity;
    }
    context = &aggregate_contexts[aggregate_depth];
    if (context->flags & 1) {
        if (((flags ^ context->bitfield_type) & 15) || !width ||
            context->bitfield_used_bits + width > capacity) {
            if (context->bitfield_type) context->size += context->bitfield_type & 15;
            context->bitfield_used_bits = 0;
            align_aggregate_member(bytes);
        }
    } else {
        /* The union path compares with the previous pending unit's size. */
        previous_bytes = context->bitfield_type & 15;
        if ((int)context->size < (int)previous_bytes) context->size = previous_bytes;
    }
    context = &aggregate_contexts[aggregate_depth];
    context->bitfield_type = width ? flags : 0;
    if (member->name == name_alignment) {
        if ((context->flags & 1) && !context->tag->associated.members) {
            copy = allocate_permanent(sizeof(FrontendSymbol));
            *copy = *member;
            append_aggregate_member(copy);
            context = &aggregate_contexts[aggregate_depth];
            copy->detail.value = context->size;
            copy->metadata.bitfield.width = (unsigned char)width;
            copy->metadata.bitfield.offset = context->bitfield_used_bits;
        }
    } else {
        append_aggregate_member(member);
        member->metadata.bitfield.width = (unsigned char)width;
        member->metadata.bitfield.offset = aggregate_contexts[aggregate_depth].bitfield_used_bits;
    }
    context = &aggregate_contexts[aggregate_depth];
    if (context->flags & 1) {
        context->bitfield_used_bits += (unsigned char)width;
        member->detail.value = context->size;
    } else member->detail.value = 0;
}

/* Original 0x347d0..0x3492a. A block-scope extern may share its global
   declaration, while retaining the local symbol in the current scope. */
FrontendSymbol *__stdcall merge_local_declaration(FrontendSymbol *symbol)
{
    FrontendSymbol *existing, *parameter;
    unsigned kind;
    if (parameter_nesting == 2 && (unsigned char)current_scope->flags != 1) {
        for (parameter = current_parameter; parameter; parameter = parameter->next)
            if (parameter->name == symbol->name) {
                report_syntax_error(82, symbol->name);
                return symbol;
            }
    }
    if ((symbol->storage[0] & 7) != 1) return 0;
    existing = find_scope_declaration(global_scope, symbol->name,
        (symbol->type->attributes & 0x70) == 0x20);
    if (!existing) return 0;
    kind = existing->kind & 0x1f;
    if (kind == 4 || (kind >= 1 && kind <= 13 && kind != 5 &&
        (existing->storage[0] & 7) == 7)) return 0;
    parser_flags |= 0x04000000;
    if (declaration_conflict(existing, symbol)) {
        parser_flags &= ~0x04000000;
        return 0;
    }
    parser_flags &= ~0x04000000;
    symbol->index = existing->index;
    symbol->type = existing->type;
    symbol->attributes = (symbol->attributes & ~0x502UL) | (existing->attributes & 0x502);
    insert_current_scope_symbol(symbol);
    if ((existing->kind & 0x1f) == 6) {
        symbol->storage[0] = (symbol->storage[0] & ~7) | 1;
        symbol->kind = 6;
        symbol->detail.intrinsic = existing->detail.intrinsic;
    }
    ++existing->references;
    symbol->flags |= 2;
    existing->flags &= ~2;
    return existing;
}

/* Original 0x34930..0x34a80. Local static data receives a persistent
   global alias and a name incorporating its function-scope level. */
void __stdcall record_local_declaration(FrontendSymbol *symbol)
{
    FrontendSymbol *copy;
    if (options.extensions_enabled && (symbol->kind & 0x1f) == 7) {
        compiler_warning(4, 210);
        copy = allocate_permanent(sizeof(FrontendSymbol));
        *copy = *symbol;
        copy->attributes |= 0x2000;
        insert_scope_symbol(global_scope, copy);
        ++copy->references;
        symbol->flags |= 2;
        return;
    }
    if ((symbol->kind & 0x1f) != 9 || (symbol->storage[0] & 7) != 3) return;
    copy = allocate_permanent(sizeof(FrontendSymbol));
    *copy = *symbol;
    if (function_symbol && (function_symbol->storage[1] & 1) &&
        (function_symbol->storage[0] & 7) != 3) {
        if (function_symbol->storage[2] & 4) {
            copy->storage[2] |= 4;
            copy->storage[2] = (copy->storage[2] & ~8) | (function_symbol->storage[2] & 8);
            symbol->storage[0] = copy->storage[0];
            symbol->storage[1] = copy->storage[1];
            symbol->storage[2] = copy->storage[2];
        }
        copy->storage[0] = ((copy->storage[0] & ~7) | 2) | 0x40;
    }
    copy->name = local_static_name(symbol);
    if (next_symbol_index >= 0x7fffffff) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_index_overflow();
    copy->index = next_symbol_index++;
    copy->flags |= 1;
    symbol->attributes |= 0x40;
    record_symbol_alias(symbol, copy);
    insert_scope_symbol(global_scope, copy);
}

/* Original 0x2fdb0..0x2fe5b. An initializer turns a file-scope extern
   into a definition; typedef initializers are rejected before preparation. */
FrontendSymbol *__stdcall declare_initialized_symbol(FrontendSymbol *symbol)
{
    unsigned char storage;
    if (!symbol) return 0;
    storage = symbol->storage[0] & 7;
    if (storage == 7) {
        report_syntax_error(71, symbol->name);
        return 0;
    }
    if (storage == 1) {
        if (parameter_nesting) report_syntax_error(205, symbol->name);
        symbol->storage[0] = (symbol->storage[0] & ~7) | 2;
    }
    if (options.prototypes_only) return symbol;
    if ((symbol->storage[2] & 4) && !(symbol->storage[2] & 8))
        parser_flags |= 0x20000000;
    return prepare_initializer_symbol(symbol);
}

/* Original 0x18c70..0x18d8d. Inside __declspec the scanner returns
   identifiers, and their interned keyword prefix selects storage modifiers. */
void parse_declspec(unsigned char *storage)
{
    unsigned char token, flags, keyword;
    char *name;
    DeclspecSpec *attribute;
    token = scan_token();
    if (token != TOK_LEFT_PAREN) {
        report_syntax_error(54, token_names[token]);
        unget_token(token);
        storage[0] |= 8;
        return;
    }
    parser_flags |= 0x10000000UL;
    while ((token = scan_token()) != TOK_RIGHT_PAREN) {
        if (token == TOK_IDENTIFIER) {
            name = current_token_value.identifier;
            flags = SYMBOL_FROM_NAME(name)->flags;
            if (flags & 0x20) {
                keyword = KEYWORD_FROM_NAME(name)->token;
                if (flags & 1) merge_storage_class(storage, 0, keyword, 0);
                else {
                    attribute = &declspec_specs[keyword];
                    if (attribute->attribute == 2)
                        merge_storage_class(storage, attribute->parameter_mode, 1, 0);
                    else if (!attribute->reserved)
                        merge_storage_class(storage, 0, attribute->attribute, 0);
                }
                continue;
            }
            report_syntax_error(485, name);
        } else report_syntax_error(59, token_names[token]);
        storage[0] |= 8;
    }
    parser_flags &= ~0x10000000UL;
}
