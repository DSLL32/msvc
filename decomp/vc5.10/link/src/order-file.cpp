#include <windows.h>
#include <stdio.h>
#include "../crt/crt-imports.h"
#include <string.h>
#include "link-driver.h"
#include "link.h"
#include "sections.h"
#include "symbols.h"
#include "comdat.h"
#include "memory.h"
#include "full-link-functions.h"
#include <recon.h>
extern "C" {
extern BYTE verbose_link,mac_target;
extern char short_symbol_name[9];
extern OutputModule *output_module;
void __stdcall initialize_hash_enumerator(HashState *);
void __stdcall end_hash_enumerator(HashState *);
char *__stdcall format_contribution_module(Contribution *,char *);
extern const char *order_file_name;
void __cdecl link_message(unsigned,...);
void __stdcall order_contribution_group(Contribution ***,DWORD *,DWORD *,const char *,LinkContext *)
{ __asm { int 3 } }
#pragma intrinsic(strlen,strcpy,strcat)
DWORD __stdcall read_order_file(StringTable *symbols,Contribution ***ordered,
                               LinkContext *context,const char *name)
{
    DWORD count=0,capacity=64,variant;
    char *buffer,*line,*text,*scan,*last,*alternate;
    FILE *file;
    BYTE decorate,quoted,whole_module,whole_group;
    StringNode *symbol;
    Contribution *contribution;
    OutputModule *module;
    ModuleContributionEnumerator contributions;
    *ordered=(Contribution **)allocate_memory(0x1000);
    buffer=(char *)allocate_memory(0x900);
    file=fopen(name,"rt");
    if(!file)link_fatal(NULL,1104,name);
    decorate=context->file_header.Machine==IMAGE_FILE_MACHINE_I386 ||
        context->file_header.Machine==0x1a2 ||
        context->file_header.Machine==0x1a6 ||
        context->file_header.Machine==0x601;
    /* The two leading bytes leave room for an inserted decoration prefix.
     * Keep the original allocation/read sizes; this case has short lines. */
    line=buffer+2;
    while(fgets(line,0x8ff,file)) {
        quoted=0;
        text=line;
        while(*text==' ' || *text=='\t')++text;
        whole_module=*text=='*';
        whole_group=*text=='&';
        if(whole_module || whole_group) {
            ++text;
            while(*text==' ' || *text=='\t')++text;
            if(*text=='"') { ++text; quoted=1; }
        }
        last=text;
        for(scan=text;*scan;++scan) {
            if((quoted && *scan=='"') || *scan=='\n' || *scan=='\r' ||
               (*scan==';' && !whole_module)) {
                *scan=0;
                break;
            }
            if(*scan!=' ' && *scan!='\t')last=scan+1;
        }
        *last=0;
        if(!*text)continue;
        if(whole_group) {
            order_contribution_group(ordered,&count,&capacity,text,context);
            continue;
        }
        if(whole_module) {
            module=context->command_line_library->FindModule(text,NULL);
            if(!module) { link_warning(name,4083,text); continue; }
            initialize_module_contribution_enumerator(&contributions,module);
            while(next_module_contribution(&contributions)) {
                contribution=contributions.current;
                if((contribution->characteristics&0x1e0)==IMAGE_SCN_CNT_CODE &&
                   contribution->rva!=1) {
                    contribution->group->has_ordered_contributions=1;
                    contribution->rva=1;
                    (*ordered)[count++]=contribution;
                    if(count==capacity) {
                        capacity*=2;
                        *ordered=(Contribution **)reallocate_memory(*ordered,capacity*4);
                    }
                }
            }
            continue;
        }
        if(decorate && *text!='?' && *text!='@')*--text='_';
        symbol=symbols->FindSymbol(text);
        if(!symbol || !(symbol->flags&1)) { link_warning(name,4037,text); continue; }
        if(!(symbol->flags&0x22)) {
            if(mac_target && (symbol->symbol.Type&0x8000)) {
                alternate=(char *)allocate_memory(strlen(text)+10);
                for(variant=0;variant<3;++variant) {
                    if(!variant) { strcpy(alternate,"__pcd"); strcat(alternate,"_tbl"); }
                    else if(variant==1)strcpy(alternate,"__nep");
                    else strcpy(alternate,"__fh");
                    strcat(alternate,text);
                    symbol=symbols->FindSymbol(alternate);
                    if(symbol) {
                        contribution=symbol->contribution;
                        while(contribution && (contribution->characteristics&0x4000))
                            contribution=contribution->associated_contribution;
                        contribution->group->has_ordered_contributions=1;
                        contribution->rva=1;
                        (*ordered)[count++]=contribution;
                        if(count==capacity) {
                            capacity*=2;
                            *ordered=(Contribution **)reallocate_memory(*ordered,capacity*4);
                        }
                    }
                }
            } else if(!(symbol->flags&0x40c0))link_warning(name,4065,text);
            continue;
        }
        contribution=symbol->contribution;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        contribution->group->has_ordered_contributions=1;
        /* Re-read the original symbol owner after the group write. */
        contribution=symbol->contribution;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        if(contribution->rva==1)link_warning(name,4097,text);
        contribution=symbol->contribution;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        contribution->rva=1;
        contribution=symbol->contribution;
        while(contribution && (contribution->characteristics&0x4000))
            contribution=contribution->associated_contribution;
        (*ordered)[count++]=contribution;
        if(count==capacity) {
            capacity*=2;
            *ordered=(Contribution **)reallocate_memory(*ordered,capacity*4);
        }
    }
    fclose(file);
    release_memory(buffer);
    return count;
}
}
void RECON_THISCALL LinkContext::PrintOrderFile(void)
{
    BYTE header=0,decorate;
    StringNode *symbol;
    Contribution *contribution;
    const char *name;
    WORD kind;
    char module_name[520];
    decorate=file_header.Machine==IMAGE_FILE_MACHINE_I386 ||
        file_header.Machine==0x1a2 || file_header.Machine==0x1a6 ||
        file_header.Machine==0x601;
    initialize_hash_enumerator(symbols->hash.state);
    while((symbol=symbols->NextSymbol())!=NULL) {
        contribution=symbol->contribution;
        if(!(symbol->flags&1) || !(symbol->flags&0x22) ||
           contribution->rva==1 ||
           (contribution->characteristics&0x1e0)!=IMAGE_SCN_CNT_CODE)continue;
        if(!header) {
            link_message(9024);
            fputc('\n',stdout);
            header=1;
        }
        name=get_symbol_name(symbol,symbols->hash.string_storage,&kind,NULL);
        if(kind==1)name=strncpy(short_symbol_name,name,8);
        if(decorate && *name=='_')++name;
        printf("        %s",name);
        if(contribution->module!=output_module)
            printf("\t\t; %s",format_contribution_module(contribution,module_name));
        fputc('\n',stdout);
    }
    if(header)fputc('\n',stdout);
    end_hash_enumerator(symbols->hash.state);
}

