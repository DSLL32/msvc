#include <windows.h>
#include <string.h>
#include "memory.h"
#include "files.h"
#include "arguments.h"
#include "omf.h"
#include <recon.h>

OmfSegmentInfo omf_segments[256];
DWORD omf_segdef_symbols[256];
/* Original first actual element478a78 is distinct from record-length478a74.
 * SEGDEF uses (section*4+478a74), whereas the writer uses index*4+478a78. */
DWORD omf_section_auxiliary_indices[256];
const DWORD omf_segment_alignments[8]={0,1,2,16,4096,4,0,0};

#pragma intrinsic(strcmp,strlen,strcpy,strcat,memset,memcpy)
void __cdecl read_omf_segdef(void)
{
    BYTE attributes;
    DWORD length,alignment,flags,created,symbol,auxiliary;
    WORD name_index;
    const char *name,*class_name,*prefix=NULL,*replacement=NULL,*suffix;
    unsigned class_length,prefix_length=0,replacement_length=0,i;
    BOOL debug_segment=FALSE,code_segment=FALSE;
    BYTE section_auxiliary[18];
    char *renamed;
    omf_segdef_count=(WORD)(omf_segdef_count+1);
    if((SHORT)omf_segdef_count>=256)omf_error("Too many SEGDEF/COMDAT records");
    attributes=(BYTE)read_omf_byte();
    if(!(attributes&0xe0)) {
        read_omf_word();
        read_omf_byte();
    }
    length=omf_record_is32?read_omf_dword():(read_omf_word()&0xffff);
    name_index=(WORD)read_omf_index();
    name=omf_names[(SHORT)name_index];
    omf_section_symbol_count=(WORD)(omf_section_symbol_count+1);
    omf_segments[(SHORT)omf_section_symbol_count].name_index=name_index;
    class_name=omf_names[(SHORT)read_omf_index()];
    read_omf_index(); /* overlay name is physically present and ignored */
    if(attributes&2)omf_error("Bad segment definition");
    if(!strcmp(name,"$$SYMBOLS") && !strcmp(class_name,"DEBSYM")) {
        name=".debug$S";
        debug_segment=TRUE;
    } else if(!strcmp(name,"$$TYPES") && !strcmp(class_name,"DEBTYP")) {
        name=omf_compile_flags?".debug$P":".debug$T";
        debug_segment=TRUE;
    } else if(!strcmp(name,".debug$F"))debug_segment=TRUE;
    if(debug_segment)flags=0x42100040;
    else {
        class_length=strlen(class_name);
        if(class_length>=4 && !strcmp(class_name+class_length-4,"CODE")) {
            prefix="_TEXT";replacement=".text";
            prefix_length=replacement_length=5;
            flags=0x60000020;
            code_segment=TRUE;
        } else if(class_length>=4 && !strcmp(class_name+class_length-4,"DATA")) {
            prefix="_DATA";replacement=".data";
            prefix_length=replacement_length=5;
            flags=0xc0000040;
        } else if(class_length>=5 && !strcmp(class_name+class_length-5,"CONST")) {
            prefix="CONST";replacement=".rdata";
            prefix_length=5;replacement_length=6;
            flags=0x40000040;
        } else if(class_length>=3 && !strcmp(class_name+class_length-3,"BSS")) {
            prefix="_BSS";replacement=".bss";
            prefix_length=replacement_length=4;
            flags=0xc0000080;
        } else flags=0xc0000040;
        if(prefix) {
            /* Match the original short-circuit byte comparison; a name may
             * be shorter than the canonical prefix without overreading it. */
            for(i=0;i<prefix_length && name[i]==prefix[i];++i) {}
            if(i==prefix_length) {
                suffix=name+prefix_length;
                if(!*suffix)name=replacement;
                else if(*suffix=='$') {
                    renamed=allocate_memory(strlen(suffix)+replacement_length+1);
                    strcpy(renamed,replacement);
                    strcat(renamed,suffix);
                    name=renamed;
                }
            }
        }
        if(!(attributes&1))flags|=0x20000;
    }
    alignment=omf_segment_alignments[attributes>>5];
    omf_segments[(SHORT)omf_section_symbol_count].characteristics=flags;
    omf_segments[(SHORT)omf_section_symbol_count].alignment=alignment;
    created=create_omf_section(name,alignment,0,length,flags);
    omf_segments[(SHORT)omf_section_symbol_count].output_section=(WORD)created;
    if(code_segment && !omf_default_line_segment)omf_default_line_segment=(WORD)created;
    /* The real caller forwards the full carried result, while append_symbol
     * consumes only its signed low-WORD section-number view. */
    symbol=append_omf_symbol(name,0,created,IMAGE_SYM_CLASS_STATIC,1);
    omf_segdef_symbols[(SHORT)omf_segdef_count]=symbol;
    memset(section_auxiliary,0,sizeof(section_auxiliary));
    memcpy(section_auxiliary,&length,sizeof(length));
    auxiliary=append_omf_auxiliary(section_auxiliary);
    omf_section_auxiliary_indices[(SHORT)created-1]=auxiliary;
}

