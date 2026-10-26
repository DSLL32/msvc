/* Map headings and symbol presentation use the actual output stream owner. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "link-driver.h"
#include "full-link-functions.h"
#include "sections.h"
#include "pass-one.h"
#include "debug-output.h"
#include <recon.h>

extern "C" {
extern const char *default_phase,*vxd_module_name;
extern char short_symbol_name[9];
extern FILE *map_output;
extern OutputModule *output_module;
extern BYTE mac_target,mapped_debug,incremental_enabled;
typedef struct MapStaticSymbol {
    struct MapStaticSymbol *next;
    char *name;
    Contribution *contribution;
    DWORD value;
    BYTE function;
} MapStaticSymbol;
typedef char MapStaticSymbolSize[(sizeof(MapStaticSymbol)==20)?1:-1];
/* Original4944a4 is the first pending static-symbol record. */
MapStaticSymbol *map_static_symbols;
/* Original4944a0, initially NULL. Its eventual producer owns the opaque nodes. */
void *map_thunk_symbols;
BYTE __stdcall is_discardable_comdat(Contribution *);
StringNode *__stdcall resolve_entry_symbol(LinkContext *);
void __stdcall write_map_sections(LinkContext *context)
{
    SectionEnumerator sections;
    GroupEnumerator groups;
    fputs(" Start         Length     Name                   Class\n",map_output);
    initialize_section_enumerator(&sections,&context->sections);
    while(next_output_section(&sections)) {
        OutputSection *section=sections.current;
        if(!section->index || (section==debug_section && !mapped_debug))continue;
        initialize_group_enumerator(&groups,section);
        while(next_contribution_group(&groups)) {
            ContributionGroup *group=groups.current;
            const char *kind=(section->characteristics&0x1e0)==0x20?"CODE":"DATA";
            DWORD size=group->size;
            if(mac_target) {
                if((section->characteristics&0x1e0)==0x20)size=(size+15)&~15U;
            } else if(context->is_vxd==1 && !size)continue;
            fprintf(map_output," %04x:%08lx %08lxH %-23s %s\n",
                (int)(SHORT)section->index,group->rva-section->rva,size,group->name,kind);
        }
    }
}
void __stdcall write_map_symbol(const char *name,Contribution *contribution,BYTE common,
    DWORD value,DWORD unused,DWORD image_base,BYTE function,OutputLibrary *command_line)
{
    char basename[256],extension[256];
    OutputSection *section=contribution->group->section;
    OutputModule *module=contribution->module;
    BYTE folded=function && (contribution->characteristics&0x1000) &&
        contribution->comdat_selection==IMAGE_COMDAT_SELECT_ANY;
    fprintf(map_output," %04x:%08lx       %-26s %08lx %c %c ",
        (int)(SHORT)section->index,value-section->rva+contribution->rva,name,
        image_base+value+contribution->rva,function?'f':' ',folded?'i':' ');
    if(common)fwrite("<common>",8,1,map_output);
    else if(module==output_module)fwrite("<linker-defined>",16,1,map_output);
    else {
        if(module->library!=command_line) {
            _splitpath(module->library->name,NULL,NULL,basename,NULL);
            fprintf(map_output,"%s:",basename);
        }
        _splitpath(module->name,NULL,NULL,basename,extension);
        fprintf(map_output,"%s%s",basename,extension);
    }
    fputc('\n',map_output);
}
void __stdcall write_map_static_symbols(LinkContext *context)
{
    if(incremental_enabled)return;
    fputs(" Static symbols\n\n",map_output);
    while(map_static_symbols) {
        MapStaticSymbol *next;
        if(!context->options.effective_reference_opt ||
           !is_discardable_comdat(map_static_symbols->contribution)) {
            Contribution *contribution=map_static_symbols->contribution;
            DWORD value=map_static_symbols->value;
            DWORD offset=contribution->rva-contribution->group->section->rva+value;
            write_map_symbol(map_static_symbols->name,contribution,0,value,offset,
                context->optional_header.ImageBase,map_static_symbols->function,
                context->command_line_library);
        }
        next=map_static_symbols->next;
        release_memory(map_static_symbols->name);
        release_memory(map_static_symbols);
        map_static_symbols=next;
    }
}
void __stdcall write_map_thunks(LinkContext *) { __asm { int 3 } }
int __cdecl compare_map_lines(const void *left,const void *right)
{
    const MapLineRecord *a=(const MapLineRecord *)left;
    const MapLineRecord *b=(const MapLineRecord *)right;
    if(a->section!=b->section)
        return (SHORT)a->section->index<(SHORT)b->section->index?-1:1;
    return (int)(a->offset-b->offset);
}
void __stdcall write_map_lines(LinkContext *context)
{
    LibraryEnumerator libraries;
    ModuleEnumerator modules;
    initialize_library_enumerator(&libraries,(OutputLibrary *)context->pointer_list.first);
    while(next_library(&libraries)) {
        initialize_module_enumerator(&modules,libraries.current);
        while(next_module(&modules)) {
            OutputModule *module=modules.current;
            MapLineFile *file=module->map_line_files;
            for(;file;file=file->next) {
                MapLineRecord *lines=(MapLineRecord *)file->lines.data;
                DWORD count=file->lines.used/sizeof(*lines),index,first=0;
                OutputSection *section=NULL;
                qsort(lines,count,sizeof(*lines),compare_map_lines);
                for(index=0;index<count;++index) {
                    if(lines[index].section!=section) {
                        const char *object=(module->library->flags&0x10)?
                            module->name:module->library->name;
                        section=lines[index].section;
                        first=index;
                        fprintf(map_output,"\nLine numbers for %s(%s) segment %s\n\n",
                            object,file->name,section->name);
                    }
                    if(index!=first && !((index-first)&3))fputc('\n',map_output);
                    fprintf(map_output,"%6u %04x:%08x",(unsigned)lines[index].line,
                        (int)(SHORT)lines[index].section->index,(unsigned)lines[index].offset);
                }
                fputc('\n',map_output);
            }
        }
    }
}
void __cdecl write_map_fixups(void)
{
    RelativeFixupBlock *block=relative_fixup_blocks;
    DWORD count=relative_fixup_count,previous=0,index;
    int column=0;
    fputc('\n',map_output);
    for(;block;block=block->next,count=32) {
        for(index=0;index<count;++index) {
            if(!column) {
                column=fprintf(map_output,"FIXUPS:");
                previous=0;
            }
            column+=fprintf(map_output," %lx",block->sites[index]-previous);
            previous=block->sites[index];
            if(column>=70) {
                fputc('\n',map_output);
                column=0;
            }
        }
    }
    if(column)fputc('\n',map_output);
    while(relative_fixup_blocks) {
        RelativeFixupBlock *next=relative_fixup_blocks->next;
        release_memory(relative_fixup_blocks);
        relative_fixup_blocks=next;
    }
}
void __stdcall write_map_exports(LinkContext *) { __asm { int 3 } }
}

