/* Native-subsystem linker-defined image-base and section-number symbols. */
#include <windows.h>
#include <string.h>
#include "link-driver.h"
#include "sections.h"
#include "full-link-functions.h"
#include "memory.h"
#include "files.h"
#include <recon.h>

typedef struct SectionNumberTarget {
    OutputSection *section;
    SectionNumberReference *next;
} SectionNumberTarget;
struct SectionNumberReference {
    StringNode *symbol;
    SectionNumberTarget *target;
};
typedef char SectionNumberReferenceSize[(sizeof(SectionNumberReference)==8 &&
    sizeof(SectionNumberTarget)==8)?1:-1];

typedef struct UndefinedEnumerator {
    BYTE (__stdcall *next)(struct UndefinedEnumerator *);
    void (__stdcall *end)(struct UndefinedEnumerator *);
    StringNode *next_symbol,*current;
} UndefinedEnumerator;
extern "C" {
extern char short_symbol_name[9];
extern OutputModule *output_module;
extern DWORD fixup_count;
extern int output_file;
extern BYTE file_access_statistics;
extern BYTE *next_base_relocation;
void __cdecl record_file_access(void);
DWORD __stdcall local_import_relocation_type(LinkContext *);
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
void __stdcall initialize_undefined_enumerator(UndefinedEnumerator *,StringTable *);
BYTE __stdcall next_undefined(UndefinedEnumerator *);
void __stdcall define_input_symbol(StringNode *,Contribution *,DWORD,SHORT,WORD,DWORD,
                                  OutputModule *,StringTable *);
}

#pragma intrinsic(strcmp,strcpy)
void RECON_THISCALL LinkContext::CollectRelocationSymbols(Contribution **section_numbers,
    SectionNumberReference **section_symbols,Contribution **image_base,IncludeSymbol **base_symbol)
{
    UndefinedEnumerator enumerator;
    StringNode *symbol;
    const char *name,*undecorated;
    WORD kind;
    char section_name[256];
    OutputSection *section;
    DWORD offset;
    SectionNumberReference **last=section_symbols;
    *section_numbers=NULL;
    *image_base=NULL;
    *section_symbols=NULL;
    *base_symbol=NULL;
    initialize_undefined_enumerator(&enumerator,symbols);
    while(next_undefined(&enumerator)) {
        symbol=enumerator.current;
        if(symbol->flags&0x100)continue;
        name=get_symbol_name(symbol,symbols->hash.string_storage,&kind,NULL);
        if(kind==1)name=strncpy(short_symbol_name,name,8);
        undecorated=name;
        if(file_header.Machine==IMAGE_FILE_MACHINE_I386 ||
            file_header.Machine==0x1a2 || file_header.Machine==0x1a6)++undecorated;
        if(!strncmp(undecorated,"_SectionNumber_",15)) {
            section=find_output_section(undecorated+15,&sections);
            if(!section) {
                strcpy(section_name,".");
                strncat(section_name,undecorated+16,255);
                section=find_output_section(section_name,&sections);
                if(!section)continue;
            }
            if(!*section_numbers) {
                *section_numbers=create_contribution(".rdata",0,0x40300040,0x40300040,
                    output_module,&sections,this);
                if(options.effective_reference_opt || options.fold_identical_comdats)
                    attach_comdat(*section_numbers,NULL,1);
            }
            offset=(*section_numbers)->raw_size;
            (*section_numbers)->raw_size=offset+4;
            define_input_symbol(symbol,*section_numbers,offset,0,0,0,output_module,symbols);
            *last=(SectionNumberReference *)allocate_memory(8);
            (*last)->symbol=symbol;
            (*last)->target=(SectionNumberTarget *)allocate_memory(8);
            (*last)->target->section=section;
            last=&(*last)->target->next;
        } else if(!strcmp(undecorated,"_ImageBase_")) {
            *image_base=create_contribution(".rdata",4,0x40300040,0x40300040,
                output_module,&sections,this);
            if(options.effective_reference_opt || options.fold_identical_comdats)
                attach_comdat(*image_base,NULL,1);
            define_input_symbol(symbol,*image_base,0,0,0,0,output_module,symbols);
            *base_symbol=(IncludeSymbol *)allocate_memory(8);
            (*base_symbol)->symbol=symbol;
            (*base_symbol)->next=NULL;
            ++fixup_count;
            if(!options.fixed_image && !options.rom)
                ++optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
        }
    }
    *last=NULL;
}

void RECON_THISCALL LinkContext::PrepareNativeRelocations(void)
{
    SectionEnumerator enumerator;
    OutputSection *initialization,*section;
    initialization=create_output_section(NULL,"INIT",0xe2000020,&sections,&optional_header);
    initialize_section_enumerator(&enumerator,&sections);
    while(next_output_section(&enumerator)) {
        section=enumerator.current;
        if(!strcmp(section->name,"INIT")) {
            section->characteristics|=IMAGE_SCN_MEM_DISCARDABLE;
            if(section!=initialization)merge_section_groups(section,initialization);
        } else if(!(section->characteristics&IMAGE_SCN_MEM_DISCARDABLE) &&
            strncmp(section->name,"PAGE",4) && strcmp(section->name,".edata"))
            section->characteristics|=IMAGE_SCN_MEM_NOT_PAGED;
    }
    end_section_enumerator(&enumerator);
}

#define WRITE_SYMBOL_VALUE(value) do { \
    int file=output_file; \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[file]; \
    if(record->flags&8)mapped_write(record,&(value),sizeof(DWORD)); \
    else buffered_write(record,&(value),sizeof(DWORD)); \
} while(0)

void RECON_THISCALL LinkContext::WriteRelocationSymbols(Contribution *section_numbers,
    SectionNumberReference **section_symbols,Contribution *image_base,IncludeSymbol **base_symbol)
{
    SectionNumberReference *section_reference;
    StringNode *symbol;
    FileRecord *record;
    LinkBaseRelocation *relocation;
    DWORD value,error;
    if(*section_symbols) {
        error=0;
        seek_file(output_file,section_numbers->file_offset,FILE_BEGIN,&error);
        while(*section_symbols) {
            section_reference=*section_symbols;
            symbol=section_reference->symbol;
            value=(LONG)(SHORT)resolve_output_section(section_reference->target->section)->index;
            WRITE_SYMBOL_VALUE(value);
            symbol->final_value=symbol->contribution->rva+symbol->symbol.Value;
            section_reference=*section_symbols;
            *section_symbols=section_reference->target->next;
            release_memory(section_reference->target);
            release_memory(section_reference);
        }
    }
    if(*base_symbol) {
        error=0;
        seek_file(output_file,image_base->file_offset,FILE_BEGIN,&error);
        value=optional_header.ImageBase;
        WRITE_SYMBOL_VALUE(value);
        symbol=(*base_symbol)->symbol;
        symbol->final_value=symbol->contribution->rva+symbol->symbol.Value;
        if(options.debug_formats&8)
            record_debug_fixup((WORD)local_import_relocation_type(this),0,symbol->final_value,0);
        if(!options.rom && !options.fixed_image) {
            relocation=(LinkBaseRelocation *)next_base_relocation;
            relocation->type=IMAGE_REL_BASED_HIGHLOW;
            relocation->address=symbol->final_value;
            relocation->section=symbol->symbol.SectionNumber;
            relocation->value=0;
            next_base_relocation+=sizeof(*relocation);
        }
        release_memory(*base_symbol);
        *base_symbol=NULL;
    }
}
#undef WRITE_SYMBOL_VALUE
