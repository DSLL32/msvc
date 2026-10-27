#include <windows.h>
#include <string.h>
#include "memory.h"
#include "arguments.h"
#include "omf.h"
#include <recon.h>

OmfFixupPosition *omf_fixup_head,*omf_fixup_tail; /*47cea8/479a84*/
void __stdcall remember_omf_fixup_position(LONG file_offset)
{
    OmfFixupPosition *position=allocate_memory(sizeof(*position));
    position->file_offset=file_offset;
    position->data_offset=omf_data_offset;
    position->segment=omf_data_segment;
    position->record_type=omf_data_record;
    position->next=NULL;
    if(!omf_fixup_tail)omf_fixup_head=position;
    else omf_fixup_tail->next=position;
    omf_fixup_tail=position;
}

DWORD omf_target_threads[4]; /*478a60..478a70*/
BYTE omf_target_thread_is_segment[4]; /*475e10..475e14*/
void __cdecl read_omf_fixupp(void)
{
    IMAGE_RELOCATION relocations[1024];
    DWORD addends[1024];
    DWORD count=0,i,target,displacement;
    unsigned method,slot,location;
    BYTE first,bytes[2],spec,target_is_segment;
    while(omf_record_length>1) {
        read_omf_bytes(&first,1);
        if(first&0x80) {
            read_omf_bytes(bytes,2);
            if((LONG)count>=1024)omf_error("Too many relocation entries");
            if(omf_data_record!=0xa0 && omf_data_record!=0xa2)
                omf_error("Bad relocatable reference");
            relocations[count].VirtualAddress=
                (((DWORD)first&3)<<8)+bytes[0]+omf_data_offset;
            spec=bytes[1];
            if(!(spec&0x80))read_omf_fixup_target((spec>>4)&7);
            if(!(spec&8)) {
                target=read_omf_fixup_target(spec&3);
                target_is_segment=(BYTE)((spec&3)==0);
            } else {
                target=omf_target_threads[spec&3];
                target_is_segment=omf_target_thread_is_segment[spec&3];
            }
            location=(first>>2)&15;
            switch(location) {
            /* These byte-relocation values8/16h are actual converter codes,
             * not a substituted public SDK relocation spelling. */
            case 0:relocations[count].Type=(WORD)((first&0x40)?8:0x16);break;
            case 1: case 5:
                relocations[count].Type=(WORD)((first&0x40)?IMAGE_REL_I386_DIR16:IMAGE_REL_I386_REL16);break;
            case 4:
                if(!(first&0x40))omf_error("Bad relocation type");
                relocations[count].Type=IMAGE_REL_I386_SECTION;break;
            case 9: case 13:
                relocations[count].Type=(WORD)((first&0x40)?IMAGE_REL_I386_DIR32:IMAGE_REL_I386_REL32);break;
            case 14:relocations[count].Type=IMAGE_REL_I386_DIR32NB;break;
            case 11:relocations[count].Type=IMAGE_REL_I386_DIR32;break;
            case 2: case 3:omf_error("Segment reference in fixup record");
            default:omf_error("Bad relocation type");break;
            }
            relocations[count].SymbolTableIndex=target;
            if(spec&4)displacement=0;
            else {
                displacement=omf_record_is32?read_omf_dword():(read_omf_word()&0xffff);
                if(target_is_segment && displacement) {
                    if(omf_pass==1)remember_omf_target_offset(target,displacement);
                    else {
                        target=lookup_omf_target_offset(target,displacement);
                        if(target!=relocations[count].SymbolTableIndex) {
                            relocations[count].SymbolTableIndex=target;
                            displacement=0;
                        }
                    }
                }
            }
            addends[count]=displacement;
            ++count;
        } else {
            method=(first>>2)&7;
            if(first&0x40)read_omf_fixup_target(method); /* frame thread discarded */
            else {
                slot=first&3;
                omf_target_threads[slot]=read_omf_fixup_target(method);
                omf_target_thread_is_segment[slot]=(BYTE)((first&0x1c)==0);
            }
        }
    }
    if(omf_pass!=1) {
        /* The original emits the collected relocations in reverse order. */
        i=count;
        while(i) {
            --i;
            append_omf_relocation(omf_segments[(SHORT)omf_data_segment].output_section,
                relocations[i].VirtualAddress,relocations[i].SymbolTableIndex,
                relocations[i].Type,addends[i]);
        }
    }
}