/* The original uses zero-based output-section arrays, independently of the
 * logical one-based SEGDEF descriptors above. New payloads are genuine zeroed
 * 128KiB allocations; unconsumed header fields retain their existing state. */
WORD omf_output_section_count; /*479a9c*/
IMAGE_SECTION_HEADER omf_output_sections[256]; /*479aa8*/
BYTE *omf_section_data[256]; /*478250*/
DWORD omf_section_capacities[256]; /*475a10*/
DWORD omf_initialized_data_size; /*478240*/

DWORD __stdcall create_omf_section(const char *name,DWORD alignment,
    DWORD address,DWORD size,DWORD characteristics)
{
    WORD index=omf_output_section_count;
    DWORD encoded_alignment,flags;
    IMAGE_SECTION_HEADER *section;
    omf_output_section_count=(WORD)(omf_output_section_count+1);
    if(omf_output_section_count>256)omf_error("Too many COFF sections");
    omf_section_data[index]=zero_allocate(0x20000);
    omf_section_capacities[index]=0x20000;
    section=&omf_output_sections[index];
    if(strlen(name)<=8)strncpy((char *)section->Name,name,8);
    else sprintf((char *)section->Name,"/%lu",intern_omf_string(name));
    section->Misc.VirtualSize=0;
    section->VirtualAddress=address;
    section->SizeOfRawData=size;
    section->NumberOfRelocations=0;
    section->NumberOfLinenumbers=0;
    switch(alignment) {
    case 1:encoded_alignment=0x100000;break;
    case 2:encoded_alignment=0x200000;break;
    case 4:encoded_alignment=0x300000;break;
    case 8:encoded_alignment=0x400000;break;
    default:encoded_alignment=0x500000;break;
    }
    flags=encoded_alignment|characteristics;
    section->Characteristics=flags;
    if(!(flags&IMAGE_SCN_CNT_UNINITIALIZED_DATA))omf_initialized_data_size+=size;
    /* MOV AX replaces only the low WORD of the just-stored characteristics.
     * Preserve the physical DWORD forwarding; current callers interpret AX. */
    return (flags&0xffff0000)|omf_output_section_count;
}

DWORD omf_group_symbols[256]; /*4715c8, logical groups1..255*/
void __cdecl read_omf_grpdef(void)
{
    const char *name=omf_names[(SHORT)read_omf_index()];
    DWORD section=(DWORD)-1;
    while(omf_record_length>1) {
        read_omf_byte(); /* the original consumes the component tag unchecked */
        /* Only SI is replaced; append_symbol consumes its signed WORD view. */
        section=(section&0xffff0000)|omf_segments[(SHORT)read_omf_index()].output_section;
    }
    omf_group_count=(WORD)(omf_group_count+1);
    if((SHORT)omf_group_count>=256)omf_error("Too many groups");
    omf_group_symbols[(SHORT)omf_group_count]=
        append_omf_symbol(name,0,section,IMAGE_SYM_CLASS_STATIC,0);
}

