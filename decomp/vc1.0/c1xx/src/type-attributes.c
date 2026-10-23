/* Calling conventions and segmented pointer attributes. */
#include "declarations.h"
#include <recon.h>

int active_plmn_pragma, active_plmf_pragma; /* original 0x9f42c/0x9f430 */

/* Original 0x3bc10..0x3becd. */
unsigned short __stdcall normalize_type_attributes(unsigned short kind, unsigned short attributes)
{
    unsigned category = kind & 0x77;
    int function = 0;
    char *conflict;
    if (options.prototypes_only)
        return attributes;
    if (category == 1 || category == 2 || category == 0x40) {
        if (!(attributes & 15))
            attributes |= (unsigned short)data_pointer_kind;
        return attributes;
    }
    if (category != 3 && category != 4 && category != 0x44)
        return attributes;
    function = category == 4;
    if (option_Au)
        attributes |= 0x800;
    if (!(attributes & 7))
        attributes |= (unsigned short)code_pointer_kind;
    if (!(attributes & 0x70)) {
        if (options.configuration.option_Gr || options.configuration.option_Gc) {
            if (attributes & 0x100) {
                compiler_warning(2, 96, "cdecl", "interrupt");
                goto default_cdecl;
            }
            if (kind & 0x8000) {
                compiler_warning(2, 96, "cdecl", "...");
                goto default_cdecl;
            }
        }
        if (options.configuration.option_Gr)
            attributes |= 0x40;
        else if (options.configuration.option_Gz)
            attributes |= (kind & 0x8000) ? 0x10 : 0x60;
        else if (options.configuration.option_Gc)
            attributes |= 0x20;
        else
            attributes |= 0x10;
    } else if (((attributes & 0x70) == 0x60 || (attributes & 0x70) == 0x40) &&
               (kind & 0x8000)) {
        attributes &= 0xff8f;
    default_cdecl:
        attributes |= 0x10;
    }
    if (!function)
        return attributes;
    if (attributes & 0x100) {
        if ((attributes & 0x70) != 0x50 && (attributes & 0x70) != 0x10)
            report_syntax_error(217, "interrupt", "cdecl");
        if (attributes & 0x200)
            report_syntax_error(216, "saveregs", "interrupt");
    }
    if ((attributes & 7) == 3 && (attributes & 0x70) != 0x20) {
        attributes &= 0xff8f;
        report_syntax_error(217, "__far16", "pascal");
        attributes |= 0x20;
    }
    if ((kind & 0x8000) && (attributes & 0x70) != 0x60 &&
        (attributes & 0x70) != 0x50 && (attributes & 0x70) != 0x10)
        report_syntax_error(217, "cdecl", "...");
    if ((attributes & 0x70) == 0x40) {
        conflict = 0;
        if (attributes & 0x200)
            conflict = "saveregs";
        else if (attributes & 0x100)
            conflict = "interrupt";
        else if (attributes & 0x400)
            conflict = "export";
        if (conflict) {
            report_syntax_error(216, "__fastcall", conflict);
            attributes &= 0xffbf;
            attributes |= 0x10;
        }
        if (active_plmf_pragma || active_plmn_pragma) {
            report_syntax_error(216, "__fastcall", "#pragma plmn/plmf()");
            active_plmn_pragma = active_plmf_pragma = 0;
        }
    }
    return attributes;
}


/* Original 0x61ee0..0x61fc5. Combine distance/calling-convention attributes,
   preserving the old distance in the upper bits for based pointers. */
unsigned short __stdcall merge_type_attributes(unsigned short left, unsigned short right)
{
    if (left & right & 0x3000) {
        if (options.configuration.extensions_enabled) compiler_warning(1, 114);
        else report_syntax_error(289);
    }
    if ((left ^ right) & 15) {
        if ((left & 4) != (right & 4)) {
            if (left & 4) left |= right << 14;
            else left = ((left | (left << 14)) & ~7) | (right & 15);
        } else if ((left & 15) && (right & 15)) {
            report_syntax_error(131);
            left &= ~7;
        }
    }
    if (((left ^ right) & 0x70) && (left & 0x70) && (right & 0x70)) {
        report_syntax_error(151);
        left &= ~0x70;
    }
    return left | right;
}

