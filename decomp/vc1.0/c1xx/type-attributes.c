/* Calling conventions and segmented pointer attributes. */
#include "declarations.h"

int active_plmn_pragma, active_plmf_pragma; /* original 0x9f42c/0x9f430 */

/* Original 0x3bc10..0x3becd. */
unsigned short __stdcall normalize_type_attributes(unsigned short kind, unsigned short attributes)
{
    unsigned category = kind & 0x77;
    int function = 0;
    char *conflict;
    if (options.syntax_only)
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
        if (options.configuration.z_a) compiler_warning(1, 114);
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