void RECON_THISCALL LinkContext::ApplyOrderFile(void)
{
    Contribution **ordered,*contribution,*next,*last,**link;
    DWORD count;
    SectionEnumerator sections_enum;
    GroupEnumerator groups;
    ContributionGroup *group;
    if(verbose_link) {
        fputc('\n',stdout);
        link_message(9022);
    }
    count=read_order_file(symbols,&ordered,this,order_file_name);
    initialize_section_enumerator(&sections_enum,&sections);
    while(next_output_section(&sections_enum)) {
        initialize_group_enumerator(&groups,sections_enum.current);
        while(next_contribution_group(&groups)) {
            group=groups.current;
            if(group->has_ordered_contributions) {
                link=&group->first;
                last=NULL;
                for(contribution=group->first;contribution;contribution=next) {
                    next=contribution->next;
                    /* Before LayoutSections, the original RVA slot is the
                     * order-selection marker. No second marker is introduced. */
                    if(!contribution->rva) {
                        *link=contribution;
                        last=contribution;
                        link=&contribution->next;
                    }
                }
                *link=NULL;
                group->last=last;
            }
        }
    }
    if(verbose_link)PrintOrderFile();
    while(count) {
        contribution=ordered[--count];
        if(contribution->rva) {
            group=contribution->group;
            contribution->rva=0;
            contribution->next=group->first;
            group->first=contribution;
            if(!group->last)group->last=contribution;
        }
    }
    release_memory(ordered);
    if(verbose_link)link_message(9023);
}
