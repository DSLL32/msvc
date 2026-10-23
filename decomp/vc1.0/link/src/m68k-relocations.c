/* Macintosh COFF relocations: big-endian fields, A5 thunks and paired deltas. */
#include <string.h>
#include "machine.h"
#include "object.h"
#include "link-pass.h"
#include "mac.h"
#include <recon.h>

extern SymbolTable *mac_symbol_table;
extern void **mac_object_symbols;
extern Library *mac_segment_library;
void resolve_mac_relocation_symbol(External **,PIMAGE_SYMBOL *,unsigned short,Contribution *);
void validate_mac_relocation_value(PIMAGE_RELOCATION,unsigned long,unsigned char);
unsigned long find_mac_static_jump(unsigned short,PIMAGE_SYMBOL,unsigned char);
void apply_mac_special_relocation(unsigned char *,const char *,PIMAGE_SYMBOL,Contribution *,External *);

#define MAC_READ_WORD(p) ((unsigned short)(((unsigned short)(p)[0]<<8)|(p)[1]))
#define MAC_READ_LONG(p) (((unsigned long)(p)[0]<<24)|((unsigned long)(p)[1]<<16)| \
    ((unsigned long)(p)[2]<<8)|(p)[3])
#define MAC_STORE_WORD(p,x) do { (p)[0]=(unsigned char)((unsigned long)(x)>>8); \
    (p)[1]=(unsigned char)(x); } while (0)
#define MAC_STORE_LONG(p,x) do { (p)[0]=(unsigned char)((unsigned long)(x)>>24); \
    (p)[1]=(unsigned char)((unsigned long)(x)>>16); \
    (p)[2]=(unsigned char)((unsigned long)(x)>>8); (p)[3]=(unsigned char)(x); } while (0)
#define MAC_REQUIRE_CODE() do { if (!(flags&0x20)) fatal_message(path,0x3e,group_name); } while (0)
#define MAC_REQUIRE_DATA() do { if (!(flags&0xc0)) fatal_message(path,0x3f,group_name); } while (0)
#define MAC_NONZERO_ADDEND(x) do { if (x) warning_message(path,0x87,name,""); } while (0)

/* Original code [00037e80,00038b81), switch data [00038b84,00038c48).
 * The common relocation-callback Image argument is unused by this backend. */