void __cdecl read_omf_ledata(void)
{
    BYTE bytes[4096];
    LONG count;
    omf_data_record=0xa0;
    omf_data_segment=read_omf_index();
    omf_data_offset=omf_record_is32?read_omf_dword():(read_omf_word()&0xffff);
    count=(LONG)omf_record_length-1;
    if(count>4096)omf_error("Bad data record; too large");
    memset(bytes,0,count);
    read_omf_bytes(bytes,count);
    /* The physical first DWORD is consumed as signed AX by the writer. */
    write_omf_section_data(omf_segments[(SHORT)omf_data_segment].output_section,
                           omf_data_offset,bytes,count);
}

void __stdcall write_omf_section_data(DWORD section,DWORD offset,
    const void *bytes,DWORD size)
{
    SHORT index=(SHORT)(section-1);
    BYTE *larger;
    DWORD capacity=omf_section_capacities[index];
    if(offset+size>capacity) {
        larger=zero_allocate(capacity+size+0x20000);
        memcpy(larger,omf_section_data[index],capacity);
        release_memory(omf_section_data[index]);
        omf_section_data[index]=larger;
        omf_section_capacities[index]=capacity+size+0x20000;
    }
    memcpy(omf_section_data[index]+offset,bytes,size);
}

DWORD __stdcall read_omf_iterated_block(DWORD offset)
{
    BYTE data[2048];
    DWORD repeat,blocks,i,length;
    LONG position;
    SHORT saved_length;
    repeat=omf_record_is32?read_omf_dword():(read_omf_word()&0xffff);
    blocks=read_omf_word()&0xffff;
    if(blocks) {
        position=ftell(omf_stream);
        saved_length=omf_record_length;
        while((LONG)repeat>0) {
            omf_record_length=saved_length;
            for(i=0;i<blocks;++i)offset=read_omf_iterated_block(offset);
            --repeat;
            if(repeat && fseek(omf_stream,position,SEEK_SET))
                omf_error("Cannot expand iterated data");
        }
    } else {
        length=(BYTE)read_omf_byte();
        read_omf_bytes(data,length);
        while((LONG)repeat>0) {
            /* The physical DWORD section slot is consumed only as a signed
             * low WORD by the existing writer; upper register carry is unused. */
            write_omf_section_data(omf_segments[(SHORT)omf_data_segment].output_section,
                offset,data,length);
            offset+=length;
            --repeat;
        }
    }
    return offset;
}

void __cdecl read_omf_lidata(void)
{
    omf_data_record=0xa2;
    omf_data_segment=read_omf_index();
    omf_data_offset=omf_record_is32?read_omf_dword():(read_omf_word()&0xffff);
    while(omf_record_length>1)
        omf_data_offset=read_omf_iterated_block(omf_data_offset);
}

void __cdecl read_omf_bakpat(void)
{
    SHORT segment=(SHORT)read_omf_index();
    BYTE location=(BYTE)read_omf_byte();
    DWORD offset,value,wide;
    WORD word_value;
    SHORT section;
    BYTE *destination;
#ifdef __linux__
    if(segment<0 || segment>=256) { __asm { int 3 } }
#endif
    while(omf_record_length>1) {
        offset=omf_record_is32?read_omf_dword():(read_omf_word()&0xffff);
        value=omf_record_is32?read_omf_dword():(read_omf_word()&0xffff);
        section=(SHORT)(omf_segments[segment].output_section-1);
#ifdef __linux__
        /* Only a live allocated output-section span is supported natively. */
        if(section<0 || section>=omf_output_section_count ||
            offset>omf_section_capacities[section] ||
            (location<=2 && (1U<<location)>omf_section_capacities[section]-offset)) {
            __asm { int 3 }
        }
#endif
        destination=omf_section_data[section]+offset;
        switch(location) {
        case 0:*destination=(BYTE)(*destination+(BYTE)value);break;
        case 1:
            memcpy(&word_value,destination,sizeof(word_value));
            word_value=(WORD)(word_value+(WORD)value);
            memcpy(destination,&word_value,sizeof(word_value));break;
        case 2:
            memcpy(&wide,destination,sizeof(wide));
            wide+=value;
            memcpy(destination,&wide,sizeof(wide));break;
        default:omf_error("Ilegal LocTyp in BAKPAT record");break;
        }
    }
}

/* Original479a88 remembers the last logical COMDAT segment for continuation
 * records, separate from current data segment478a70 and data offset479a90. */
