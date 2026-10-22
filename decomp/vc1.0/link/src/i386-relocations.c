/* i386 COFF relocation application and optional base/debug fixup records. */
#include "machine.h"
#include "pass-two.h"
#include <recon.h>

void apply_i386_relocations(Contribution *contribution,void *data,
                            PIMAGE_SYMBOL symbols,Image *image)
{
    PIMAGE_RELOCATION relocation=contribution->relocations;
    PIMAGE_SYMBOL symbol;
    unsigned long remaining=contribution->relocation_count,offset,rva,value,address;
    unsigned char debug=contribution->group->section==debug_section,absolute,found;
    char *fixup;
    Section *target_section;
    SectionEnumerator sections;
    while (remaining--) {
        offset=relocation->VirtualAddress-contribution->input_rva;
        rva=contribution->rva+offset;
        fixup=(char *)data+offset;
        symbol=&symbols[relocation->SymbolTableIndex];
        value=symbol->Value;
        address=value;
        absolute=symbol->SectionNumber==-1;
        if (!absolute) address+=image->optional.ImageBase;
        if ((image->options.debug_types&0x10) && !absolute && !debug)
            record_debug_fixup(relocation->Type,rva,value);
        switch (relocation->Type) {
        case IMAGE_REL_I386_ABSOLUTE:
            break;
        case IMAGE_REL_I386_DIR32:
            if (!debug) {
                if (!image->options.fixed) {
                    base_fixup_next->type=absolute?IMAGE_REL_BASED_ABSOLUTE:IMAGE_REL_BASED_HIGHLOW;
                    base_fixup_next->rva=absolute?0:rva;
                    base_fixup_next->value.adjustment=0;
                    ++base_fixup_next;
                }
                *(unsigned long *)fixup+=address;
            } else {
                found=0;
                initialize_section_enumerator(&sections,&image->sections);
                while (next_section(&sections)) {
                    target_section=sections.current;
                    if (value>=target_section->rva &&
                        value<target_section->rva+target_section->raw_size) {
                        found=1;
                        end_section_enumerator(&sections);
                        break;
                    }
                }
                *(unsigned long *)fixup+=value;
                if (found && !absolute) {
                    *(unsigned long *)fixup-=target_section->rva;
                    *(unsigned short *)(fixup+4)+=target_section->index;
                }
            }
            break;
        case IMAGE_REL_I386_DIR32NB:
            *(unsigned long *)fixup+=value;
            break;
        case IMAGE_REL_I386_SEG12:
            warning_message(contribution->module->path,0x7e,relocation->VirtualAddress,"");
            break;
        case IMAGE_REL_I386_SECTION:
            if (absolute) *(unsigned short *)fixup+=image->file.NumberOfSections+1;
            else {
                /* The original SECTION case retains the previous found flag.
                 * Defined section references set it when locating their RVA. */
                initialize_section_enumerator(&sections,&image->sections);
                while (next_section(&sections)) {
                    target_section=sections.current;
                    if (value>=target_section->rva &&
                        value<target_section->rva+target_section->raw_size) {
                        found=1;
                        end_section_enumerator(&sections);
                        break;
                    }
                }
                if (found) *(unsigned short *)fixup+=target_section->index;
            }
            break;
        case IMAGE_REL_I386_SECREL:
            found=0;
            initialize_section_enumerator(&sections,&image->sections);
            while (next_section(&sections)) {
                target_section=sections.current;
                if (value>=target_section->rva &&
                    value<target_section->rva+target_section->raw_size) {
                    found=1;
                    end_section_enumerator(&sections);
                    break;
                }
            }
            *(unsigned long *)fixup+=value;
            if (found && target_section && !absolute)
                *(unsigned long *)fixup-=target_section->rva;
            break;
        case IMAGE_REL_I386_REL32:
            *(unsigned long *)fixup+=value-4+contribution->input_rva-
                                     relocation->VirtualAddress-contribution->rva;
            if (image->options.map_format) record_map_relocation(rva);
            break;
        default:
            warning_message(contribution->module->path,0x4f,
                            relocation->Type,relocation->VirtualAddress);
            break;
        }
        ++relocation;
    }
}