void apply_m68k_relocations(Contribution *contribution,void *data,
    PIMAGE_SYMBOL symbols,Image *image)
{
    Section *section=contribution->group->section;
    PIMAGE_RELOCATION relocation=contribution->relocations,pair=NULL;
    PIMAGE_SYMBOL symbol;
    External *external;
    Module *duplicate;
    Contribution *copy;
    unsigned char *site,standalone;
    unsigned char unpaired=1;
    unsigned short resource=section->index,target_resource,pair_resource=0,type;
    unsigned long flags=contribution->flags,remaining=contribution->relocation_count;
    unsigned long pc,value,kind,width,pair_address=0,pair_value=0;
    long signed_value,magnitude;
    char group_name[9],*name,*path=contribution->module->path;

    strncpy(group_name,contribution->group->name,8);
    group_name[8]=0;
    standalone=(section->characteristics&0x20) && section->mac_type!=0x45444f43;
    while (remaining--) {
        site=(unsigned char *)data+relocation->VirtualAddress-contribution->input_rva;
        pc=relocation->VirtualAddress+contribution->rva-section->rva-contribution->input_rva;
        symbol=&symbols[relocation->SymbolTableIndex];
        target_resource=(unsigned short)symbol->SectionNumber;
        if (!symbol->N.Name.Short)
            name=mac_symbol_table->strings.data+symbol->N.Name.Long;
        else name=strncpy(short_symbol_name,(char *)symbol,8);
        if (symbol->StorageClass==2 || symbol->StorageClass==0x44)
            external=mac_object_symbols[relocation->SymbolTableIndex];
        else external=NULL;
        if ((external?external->symbol.Type:symbol->Type)&0x8000)
            resolve_mac_relocation_symbol(&external,&symbol,relocation->Type,contribution);
        if (standalone) {
            if (resource!=target_resource)
                fatal_message(program_name,4,"sacode ref to another segment");
            else if (relocation->Type==8) relocation->Type=7;
            else if (relocation->Type==12) relocation->Type=11;
        }
        validate_mac_relocation_value(relocation,symbol->Value,standalone);
        type=relocation->Type;
        switch (type) {
        case 0: case 6:
            if (!type) { MAC_REQUIRE_DATA(); }
            else { MAC_REQUIRE_CODE(); }
            value=symbol->Value+(long)(short)MAC_READ_WORD(site);
            if ((long)value<-32768) fatal_message(path,0x3b,name);
            goto store_word;
        case 1:
            MAC_REQUIRE_DATA();
            value=external?external->mac_jump_offset+2:
                find_mac_static_jump(target_resource,symbol,0);
            MAC_NONZERO_ADDEND(MAC_READ_WORD(site));
            if (value>32767) fatal_message(path,0x3b,name);
            goto store_word;
        case 2: case 4:
            MAC_REQUIRE_DATA();
            value=symbol->Value+MAC_READ_LONG(site);
            goto store_long;
        case 3: case 5:
            MAC_REQUIRE_DATA();
            value=external?external->mac_jump_offset+2:
                find_mac_static_jump(target_resource,symbol,1);
            MAC_NONZERO_ADDEND(MAC_READ_LONG(site));
            goto store_long;
        case 7: case 21:
            MAC_REQUIRE_CODE();
            if (resource!=target_resource) {
                if (type==21) fatal_message(path,0x40,name);
                goto code_jump_word;
            }
            if (type==7) site[-1]=(site[-1]&0xfa)|0x3a;
            value=symbol->Value-pc+(long)(short)MAC_READ_WORD(site);
            signed_value=(long)value;
            magnitude=signed_value<0?(long)(0-value):signed_value;
            if (magnitude>32767) fatal_message(path,0x3b,name);
            goto store_word;
        case 8:
code_jump_word:
            MAC_REQUIRE_CODE();
            if (type==7) site[-1]=(site[-1]&0xed)|0x2d;
            MAC_NONZERO_ADDEND(MAC_READ_WORD(site));
            value=external?external->mac_jump_offset+2:
                find_mac_static_jump(target_resource,symbol,0);
            if ((long)value>32767) fatal_message(path,0x3b,name);
            goto store_word;
        case 9: case 10:
            MAC_REQUIRE_CODE();
            value=symbol->Value+MAC_READ_LONG(site);
            goto store_long;
        case 11: case 22:
            MAC_REQUIRE_CODE();
            if (section->index!=(unsigned short)symbol->SectionNumber) {
                if (type==22) fatal_message(path,0x40,name);
                goto code_jump_long;
            }
            value=symbol->Value-40+MAC_READ_LONG(site);
            goto store_long;
        case 12:
code_jump_long:
            MAC_REQUIRE_CODE();
            MAC_NONZERO_ADDEND(MAC_READ_LONG(site));
            value=external?external->mac_jump_offset+2:
                find_mac_static_jump(target_resource,symbol,1);
            goto store_long;
        case 19:
            value=symbol->Value>>1;
            if (value&0xffe00000) fatal_message(path,0x3b,name);
            if (target_resource>2047) fatal_message(path,4,"Too many sections for profiler");
            value|=(unsigned long)target_resource<<21;
            goto store_long;
        case 20:
            apply_mac_special_relocation(site,name,symbol,contribution,external);
            break;
        case 23:
            *(unsigned long *)site+=symbol->Value;
            if (*(unsigned short *)(site+4)) fatal_message(path,0x3a,name);
            *(unsigned short *)(site+4)=target_resource;
            break;
        case 30: case 31: case 32:
            if (unpaired) {
                pair=relocation;
                unpaired=0;
                pair_address=relocation->VirtualAddress;
                pair_value=symbol->Value;
                pair_resource=target_resource;
                break;
            }
            if ((char *)pair-(char *)relocation!=-10) fatal_message(path,0x41,group_name);
            if (target_resource!=pair_resource) fatal_message(path,0x42,group_name);
            if (relocation->VirtualAddress!=pair_address) fatal_message(path,0x43,group_name);
            value=pair_value-symbol->Value;
            unpaired=1;
            if (type==30) {
                value+=(long)(signed char)*site;
                if ((long)value<-256 || (long)value>255) fatal_message(path,0x45,group_name);
                *site=(unsigned char)value;
                break;
            }
            if (type==31) {
                value+=(long)(short)MAC_READ_WORD(site);
                if ((long)value<-32768 || (long)value>32767) fatal_message(path,0x45,group_name);
                goto store_word;
            }
            value+=MAC_READ_LONG(site);
            goto store_long;
        case 35: case 36: case 37: case 38: case 39: case 40:
            if (!external || (external->owner.contribution->flags&0x20))
                fatal_message(program_name,4,"CSECTABLE fixup must be to external symbol in code");
            kind=external->flags&0x0f000000;
            width=kind==0x04000000?4:(kind==0x02000000?2:1);
            value=((long)(short)mac_section_resources[resource-1].id-1)*width+symbol->Value;
            if (type==38 || type==39 || type==40) {
                value+=MAC_READ_LONG(site);
                goto store_long;
            }
            value+=(long)(short)MAC_READ_WORD(site);
            if ((long)value<-32768) fatal_message(path,0x3b,name);
            goto store_word;
        case 41: case 42:
            if (!external || !(external->owner.contribution->flags&0x20))
                fatal_message(program_name,4,"DUPCON fixup must be to external symbol in code");
            duplicate=find_module(mac_segment_library,name,0);
            copy=&duplicate->contributions[section->index-1];
            symbol->Value=copy->rva-copy->group->section->rva+external->symbol.Value;
            if (type==41) {
                value=symbol->Value-pc+(long)(short)MAC_READ_WORD(site);
                signed_value=(long)value;
                magnitude=signed_value<0?(long)(0-value):signed_value;
                if (magnitude>32767) fatal_message(path,0x3b,name);
                goto store_word;
            }
            value=symbol->Value-40+MAC_READ_LONG(site);
            goto store_long;
        case 45:
            value=mac_section_resources[resource-1].id;
            goto store_word;
        case 46: case 47: case 48:
            MAC_REQUIRE_CODE();
            value=MAC_READ_LONG(site)>>8;
            if (value&0x800000) value|=0xff000000;
            if (type==46) value+=symbol->Value;
            else if (type==47) {
                MAC_NONZERO_ADDEND(value);
                value=external?external->mac_jump_offset+2:
                    find_mac_static_jump(target_resource,symbol,1);
            } else {
                if (resource!=target_resource) fatal_message(path,0x40,name);
                value=value-pc+symbol->Value;
            }
            if ((long)value<-16777216 || (long)value>16777215)
                fatal_message(path,0x3b,name);
            site[0]=(unsigned char)(value>>16);
            site[1]=(unsigned char)(value>>8);
            site[2]=(unsigned char)value;
            break;
        default:
            warning_message(path,0x4f,type,relocation->VirtualAddress);
            break;
store_word:
            MAC_STORE_WORD(site,value);
            break;
store_long:
            MAC_STORE_LONG(site,value);
            break;
        }
        ++relocation;
    }
    if (!unpaired) fatal_message(path,0x44,group_name);
}