WORD omf_last_comdat_segment;
void __cdecl read_omf_comdat(void)
{
    BYTE flags,attributes,alignment,checksum=0;
    BYTE bytes[4096];
    WORD base_segment,name_index,output_section;
    DWORD size,created,symbol;
    LONG count;
    const char *name,*section_name;
    IMAGE_AUX_SYMBOL auxiliary;
    flags=(BYTE)read_omf_byte();
    attributes=(BYTE)read_omf_byte();
    alignment=(BYTE)read_omf_byte();
    omf_data_record=0xa0;
    omf_data_offset=omf_record_is32?read_omf_dword():(read_omf_word()&0xffff);
    read_omf_index(); /* type index */
    read_omf_index(); /* group index */
    base_segment=read_omf_index();
    if(!base_segment)omf_error("Section not defined in COMDAT");
    name_index=read_omf_index();
    name=omf_names[(SHORT)name_index];
    count=(LONG)omf_record_length-1;
    if(count>4096)omf_error("Bad data record; too large");
    memset(bytes,0,count);
    read_omf_bytes(bytes,count);
    if(flags&1) {
        omf_data_segment=omf_last_comdat_segment;
        append_omf_section_data(omf_segments[(SHORT)omf_data_segment].output_section,
            omf_data_offset,bytes,count);
        return;
    }
    omf_segdef_count=(WORD)(omf_segdef_count+1);
    if((SHORT)omf_segdef_count>=256)omf_error("Too many SEGDEF/COMDAT records");
    omf_section_symbol_count=(WORD)(omf_section_symbol_count+1);
    omf_data_segment=omf_section_symbol_count;
    omf_segments[(SHORT)omf_data_segment]=omf_segments[(SHORT)base_segment];
    size=alignment?omf_segment_alignments[alignment]:omf_segments[(SHORT)omf_data_segment].alignment;
    section_name=omf_names[(SHORT)omf_segments[(SHORT)base_segment].name_index];
    created=create_omf_section(section_name,size,0,count,
        omf_segments[(SHORT)base_segment].characteristics|IMAGE_SCN_LNK_COMDAT);
    omf_last_comdat_segment=omf_data_segment;
    omf_segments[(SHORT)omf_data_segment].output_section=(WORD)created;
    write_omf_section_data(created,omf_data_offset,bytes,count);
    if(flags&2)omf_error("COMDAT uses iterated data");
    symbol=append_omf_symbol(section_name,0,
        omf_segments[(SHORT)omf_data_segment].output_section,IMAGE_SYM_CLASS_STATIC,1);
    omf_segdef_symbols[(SHORT)omf_segdef_count]=symbol;
    memset(&auxiliary,0,sizeof(auxiliary));
    auxiliary.Section.Length=count;
    /* Read and put back the actual following checksum byte, without changing
     * the record-length owner as read_omf_bytes would do. */
    if(fread(&checksum,1,1,omf_stream)!=1)omf_error("Bad read of object file");
    if(fseek(omf_stream,-1,SEEK_CUR))omf_error("Bad seek on object file");
    auxiliary.Section.CheckSum=checksum;
    if(attributes==0)auxiliary.Section.Selection=1;
    else if(attributes==0x10)auxiliary.Section.Selection=2;
    else if(attributes==0x20)auxiliary.Section.Selection=3;
    else if(attributes==0x30)auxiliary.Section.Selection=4;
    else auxiliary.Section.Selection=0;
    output_section=omf_segments[(SHORT)omf_data_segment].output_section;
    omf_section_auxiliary_indices[(SHORT)output_section-1]=append_omf_auxiliary(&auxiliary);
    symbol=append_omf_symbol(name,0,output_section,2|((flags&4)>>2),0);
    if(omf_name_kinds[(SHORT)name_index]==(DWORD)-1)
        omf_name_kinds[(SHORT)name_index]=symbol;
}

void __stdcall append_omf_section_data(DWORD section,DWORD offset,
    const void *bytes,DWORD count)
{
    SHORT index=(SHORT)section;
    omf_output_sections[index-1].SizeOfRawData+=count;
    omf_initialized_data_size+=count;
    write_omf_section_data(section,offset,bytes,count);
}