/* Original 0x3cd80..0x3ce2b. Retain the complete shape walk used by the
   original classifier, then select only its bare-class result bit. */
int __stdcall is_unqualified_class_type(CType *type)
{
    TypeShape *shape;
    unsigned classification;
    if (!type || !(type->flags & 0x400)) classification = 0;
    else {
        shape = type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        if (!shape) classification = 0x80;
        else if ((shape->flags & 0x40) && !shape->related) classification = 0x40;
        else if ((shape->flags & 0x77) == 2) {
            shape = shape->related;
            while (shape && (shape->flags & 0x77) == 2) shape = shape->related;
            classification = shape ? 0 : 2;
        } else classification = 0;
    }
    return classification & 0x80;
}

/* Original 0x3ce30..0x3ceb6. Return the class shape classification word:
   bare class 0x80, class reference 0x40, class array 2, other shape zero.
   The native function specifies AX; its unused EAX high half is incidental. */
unsigned short __stdcall classify_incomplete_type(CType *type)
{
    TypeShape *shape;
    if (!type || !(type->flags & 0x400)) return 0;
    shape = type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    if (!shape) return 0x80;
    if ((shape->flags & 0x40) && !shape->related) return 0x40;
    if ((shape->flags & 0x77) != 2) return 0;
    shape = shape->related;
    while (shape && (shape->flags & 0x77) == 2) shape = shape->related;
    return shape ? 0 : 2;
}

/* Original 0x635a0..0x635f2. Arrays/functions forward qualification to the
   next shape; a bare base uses the descriptor's own const/volatile bits. */
unsigned short __stdcall type_qualifiers(CType *type)
{
    TypeShape *shape = type->related;
    unsigned kind;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    while (shape) {
        kind = shape->flags & 0x77;
        if (kind != 2 && kind != 4) break;
        shape = shape->related;
    }
    return (unsigned short)((shape ? shape->attributes : type->attributes) & 0x3000);
}

/* Original 0x610a0..0x610ee. Prototype-only output accepts all attributes.
   Otherwise low pointer/model bits and bit 0x100 must agree; an unspecified
   incoming convention (bits 0x70 zero) accepts the earlier convention. */
int __stdcall equivalent_shape_attributes(unsigned short left, unsigned short right)
{
    unsigned short difference;
    if (options.prototypes_only) return 1;
    difference = left ^ right;
    if (difference & 15) return 0;
    if (difference & 0x100) return 0;
    if (!(right & 0x70)) return 1;
    return !(difference & 0x70);
}

/* Original 0x52990..0x52a65. Permit the unspecified incoming convention,
   then resolve unspecified pointer distance using the selected memory model.
   The final model-bit result is deliberately 0 or 4, rather than Boolean. */
unsigned __stdcall compatible_declaration_attributes(unsigned short left,
    unsigned short right, int function)
{
    unsigned short combined;
    if ((right ^ left) & 0x70) {
        if (right & 0x70) return 0;
        left &= ~0x70;
    }
    if ((right ^ left) & 0xfff8) return 0;
    if (right == left) return 1;
    if ((left & 7) && (right & 7)) return 0;
    combined = (right & 7) | (left & 7);
    if (function) {
        if (combined == 1) return !(options.configuration.option_M & 1);
        return (options.configuration.option_M & 1) == 1;
    }
    if (combined == 1) return !(options.configuration.option_M & 2);
    if (combined == 2) return (options.configuration.option_M & 2) == 2;
    return options.configuration.option_M & 4;
}

/* Original 0x636c0..0x636d8. The null chain has no final shape. */
TypeShape *__stdcall last_type_shape(TypeShape *shape)
{
    if (shape)
        while (shape->related) shape = shape->related;
    return shape;
}
