/* Declaration storage, linkage and type normalization. */
#include "declarations.h"
#include "cpp-context.h"
#include "operators.h"
#include "parameters.h"

void *deferred_declarations[60]; /* original 0x913cc; bounded declaration depth */
int current_class_context = -1; /* original 0x9fbe8 */
int allow_incomplete_class; /* original 0x9fc80 */

#define CLASS_SYMBOL(type_) ((CppSymbol *)cpp_context_stack->items[(type_)->field08])
#define STORAGE_KIND (storage & 7)

/* Original 0x52ab0..0x53534; switch tables 0x53534..0x53570. */
void __stdcall normalize_symbol_type(CType *base, CppSymbol *symbol)
{
    CType *type = symbol->type;
    TypeShape *shape;
    CppSymbol *class_symbol;
    QualifiedDeclarator *qualifier;
    unsigned short storage = symbol->storage[0] | (symbol->storage[1] << 8);
    unsigned char extended = symbol->storage[2];
    unsigned short attributes;
    unsigned kind;
    int invalid_type = 0, member;
    char *name = symbol->name, *modifier, *display, *class_display;

    if (deferred_declarations[declaration_depth])
        check_qualified_member_access(deferred_declarations[declaration_depth]);
    qualifier = (QualifiedDeclarator *)symbol->next;
    member = !(symbol->attributes & 0x40) &&
        ((current_class_context >= 0 && !(storage & 0x400)) ||
         (qualifier && (!qualifier->resolution || qualifier->resolution->binding->next)));
    if (base) {
        type->field08 = base->field08;
        type->flags = base->flags;
        merge_declaration_type(symbol, base, STORAGE_KIND == 7);
        shape = symbol->type->related;
        if (shape && !(shape->flags & 0x77))
            shape = shape->related;
        if (!shape && STORAGE_KIND != 7) {
            kind = symbol->kind & 31;
            if (symbol->type->flags == 0x80) {
                if (kind != 12 && kind != 11) {
                    report_syntax_error(182, name);
                    symbol->type->flags = 0x64;
                }
                invalid_type = 1;
            } else if (symbol->type->flags & 0x400) {
                class_symbol = CLASS_SYMBOL(symbol->type);
                if ((class_symbol->detail.class_info->flags & 0x80) &&
                    !(symbol->declaration_flags & 2) && !allow_incomplete_class) {
                    report_syntax_error(259, class_symbol->name);
                    symbol->declaration_flags |= 2;
                }
                if (STORAGE_KIND != 1 && !(class_symbol->flags & 4) &&
                    kind != 12 && kind != 11 && kind != 10 &&
                    (symbol->storage[0] & 7) != 3) {
                    if (is_current_class(class_symbol))
                        report_syntax_error(460, name, CLASS_SYMBOL(symbol->type)->name);
                    else
                        report_syntax_error(79, name, CLASS_SYMBOL(symbol->type)->name);
                    symbol->type = type_0064;
                }
            }
        }
    }
    type = symbol->type;
    attributes = type->attributes;
    if (symbol->declaration_flags & 2)
        storage |= 8;
    if ((symbol->kind & 31) == 7) {
        shape = type->related;
        if (shape && !(shape->flags & 0x77))
            shape = shape->related;
        if (!(storage & 8)) {
            invalid_type = 0;
            if (shape->flags & 0x200)
                symbol->cpp.flags |= 0x800000;
            else if ((shape->flags & 0x100) && linkage_stack[linkage_depth] == LINKAGE_C) {
                class_symbol = CLASS_SYMBOL(symbol->type);
                if (!(class_symbol->declaration_flags & 0x200) &&
                    (!(class_symbol->flags & 4) || !(class_symbol->detail.class_info->flags2 & 1))) {
                    class_display = format_symbol_name(class_symbol);
                    display = format_symbol_name(symbol);
                    report_syntax_error(526, display, class_display);
                    invalid_type = 1;
                }
            }
            if (storage & 0x200) {
                if (!member) {
                    report_syntax_error(575, symbol->name);
                    storage &= ~0x200;
                } else if (symbol->name == name_new || symbol->name == name_delete) {
                    report_syntax_error(650, symbol->name);
                    storage &= ~0x200;
                    invalid_type = 1;
                }
            }
            if (storage & 0x400) {
                if (current_class_context < 0) {
                    report_syntax_error(255, symbol->name);
                    storage &= ~0x400;
                    invalid_type = 1;
                }
            } else if (STORAGE_KIND == 1 && member && !(storage & 0x20)) {
                display = format_symbol_name(symbol);
                if (current_class_context < 0 && options.configuration.extensions_enabled)
                    compiler_warning(1, 630, display, token_spellings[13]);
                else
                    report_syntax_error(720, display, token_spellings[13]);
                storage &= ~7;
            }
            if (STORAGE_KIND == 3) {
                if (symbol->declaration_flags & 0x2000) {
                    report_syntax_error(574, format_symbol_name(symbol));
                    invalid_type = 1;
                } else if (member)
                    symbol->cpp.flags &= ~0x10000;
                else if (declaration_mode)
                    report_syntax_error(267, name);
            }
            if (!(storage & 0x10)) {
                if (symbol->declaration_flags & 0x2000) {
                    report_syntax_error(631, format_symbol_name(symbol));
                    invalid_type = 1;
                } else if (symbol->declaration_flags & 0x1000) {
                    report_syntax_error(533, format_symbol_name(symbol));
                    invalid_type = 1;
                }
                if (invalid_type) {
                    symbol->type->flags = 0x80;
                    symbol->type->field08 = 0;
                }
            } else if (symbol->declaration_flags & 0x3000)
                symbol->type->flags = 0x80;
            if (invalid_type) {
                storage = (storage & ~7) | 8;
                symbol->declaration_flags |= 2;
            }
        }
        shape = type->related;
        if (shape && !(shape->flags & 0x77))
            shape = shape->related;
        attributes = (attributes & 0xf488) | (shape->attributes & 0xb77);
        switch (STORAGE_KIND) {
        case 3:
            if (storage & 0x200) {
                report_syntax_error(576, symbol->name);
                storage &= ~0x200;
            }
            if ((extended & 4) && !member) {
                report_syntax_error(201, name);
                storage = (storage & ~6) | 0x21;
            }
            if (declaration_mode) {
                report_declaration_diagnostic(205, 201, name);
                storage = (storage & ~6) | 0x21;
            }
            break;
        case 4:
        case 5:
        case 6:
            compiler_warning(1, 42, name);
            /* Fall through to a function declaration's external storage. */
        case 0:
        case 2:
            storage = (storage & ~6) | 0x21;
            break;
        case 7:
            if (symbol->flags & 4) {
                report_syntax_error(71, name);
                storage = (storage & ~6) | 1;
            }
            symbol->kind = member ? 0x29 : 9;
            break;
        }
        goto finish;
    }

    kind = symbol->kind & 31;
    if (((STORAGE_KIND >= 4 && STORAGE_KIND <= 6) || STORAGE_KIND == 1 ||
         STORAGE_KIND == 2 || ((extended & 4) && STORAGE_KIND != 3)) &&
        (kind == 3 || kind == 2) || (STORAGE_KIND == 3 && kind == 2)) {
        report_syntax_error(71, symbol->name);
        storage = (storage & ~7) | 8;
        symbol->declaration_flags |= 2;
    }
    modifier = 0;
    if (storage & 0x200) { modifier = "virtual"; storage &= ~0x200; }
    if (storage & 0x400) {
        modifier = "friend";
        storage = (storage & ~0x400) | 8;
        member = 1;
        symbol->kind = 3;
        symbol->declaration_flags |= 2;
    }
    if (storage & 0x100) { modifier = "inline"; storage &= ~0x100; }
    if (modifier) {
        switch (symbol->kind & 31) {
        case 11: report_syntax_error(735, modifier); break;
        case 12: report_syntax_error(736, modifier); break;
        default: report_syntax_error(433, symbol->name, modifier); break;
        }
    }
    if ((symbol->kind & 31) == 12) {
        switch (STORAGE_KIND) {
        case 1: modifier = "extern"; break;
        case 3: modifier = "static"; break;
        case 4: modifier = "auto"; break;
        case 5: modifier = "register"; break;
        case 7: modifier = "typedef"; break;
        default: modifier = 0; break;
        }
        storage &= ~7;
        if (modifier)
            report_syntax_error(736, modifier);
    }
    if (!options.prototypes_only && !invalid_type && !(attributes & 0x70)) {
        if (options.configuration.option_Gz)
            attributes |= 0x60;
        else
            attributes |= options.configuration.option_Gc ? 0x20 : 0x10;
    } else if ((attributes & 0x70) == 0x40) {
        compiler_warning(2, 8, symbol->name, token_spellings[44]);
        attributes = (attributes & ~0x70) | 0x10;
    }
    if (attributes & 0xb00) {
        compiler_warning(1, 105, name);
        attributes &= ~0xb00;
    }
    if (STORAGE_KIND == 5 && type->related && (type->related->attributes & 8)) {
        compiler_warning(1, 42, name);
        storage &= ~7;
    }
    kind = symbol->kind & 31;
    if (kind == 10 || kind == 11) {
        if (STORAGE_KIND != 5) {
            if (STORAGE_KIND)
                compiler_warning(1, 42, name);
            storage = (storage & ~3) | 4;
        }
        if ((storage & 0x700) || (extended & 7)) {
            compiler_warning(1, 42, name);
            extended = declaration_storage[2];
            storage = ((declaration_storage[0] | (declaration_storage[1] << 8)) & ~7) |
                STORAGE_KIND;
        }
    } else if (kind == 9) {
        if (!(token_allocation_flags & 0x80000)) {
            if (STORAGE_KIND >= 4 && STORAGE_KIND <= 6) {
                report_syntax_error(390, name, token_spellings[STORAGE_KIND >= 5 ? 14 : 11]);
                storage = (storage & ~5) | 2;
            } else if (!STORAGE_KIND) {
                if (type_qualifiers(type) & 0x1000)
                    storage = (storage & ~4) | 3;
                else if ((symbol->attributes & 0x500) == 0x500 && !(symbol->attributes & 4))
                    storage = (storage & ~6) | 1;
                else
                    storage = (storage & ~5) | 2;
            }
        } else if (!STORAGE_KIND) {
            if (token_allocation_flags & 0x40)
                storage = (storage & ~5) | 2;
            else if ((extended & 4) && !(extended & 8))
                storage = (storage & ~6) | 1;
            else
                storage = (storage & ~3) | 4;
        }
    }
    switch (STORAGE_KIND) {
    case 3:
        kind = symbol->kind & 31;
        if ((extended & 4) && kind != 2 && kind != 3) {
            report_syntax_error(201, name);
            extended &= ~12;
            storage |= 8;
        }
        /* Fall through: these storage classes may contain a based shape. */
    case 1:
    case 2:
        shape = type->related;
        if (!shape || (shape->flags & 0x77))
            break;
        attributes = (attributes & ~7) | 2;
        if ((shape->attributes & 7) < 5) {
            report_syntax_error(185, name);
        } else if ((((CppSymbol *)shape->data.based_symbol)->kind & 31) == 13) {
            if (((CppSymbol *)shape->data.based_symbol)->index != 6)
                break;
            report_syntax_error(185, name);
        } else {
            if (STORAGE_KIND == 1 && !(symbol->attributes & 4))
                break;
            report_syntax_error(154, name);
        }
        /* Fall through to repair the invalid distance. */
    case 0:
    case 4:
    case 5:
    case 6:
        if ((symbol->kind & 31) != 12)
            attributes = normalize_symbol_distance(attributes, symbol);
        break;
    }
    if ((extended & 4) && (token_allocation_flags & 0x80000) &&
        !(token_allocation_flags & 0x40) && STORAGE_KIND != 1)
        report_syntax_error(71, format_symbol_name(symbol));
finish:
    if (STORAGE_KIND != 7)
        type->attributes = attributes;
    symbol->storage[0] = (unsigned char)storage;
    symbol->storage[1] = (unsigned char)(storage >> 8);
    symbol->storage[2] = extended;
    if (!member)
        symbol->declaration_flags = (symbol->declaration_flags & ~0x300) |
            ((linkage_stack[linkage_depth] & 3) << 8);
    if (!type->related || !(type->related->flags & 0x1000))
        symbol->type = intern_type((TypeDescription *)type);
}
#undef STORAGE_KIND
#undef CLASS_SYMBOL

/* Original 0x52900..0x52983. Normalize explicit object-distance attributes. */
unsigned short __stdcall normalize_symbol_distance(unsigned short attributes, CppSymbol *symbol)
{
    unsigned kind;
    if (symbol->attributes & 2) {
        symbol->type = strip_based_shape(symbol->type);
        symbol->attributes &= ~2;
    }
    if (attributes & 15) {
        if (classify_incomplete_type(symbol->type) & 0x82) {
            kind = symbol->kind & 31;
            if ((option_Au || option_Aw) && (attributes & 7) == 1 &&
                (kind == 10 || kind == 11))
                attributes = (attributes & ~7) | 2;
            return attributes;
        }
        warn_ignored_distance(symbol);
        attributes &= 0xfbf8;
    }
    return attributes;
}
