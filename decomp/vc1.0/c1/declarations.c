/* Symbol declaration and redeclaration processing. */
#include "parser.h"
#include "global-ir.h"

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
        if (!options.syntax_only && is_record_type(symbol->type)) {
            tag = (FrontendSymbol *)symbol->type->field08;
            if (tag->name == name_unnamed) tag->name = symbol->name;
        }
    }
    if (member) return symbol;
    existing = merge_global_declaration(symbol);
    if (!existing) {
        if (parameter_nesting >= 2) existing = merge_local_declaration(symbol);
        else if (!options.z_a && (current_scope->flags & 0xff) == 1) {
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
    if (!options.syntax_only && !void_declaration && !(attributes & 0x70))
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
    if (options.z_a && (existing->storage[0] & 7) == 7 &&
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
    if (!current_function) {
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
    if (current_function) return;
    for (symbol = pending_global_symbols; symbol; symbol = symbol->next)
        emit_symbol_declaration(symbol);
    write_function_directory();
    serialize_scope(global_scope);
    for (symbol = pending_global_symbols; symbol; symbol = symbol->next) {
        if (symbol->field28) {
            emit_symbol_declaration((FrontendSymbol *)symbol->field28);
            initializer = make_symbol_reference(symbol);
            target = prepare_initializer_symbol((FrontendSymbol *)symbol->field28);
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
    if (current_function > 0 || (symbol->flags & 2)) return;
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
        tag->field24 |= 0x02000000;
    else tag->field24 &= ~0x02000000;
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
    context->field04 = 0;
    context->field0c = 0;
    context->field14 = 0;
    context->field10 = 0;
    context->type = type;
    context->field12 = 1;
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
        tag->field28 = (unsigned)new_hash_table(4, (short)((scanner_flags & 0x08000000) != 0), 0, 0);
    }
    description->field08 = (unsigned)tag;
    tag->type = intern_type(description);
    if (name) {
        tag->name = name;
        if (declaration_depth > 0) {
            compiler_warning(options.z_a ? 4 : 1, 115, name);
            if (!definition && options.z_a) {
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
    names = (HashTable *)current_enum_tag->field28;
    if (!names) return;
    insert_hash_value(names, name, symbol);
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_index_overflow();
    symbol->index = next_symbol_index++;
    if (integer_conversion_overflow(4, 0, 0, value))
        compiler_warning(options.z_a ? 3 : 1, 341, symbol->name);
    ((EnumeratorSymbol *)symbol)->value = value;
    enumerator_value = value;
    if (browser_filename && (!parameter_nesting || options.option_FR)) browser_symbol_definition(symbol);
}

typedef char EnumeratorSymbol_must_be_28_bytes[(sizeof(EnumeratorSymbol) == 28) ? 1 : -1];