DWORD __stdcall read_omf_fixup_target(DWORD method)
{
    SHORT index;
    DWORD symbol;
    switch(method) {
    case 0:return omf_segdef_symbols[(SHORT)read_omf_index()];
    case 1:return omf_group_symbols[(SHORT)read_omf_index()];
    case 2:
        index=(SHORT)read_omf_index();
        symbol=omf_name_kinds[index];
        return symbol?symbol:omf_public_map[omf_external_map[index]];
    case 4:case 5:return (DWORD)-1;
    default:omf_error("Bad method in FIXUP record");return method;
    }
}

void __cdecl replay_omf_fixups(void)
{
    OmfFixupPosition *position=omf_fixup_head,*next;
    WORD kind;
    while(position) {
        fseek(omf_stream,position->file_offset,SEEK_SET);
        omf_record_type=(SHORT)getc(omf_stream);
        kind=(WORD)omf_record_type&0xfe;
        if(kind!=0x9c && kind!=0x9d)
            omf_error("proc_fixups: not a fixup record");
        omf_record_is32=(BYTE)omf_record_type&1;
        omf_record_length=(SHORT)read_omf_word();
        omf_data_segment=position->segment;
        omf_data_record=position->record_type;
        omf_data_offset=position->data_offset;
        read_omf_fixupp();
        next=position->next;
        release_memory(position);
        position=next;
    }
    omf_fixup_tail=NULL;
    omf_fixup_head=NULL;
}

OmfRelocationList omf_relocations[256]; /*478e78..479a78*/
DWORD omf_relocation_count; /*479a8c*/
WORD omf_file_symbol_reference; /*479aa0*/
#pragma intrinsic(strcmp,memcpy)
void __stdcall append_omf_relocation(DWORD section,DWORD offset,
    DWORD symbol,DWORD type,DWORD displacement)
{
    SHORT index=(SHORT)(section-1);
    BYTE *destination;
    WORD word_value;
    DWORD large;
    if(!strcmp((const char *)omf_symbols[symbol].N.ShortName,".file")) {
        omf_file_symbol_reference=1;
        return;
    }
    ++omf_relocation_count;
    ++omf_output_sections[index].NumberOfRelocations;
    append_omf_relocation_node(&omf_relocations[index],offset,symbol,type);
    destination=omf_section_data[index]+offset;
    switch((WORD)type) {
    case IMAGE_REL_I386_DIR16:case IMAGE_REL_I386_REL16:
        memcpy(&word_value,destination,sizeof(word_value));
        word_value=(WORD)(word_value+(WORD)displacement);
        memcpy(destination,&word_value,sizeof(word_value));break;
    case IMAGE_REL_I386_DIR32:case IMAGE_REL_I386_REL32:
        memcpy(&large,destination,sizeof(large));
        large+=displacement;
        memcpy(destination,&large,sizeof(large));break;
    case IMAGE_REL_I386_DIR32NB:break;
    /* Actual converter byte-update codes, including SECTION's low byte. */
    case 8:case IMAGE_REL_I386_SECTION:case 0x16:
        *destination=(BYTE)(*destination+(BYTE)displacement);break;
    default:omf_error("Bad COFF relocation type");break;
    }
}

void __stdcall append_omf_relocation_node(OmfRelocationList *list,
    DWORD offset,DWORD symbol,DWORD type)
{
    OmfRelocationNode *entry=allocate_memory(sizeof(*entry));
    entry->next=NULL;
    entry->offset=offset;
    entry->type=(WORD)type;
    entry->symbol=symbol;
    if(!list->head) {
        ++list->count;
        list->head=entry;
        list->tail=entry;
    } else {
        list->tail->next=entry;
        ++list->count;
        list->tail=entry;
    }
}
