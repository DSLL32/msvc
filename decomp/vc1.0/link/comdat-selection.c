/* Select an input COMDAT definition while retaining the COFF scan position. */
#include "object.h"
extern char *coff_strings;
Module *comdat_scan_module;
PIMAGE_SYMBOL comdat_scan_cursor;
unsigned long comdat_scan_index;

unsigned char select_comdat(SymbolTable *table,Contribution *contribution,
                            PIMAGE_SYMBOL symbols,unsigned short section,char **name)
{
    PIMAGE_SYMBOL symbol;
    PIMAGE_AUX_SYMBOL auxiliary;
    External *external;
    Contribution *previous;
    unsigned char have_auxiliary=0,associative=0,selected,selection,created,imports;
    unsigned long checksum,after_auxiliary=0;
    const char *key;
    char module_name[MAX_PATH],previous_name[MAX_PATH];
    format_module_name(contribution->module,module_name);
    if (!comdat_scan_module || comdat_scan_module!=contribution->module) {
        comdat_scan_module=contribution->module;
        comdat_scan_index=0;
        comdat_scan_cursor=symbols;
    }
    while (comdat_scan_index!=contribution->module->symbol_count) {
        symbol=next_coff_symbol(&comdat_scan_cursor,0);
        ++comdat_scan_index;
        if (symbol->SectionNumber!=(short)section) {
            comdat_scan_index+=skip_coff_auxiliaries(&comdat_scan_cursor,symbol->NumberOfAuxSymbols);
            continue;
        }
        if (!have_auxiliary) {
            have_auxiliary=1;
            if (symbol->NumberOfAuxSymbols) {
                auxiliary=(PIMAGE_AUX_SYMBOL)next_coff_symbol(&comdat_scan_cursor,1);
                ++comdat_scan_index;
                selection=auxiliary->Section.Selection;
                checksum=auxiliary->Section.CheckSum;
                if (selection==IMAGE_COMDAT_SELECT_ASSOCIATIVE) {
                    associative=1;
                    contribution->comdat_selection=IMAGE_COMDAT_SELECT_ASSOCIATIVE;
                    contribution->comdat.associated=
                        &contribution->module->contributions[auxiliary->Section.Number-1];
                    selected=contribution->comdat.associated->raw_size!=0;
                }
                comdat_scan_index+=skip_coff_auxiliaries(&comdat_scan_cursor,
                    (unsigned char)(symbol->NumberOfAuxSymbols-1));
                after_auxiliary=comdat_scan_index;
                if (associative) return selected;
                continue;
            }
            fatal_message(program_name,4,"Expected aux symbol for comdat");
        }
        comdat_scan_index=after_auxiliary;
        comdat_scan_cursor=symbols+after_auxiliary;
        if (symbol->StorageClass==IMAGE_SYM_CLASS_STATIC) {
            if (symbol->N.Name.Short)
                *name=strncpy(short_symbol_name,(const char *)symbol,8);
            else *name=coff_strings+symbol->N.Name.Long;
            return 1;
        }
        key=symbol->N.Name.Short ? (const char *)symbol : coff_strings+symbol->N.Name.Long;
        external=lookup_external(table,(unsigned short)(symbol->N.Name.Short!=0),key,&created,&imports);
        if (symbol->N.Name.Short)
            *name=strncpy(short_symbol_name,(const char *)symbol,8);
        else *name=coff_strings+symbol->N.Name.Long;
        previous=external->owner.contribution;
        if ((external->flags&EXTERNAL_DEFINED) && previous && previous->comdat_section &&
            !(external->flags&EXTERNAL_COMMON)) {
            /* This target uses the recorded section number for these checks. */
            if (previous->comdat_selection!=selection || previous->comdat_section==1 ||
                ((previous->comdat_section==3 || previous->comdat_section==4) &&
                 previous->comdat.checksum!=checksum)) {
                format_module_name(previous->module,previous_name);
                warning_message(module_name,0x4d,*name,previous_name);
            }
            return 0;
        }
        if (external->flags&EXTERNAL_COMMON) {
            external->flags &= ~EXTERNAL_COMMON;
            set_external_defined(external,0,table);
        }
        contribution->comdat_section=(short)section;
        contribution->comdat.checksum=checksum;
        contribution->comdat_selection=selection;
        external->flags |= EXTERNAL_COMDAT;
        return 1;
    }
    fatal_message(format_module_name(contribution->module,module_name),0x2e,(short)section);
    return 0;
}

