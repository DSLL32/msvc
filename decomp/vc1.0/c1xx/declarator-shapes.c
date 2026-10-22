/* Folding a parsed declarator onto its base type. */
#include "declarations.h"
#include "cpp-context.h"
#include "macros.h"
#include "keywords.h"
#include "operators.h"

/* The original expands this test at each data-pointer/array/member-pointer
   case. Keep it local to that folding operation rather than inventing a
   reconstructed callee. Class defaults apply through an array-only suffix. */
#define INHERIT_CLASS_DISTANCE() \
    if (!(current.attributes & 7) && !inner.shape && \
        is_unqualified_class_type(base)) { \
        scan = next; \
        if (scan && (scan->flags & 0x77) == 2) { \
            do { scan = scan->related; } \
            while (scan && (scan->flags & 0x77) == 2); \
        } else scan = 0; \
        if (!scan) { \
            tag = (CppSymbol *)cpp_context_stack->items[base->field08]; \
            distance = tag->detail.class_info->pointer_attributes; \
            current.attributes |= (distance & 7) ? distance : \
                (unsigned short)data_pointer_kind; \
        } \
    }

/* Original 0x485d0..0x48e51. ShapeCursor is also passed by value to the
   specialized folding functions; its packed attributes/flags word matters. */
void __stdcall merge_declaration_type(CppSymbol *symbol, CType *base, int is_typedef)
{
    TypeShape *head = symbol->type->related;
    TypeShape *work = head, *prefix = 0, *base_shape = base->related;
    TypeShape *next, *extra, *result, *scan;
    unsigned short qualifiers = symbol->type->attributes & 0x3000, distance;
    ShapeCursor current, outer, inner, prepared_base;
    unsigned category;
    CppSymbol *tag;
    QualifiedDeclarator *qualifier;
    prepared_base.shape = base_shape;
    if (!prepare_declarator_shapes(symbol, base, &work, &prepared_base,
                                   &prefix, &head, is_typedef))
        return;
    base_shape = prepared_base.shape;
    if (!work) {
        work = base_shape;
        current.flags = work ? work->flags : 0;
        current.attributes = work ? work->attributes : 0;
    } else {
        for (;;) {
            next = work->related;
            outer.shape = next;
            if (next) {
                outer.flags = next->flags;
                outer.attributes = next->attributes;
                next->attributes &= ~0x3000;
            } else {
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
            switch (current.flags & 0x77) {
            case 1:
                INHERIT_CLASS_DISTANCE();
                if ((inner.flags & 0x77) == 4) {
                    current.flags = (current.flags & ~0x74) | 3;
                    current.flags |= inner.flags & 0xc800;
                    if ((current.attributes & 0x70) == 0x60 && (current.flags & 0x8000))
                        current.attributes &= ~0x70;
                    current.attributes = merge_type_attributes(current.attributes, inner.attributes);
                } else {
                    if (inner.flags & 0x40) report_syntax_error(528);
                    else {
                        current.shape = work;
                        check_pointer_attributes(symbol, &current, 165);
                        work = current.shape;
                    }
                }
                current.attributes = (current.attributes & ~0x3000) | (outer.attributes & 0x3000);
                break;
            case 2:
                INHERIT_CLASS_DISTANCE();
                if ((inner.flags & 0x77) == 4) {
                    report_syntax_error(92);
                    extra = new_type_shape(3, 0, 0);
                }
                if (inner.flags & 0x40) {
                    report_syntax_error(234);
                    symbol->declaration_flags |= 2;
                }
                current.attributes = normalize_type_attributes(2,
                    (unsigned short)((symbol->type->attributes & 7) | (inner.attributes & 0x3000)));
                if (!inner.shape && (symbol->type->flags & 0x400)) {
                    tag = (CppSymbol *)cpp_context_stack->items[symbol->type->field08];
                    if (tag->attributes & 0x100) report_syntax_error(233, symbol->name);
                }
                if (current.flags & 0x1000) {
                    if (work->data.array_bound) {
                        current.flags &= ~0x1000;
                        work->data.array_bound = 1;
                        report_syntax_error(540);
                    }
                } else if ((outer.flags & 0x77) == 2 && !work->data.array_bound)
                    report_syntax_error(87, symbol->name);
                break;
            case 4:
                current.shape = work;
                extra = merge_function_shape(symbol, inner, &current, outer, is_typedef);
                work = current.shape;
                break;
            case 0x10:
                member_pointer_class_kind(work->data.tag_symbol);
                if ((inner.flags & 0x77) == 4) {
                    current.flags = (current.flags & ~0x57) | 0x20;
                    current.flags |= inner.flags & 0xc800;
                    if ((current.attributes & 0x70) == 0x60 && (current.flags & 0x8000))
                        current.attributes &= ~0x70;
                    current.attributes = merge_type_attributes(current.attributes, inner.attributes);
                } else if (inner.flags & 0x40) report_syntax_error(636);
                else {
                    current.shape = work;
                    check_pointer_attributes(symbol, &current, 637);
                    work = current.shape;
                    if (current.attributes & 15) {
                        report_syntax_error(638, symbol->name);
                        current.attributes &= ~15;
                    }
                }
                current.attributes = (current.attributes & ~0x3000) | (outer.attributes & 0x3000);
                break;
            case 0x40:
                INHERIT_CLASS_DISTANCE();
                current.shape = work;
                if (!merge_reference_shape(symbol, inner, &current, outer)) {
                    base_shape = inner.shape;
                    work = next;
                    continue;
                }
                work = current.shape;
                break;
            }
            if (extra) {
                extra->related = base_shape;
                base_shape = intern_type_shape(extra);
            }
            category = current.flags & 0x77;
            if (next || !is_typedef || (category != 4 && category != 2)) {
                qualifier = (QualifiedDeclarator *)symbol->next;
                if (category == 4 && !next && qualifier &&
                    (!qualifier->resolution || qualifier->resolution->binding->next) &&
                    current_class_context < 0 && !(current.attributes & 0x70)) {
                    current.attributes = normalize_type_attributes(current.flags, current.attributes);
                    current.attributes &= ~0x70;
                } else
                    current.attributes = normalize_type_attributes(current.flags, current.attributes);
            }
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
    category = current.flags & 0x77;
    if (category == 4)
        finish_function_declarator(symbol, &result);
    else if (category == 2)
        finish_array_declarator(symbol, &result, current, is_typedef);
    else if ((current.flags & 0x40) && (symbol->kind & 0x1f) == 2)
        report_syntax_error(531, symbol->name);
    if (symbol->type->flags == 0x80) {
        if ((head->flags & 0x77) == 2)
            report_syntax_error(182, symbol->name);
        else if (head->flags & 0x40) {
            report_syntax_error(624, symbol->name);
            symbol->type->flags = 100;
        }
    }
    symbol->type->related = result;
}

/* Original 0x497f0..0x49c27. Separate prefix qualifiers from the two
   shape chains, then reconcile the boundary before the folding pass. */
int __stdcall prepare_declarator_shapes(CppSymbol *symbol, CType *base,
    TypeShape **work, ShapeCursor *base_shape, TypeShape **prefix,
    TypeShape **head, int is_typedef)
{
    TypeShape *node, *copy, *scan, *detached = 0;
    unsigned short *attributes, distance;
    ArenaPool *pool;
    CppSymbol *tag;
    if (!base_shape->shape) {
        if (*work) {
            node = extract_declarator_prefix(*work, prefix, symbol, is_typedef);
            *work = symbol->type->related;
            if (node)
                node->attributes = merge_type_attributes(node->attributes,
                    (unsigned short)(base->attributes & 0xcfff));
            else
                symbol->type->attributes = merge_type_attributes(symbol->type->attributes, base->attributes);
            *work = symbol->type->related;
            return 1;
        }
        symbol->type->attributes = merge_type_attributes(symbol->type->attributes, base->attributes);
        if (!is_typedef && !(symbol->type->attributes & 7) && !base_shape->shape &&
            is_unqualified_class_type(symbol->type)) {
            scan = *work;
            if (scan && (scan->flags & 0x77) == 2) {
                do { scan = scan->related; }
                while (scan && (scan->flags & 0x77) == 2);
            } else scan = 0;
            if (!scan) {
                tag = (CppSymbol *)cpp_context_stack->items[symbol->type->field08];
                distance = tag->detail.class_info->pointer_attributes;
                symbol->type->attributes |= (distance & 7) ? distance :
                    (unsigned short)data_pointer_kind;
            }
        }
        return 0;
    }
    if ((base_shape->shape->flags & 0x77) == 2 && !base_shape->shape->data.array_bound &&
        *work && (((*work)->flags & 0x77) == 2)) {
        report_syntax_error(87, symbol->name);
        symbol->declaration_flags |= 2;
    }
    if ((base_shape->shape->flags & 0x77) == 4) {
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
    *head = base_shape->shape;
    while ((*head)->related) *head = (*head)->related;
    if (!*work) {
        if (!is_typedef && !(symbol->type->attributes & 7) && !base_shape->shape &&
            is_unqualified_class_type(symbol->type)) {
            /* Both null tests above are retained from the original, even
               though this branch normally has a nonempty base chain. */
            scan = *work;
            if (scan && (scan->flags & 0x77) == 2) {
                do { scan = scan->related; }
                while (scan && (scan->flags & 0x77) == 2);
            } else scan = 0;
            if (!scan) {
                tag = (CppSymbol *)cpp_context_stack->items[symbol->type->field08];
                distance = tag->detail.class_info->pointer_attributes;
                symbol->type->attributes |= (distance & 7) ? distance :
                    (unsigned short)data_pointer_kind;
            }
        }
        base_shape->shape = extract_base_prefix(base_shape->shape, prefix, symbol,
            is_typedef, &symbol->type->attributes);
        symbol->type->attributes = merge_type_attributes(symbol->type->attributes, base->attributes);
        return 1;
    }
    node = extract_declarator_prefix(*work, prefix, symbol, is_typedef);
    *work = symbol->type->related;
    if (!(base_shape->shape->flags & 0x77) && prefix && *prefix)
        merge_type_shape_prefix(*prefix, base_shape->shape);
    attributes = node ? &node->attributes : &symbol->type->attributes;
    base_shape->shape = extract_base_prefix(base_shape->shape, &detached,
        symbol, is_typedef, attributes);
    if (node) {
        node->attributes = merge_type_attributes(node->attributes,
            (unsigned short)(base->attributes & 0xcfff));
        if (detached) merge_type_shape_prefix(node, detached);
    } else {
        symbol->type->attributes = merge_type_attributes(symbol->type->attributes, base->attributes);
        if (!*prefix && detached)
            *prefix = new_type_shape(detached->flags, detached->attributes, &detached->data);
    }
    return 1;
}

/* Original 0x48f80..0x48ff2. Detach the qualifier prefix after any initial
   function/array shapes, leaving the symbol's remaining chain mutable. */
TypeShape *__stdcall extract_declarator_prefix(TypeShape *shape, TypeShape **prefix,
    CppSymbol *symbol, int is_typedef)
{
    TypeShape *previous = 0;
    if (!shape) return 0;
    while (shape && ((shape->flags & 0x77) == 4 || (shape->flags & 0x77) == 2)) {
        previous = shape;
        shape = shape->related;
    }
    if (shape && !(shape->flags & 0x77)) {
        copy_shape_prefix(symbol, shape, prefix, is_typedef);
        if (previous) previous->related = 0;
        else symbol->type->related = 0;
        return previous;
    }
    return shape;
}

/* Original 0x49000..0x490cf. Prefix records are separated before recursively
   rebuilding the immutable base chain. */
TypeShape *__stdcall extract_base_prefix(TypeShape *shape, TypeShape **prefix,
    CppSymbol *symbol, int is_typedef, unsigned short *attributes)
{
    unsigned short flags, category, merged;
    TypeShape *related;
    while (shape && !(shape->flags & 0x77)) {
        if (!symbol->type->related)
            copy_shape_prefix(symbol, shape, prefix, is_typedef);
        else
            *prefix = shape;
        shape = shape->related;
    }
    if (!shape) return 0;
    flags = shape->flags;
    category = flags & 0x77;
    if (category != 4 && category != 2) {
        merged = merge_type_attributes((unsigned short)(*attributes & 0x3000), shape->attributes);
        related = intern_shape_on(shape->related, flags, merged, &shape->data);
        *attributes &= ~0x3000;
        return related;
    }
    if (shape->related) {
        if (category == 4) flags |= 0x400;
        merged = shape->attributes;
        related = extract_base_prefix(shape->related, prefix, symbol, is_typedef, attributes);
        shape = intern_shape_on(related, flags, merged, &shape->data);
    }
    return shape;
}

/* Original 0x62730..0x62756. */
TypeShape *__stdcall intern_shape_on(TypeShape *related, unsigned short kind,
    unsigned short attributes, void *parameter)
{
    TypeShape *shape = new_type_shape(kind, attributes, parameter);
    shape->related = related;
    return intern_type_shape(shape);
}

/* Original 0x493c0..0x4974a. Validate the result and combine the function's
   calling convention with the surrounding declarator. */
TypeShape *__stdcall merge_function_shape(CppSymbol *symbol, ShapeCursor inner,
    ShapeCursor *current, ShapeCursor outer, int is_typedef)
{
    TypeShape *extra = 0;
    CppSymbol *tag;
    QualifiedDeclarator *qualifier;
    unsigned category = inner.flags & 0x77;
    unsigned convention_bits;
    int pointer_follows = 0;
    int member_function = (current->flags & 0x2000) ||
        ((current->flags & 0x77) == 4 && (outer.flags & 0x10));
    char *convention;
    if (category == 2) {
        report_syntax_error(90);
        extra = new_type_shape(1, 0, 0);
    } else if (category == 4) {
        report_syntax_error(91);
        extra = new_type_shape(3, 0, 0);
    } else if (category == 1 && (inner.attributes & 8)) {
        report_syntax_error(263);
        extra = new_type_shape(1, 0, 0);
    } else if (!inner.flags && (symbol->type->flags & 0x400)) {
        tag = (CppSymbol *)cpp_context_stack->items[symbol->type->field08];
        if (tag->detail.class_info->flags & 0x80)
            report_syntax_error(259, tag->name);
        if (linkage_stack[linkage_depth] == LINKAGE_C)
            current->flags |= 0x100;
        else current->flags |= 0x200;
    }
    if ((outer.flags & 0x77) == 1 || (outer.flags & 0x50)) {
        pointer_follows = 1;
        current->attributes = merge_type_attributes(current->attributes,
            (unsigned short)(outer.attributes & 0xf77));
        if (outer.attributes & 12) outer.shape->attributes &= ~15;
    } else if (!outer.shape) {
        current->attributes = merge_type_attributes(current->attributes, outer.attributes);
        if (!inner.shape) current->attributes &= ~0x1000;
        if (symbol->flags & 4) {
            if (function_declaration_kind == 1) current->flags |= 0x4000;
            if (is_typedef)
                current->attributes = normalize_type_attributes(current->flags, current->attributes);
        }
    }
    convention_bits = current->attributes & 0x70;
    if (!convention_bits) {
        if (!(current->flags & 0x2000) && !(outer.flags & 0x10)) {
            convention = 0;
            if ((options.configuration.option_Gr || options.configuration.option_Gc ||
                 options.configuration.option_Gz) && symbol->name == name_main) {
                current->attributes = (current->attributes & ~0x70) | 0x10;
                convention = "cdecl";
            } else if ((options.configuration.option_Gr || options.configuration.option_Gc ||
                        options.configuration.option_Gd) &&
                       (symbol->name == name_winmain || symbol->name == name_dllmain)) {
                current->attributes = (current->attributes & ~0x70) | 0x60;
                convention = "__stdcall";
            }
            if (convention) compiler_warning(3, 7, symbol->name, convention);
        }
        qualifier = (QualifiedDeclarator *)symbol->next;
        if (member_function &&
            !(!outer.shape && qualifier &&
              (!qualifier->resolution || qualifier->resolution->binding->next) &&
              current_class_context < 0)) {
            if (current->flags & 0x8000)
                current->attributes = (current->attributes & ~0x70) | 0x10;
            else if ((symbol->storage[0] & 7) != 3 || (outer.flags & 0x10))
                current->attributes = (current->attributes & ~0x70) | 0x70;
        }
    } else if (!pointer_follows && (symbol->declaration_flags & 0x3000) &&
               !(token_allocation_flags & 0x400)) {
        if (convention_bits != 0x70 && convention_bits != 0x60)
            compiler_warning(1, 166);
        current->attributes = (current->attributes & ~0x70) |
            ((current->flags & 0x8000) ? 0x10 : 0x70);
    }
    if (current->attributes & 12) {
        report_syntax_error(212, symbol->name);
        current->attributes &= ~15;
        if (outer.shape && !(outer.flags & 0x77)) {
            outer.shape = 0;
            symbol->attributes &= ~2;
        }
    }
    if ((current->attributes & 0x70) == 0x60 && (current->flags & 0x8000))
        current->attributes &= ~0x70;
    return extra;
}

/* Original 0x49750..0x497e7. Classify a completed function declarator or
   supply the pointer shape required by a nonfunction symbol category. */
void __stdcall finish_function_declarator(CppSymbol *symbol, TypeShape **result)
{
    unsigned char kind = symbol->kind;
    unsigned short attributes;
    TypeShape *shape;
    if ((kind & 31) == 9) {
        symbol->kind = 7;
        return;
    }
    if (kind == 2) {
        report_syntax_error(656, format_symbol_name(symbol));
        symbol->kind = 7;
        return;
    }
    if (kind == 3) {
        symbol->kind = 7;
        return;
    }
    if (kind == 12) report_syntax_error(66);
    attributes = normalize_type_attributes(3,
        (unsigned short)(symbol->type->attributes & 15));
    symbol->type->attributes &= ~7;
    shape = new_type_shape(3, attributes, 0);
    shape->related = *result;
    *result = intern_type_shape(shape);
}

/* Original 0x49140..0x492e8. Fold a reference shape onto its referred type,
   validating calling conventions, pointer qualifiers and array boundaries.
   Both neighboring ShapeCursor values retain their eight-byte value ABI. */
int __stdcall merge_reference_shape(CppSymbol *symbol, ShapeCursor inner,
    ShapeCursor *current, ShapeCursor outer)
{
    unsigned convention;
    char *spelling = 0;
    if ((inner.flags & 0x77) == 4) {
        current->flags |= 4;
        current->flags |= inner.flags & 0xc800;
        if ((current->attributes & 0x70) == 0x60 && (current->flags & 0x8000))
            current->attributes &= ~0x70;
        current->attributes = merge_type_attributes(current->attributes, inner.attributes);
    } else {
        convention = current->attributes & 0x70;
        if (convention == 0x10) spelling = token_spellings[TOK_CDECL];
        else if (convention == 0x20) spelling = token_spellings[TOK_PASCAL];
        else if (convention == 0x40) spelling = token_spellings[TOK_FASTCALL];
        else if (convention == 0x60) spelling = token_spellings[TOK_STDCALL];
        if (spelling) report_syntax_error(532, spelling);
        if (current->attributes & 0x0f00) {
            compiler_warning(1, 105, symbol->name);
            current->attributes &= ~0x0f00;
        }
        if ((current->attributes & 7) == 2) {
            report_syntax_error(532, "_far");
            current->attributes &= ~7;
        }
    }
    if (inner.flags & 0x40) {
        report_syntax_error(529);
        current->flags = 1;
        symbol->declaration_flags |= 2;
    }
    if ((outer.flags & 0x77) == 2) {
        report_syntax_error(234);
        symbol->declaration_flags |= 2;
        return 0;
    }
    if ((inner.flags & 0x77) == 2) {
        if (inner.flags & 0x1000) report_syntax_error(266, format_symbol_name(symbol));
        else if (!inner.shape->data.array_bound) {
            report_syntax_error(265, format_symbol_name(symbol));
            symbol->declaration_flags |= 2;
        }
    }
    current->attributes = (current->attributes & ~0x3000) | (outer.attributes & 0x3000);
    return 1;
}