void resolve_mac_relocation_symbol(External **external,PIMAGE_SYMBOL *symbol,
    unsigned short type,Contribution *contribution) { __asm { int 3 } }
void validate_mac_relocation_value(PIMAGE_RELOCATION relocation,unsigned long value,
    unsigned char standalone)
{
    unsigned char a5_reference=0;
    /* Original [00038ff0,000390c7), switch data [000390c8,00039110). */
    switch (relocation->Type) {
    case 12:
        if ((long)value<0) { relocation->Type=10; a5_reference=1; }
        break;
    case 25: relocation->Type=(long)value<0?0:1; break;
    case 26: relocation->Type=(long)value<0?2:3; break;
    case 27: relocation->Type=(long)value<0?4:5; break;
    case 28:
        if ((long)value<0) { relocation->Type=6; a5_reference=1; }
        else relocation->Type=8;
        break;
    case 29:
        if ((long)value<0) { relocation->Type=10; a5_reference=1; }
        else relocation->Type=11;
        break;
    }
    if (a5_reference && standalone)
        fatal_message(program_name,4,"Illegal a5 ref in sacode.");
}
unsigned long find_mac_static_jump(unsigned short section,PIMAGE_SYMBOL symbol,
    unsigned char wide) { __asm { int 3 } }
void apply_mac_special_relocation(unsigned char *site,const char *name,PIMAGE_SYMBOL symbol,
    Contribution *contribution,External *external) { __asm { int 3 } }