#pragma intrinsic(strlen,strcpy)
extern "C" void __stdcall write_image_map(LinkContext *context,const char *output_name)
{
    char basename[256],encoded_name[84];
    char *date;
    DWORD saved_bytes=0,count,index,entry_offset=0;
    WORD entry_section=0,kind;
    StringNode **symbols,*entry;
    default_phase="EmitMap";
    if(context->is_vxd==1 && *vxd_module_name)
        fprintf(map_output," %s\n\n",vxd_module_name);
    else {
        _splitpath(output_name,NULL,NULL,basename,NULL);
        fprintf(map_output," %s\n\n",basename);
    }
    date=ctime((const time_t *)&context->file_header.TimeDateStamp);
    if(date)date[strlen(date)-1]=0;
    else date=(char *)"invalid";
    fprintf(map_output," Timestamp is %08lx (%s)\n\n",
        context->file_header.TimeDateStamp,date);
    fprintf(map_output," Preferred load address is %08lx\n\n",
        context->optional_header.ImageBase);
    write_map_sections(context);
    fputs("\n  Address         Publics by Value              Rva+Base     Lib:Object\n\n",
        map_output);
    symbols=context->symbols->GetSymbolsByValue();
    count=count_hash_symbols(context->symbols->hash.state);
    for(index=0;index<count;++index) {
        StringNode *symbol=symbols[index];
        Contribution *contribution=symbol->contribution;
        const char *name;
        if(contribution && (contribution->characteristics&0x4000))
            saved_bytes+=contribution->raw_size-contribution->padding;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        if(!(symbol->flags&1) || !contribution ||
           (contribution->characteristics&0x800))continue;
        if(context->options.effective_reference_opt &&
           is_discardable_comdat(contribution))continue;
        name=get_symbol_name(symbol,context->symbols->hash.string_storage,&kind,NULL);
        if(kind==1)name=strncpy(short_symbol_name,name,8);
        if((signed char)*name>=0x7f) {
            strcpy(encoded_name,"\\177");
            strcpy(encoded_name+4,name+1);
            name=encoded_name;
        }
        write_map_symbol(name,contribution,(BYTE)((symbol->flags>>1)&1),
            symbol->symbol.Value,0,context->optional_header.ImageBase,
            (BYTE)((symbol->symbol.Type&0x30)==0x20),context->command_line_library);
    }
    context->symbols->ReleaseLookupIndex();
    if(context->entry_name) {
        entry=resolve_entry_symbol(context);
        if(entry && (entry->flags&1)) {
            Contribution *contribution=entry->contribution;
            OutputSection *section=contribution->group->section;
            entry_section=section->index;
            entry_offset=entry->symbol.Value-section->rva+contribution->rva;
        }
    }
    fprintf(map_output,"\n entry point at        %04x:%08lx\n\n",entry_section,entry_offset);
    write_map_static_symbols(context);
    if(map_thunk_symbols)write_map_thunks(context);
    if(context->options.map_lines)write_map_lines(context);
    if(context->options.map_fixups)write_map_fixups();
    if(saved_bytes)fprintf(map_output,"\n %ld bytes saved through ICF\n\n",saved_bytes);
    if(!mac_target && context->options.map_exports && export_section)
        write_map_exports(context);
}
