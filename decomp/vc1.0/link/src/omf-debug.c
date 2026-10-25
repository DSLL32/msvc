/* Older CodeView-to-COFF debug records used by the converter's -g mode. */
#include "omf.h"
#include <recon.h>
unsigned char *omf_debug_symbol_data;
unsigned long omf_debug_symbol_capacity;
unsigned char *omf_debug_type_data;
unsigned long omf_debug_type_bytes;
unsigned char **omf_debug_type_records;
OmfDebugType *omf_debug_type_map;
unsigned short omf_debug_type_count;
OmfDeferredDebugType *omf_deferred_debug_types;

#pragma intrinsic(memcpy)
/* Original [00049c20,00049cfa), including two bytes of internal alignment. */
void omf_write_debug_symbol_data(unsigned long offset,const void *data,
                                 unsigned long length)
{
    unsigned char *buffer=omf_debug_symbol_data,*replacement;
    unsigned long capacity;
    if (!buffer) {
        buffer=calloc(0x20000,1);
        omf_debug_symbol_data=buffer;
        if (!buffer) {
            omf_error("Cannot malloc memory for $$Symbols");
            buffer=omf_debug_symbol_data;
        }
        capacity=0x20000;
    } else capacity=omf_debug_symbol_capacity;
    omf_debug_symbol_data=buffer;
    omf_debug_symbol_capacity=capacity;
    if (offset+length>capacity) {
        replacement=calloc(capacity+length+0x20000,1);
        if (!replacement) out_of_memory();
        memcpy(replacement,buffer,omf_debug_symbol_capacity);
        free(buffer);
        buffer=replacement;
        capacity=omf_debug_symbol_capacity+length+0x20000;
        omf_debug_symbol_capacity=capacity;
    }
    omf_debug_symbol_data=buffer;
    memcpy(buffer+offset,data,length);
}
#pragma function(memcpy)

/* Original [000479f0,00047b8b). Its input is the converter's live FIXUPP
 * record; the saved eight-byte rows describe references in $$SYMBOLS. */
void omf_save_debug_fixups(void)
{
    unsigned char header,fields[2];
    unsigned long offset,symbol;
    unsigned short external;
    short section,index;
    while (omf_record_remaining>1) {
        external=0;
        section=0;
        omf_read_block(&header,1);
        omf_read_block(fields,2);
        offset=((unsigned long)(header&3)<<8|fields[0])+omf_data_offset;
        if (!(fields[1]&8)) {
            switch (fields[1]&3) {
            case 0:
                index=(short)omf_read_index();
                section=(short)omf_segments[index].coff_section;
                break;
            case 1:
                omf_read_index();
                section=(short)omf_data_section;
                break;
            case 2:
                omf_read_index();
                external=1;
                break;
            }
        } else {
            section=-1;
            symbol=omf_thread_symbols[fields[1]&3];
            for (index=0;index<(short)omf_defined_segment_count;++index) {
                if (omf_segment_symbol_indices[index]!=symbol) continue;
                section=(short)omf_segments[index].coff_section;
                break;
            }
            if (section<0) {
                for (index=0;index<(short)omf_group_count;++index) {
                    if (omf_group_symbol_indices[index]!=symbol) continue;
                    section=(short)omf_data_section;
                    break;
                }
            }
        }
        omf_debug_fixups[omf_debug_fixup_count].offset=offset;
        omf_debug_fixups[omf_debug_fixup_count].external=external;
        omf_debug_fixups[omf_debug_fixup_count].section=section;
        ++omf_debug_fixup_count;
        if (!(omf_debug_fixup_count&63)) {
            omf_debug_fixups=realloc(omf_debug_fixups,
                                    omf_debug_fixup_count*8+512);
            if (!omf_debug_fixups)
                omf_error("debugfixup: Out of memory");
        }
    }
}

#pragma intrinsic(memcpy)
/* Original [0004acb0,0004ad3e). The record's end is the new logical size;
 * gaps are not zero-filled by this original malloc/realloc path. */
void omf_write_debug_type_data(unsigned long offset,const void *data,
                               unsigned long length)
{
    unsigned long growth=offset-omf_debug_type_bytes+length;
    unsigned char *buffer;
    if (!omf_debug_type_bytes) {
        buffer=malloc(growth);
        omf_debug_type_data=buffer;
        if (!buffer) {
            omf_error("Not enough memory to process types data");
            buffer=omf_debug_type_data;
        }
    } else {
        buffer=realloc(omf_debug_type_data,omf_debug_type_bytes+growth);
        omf_debug_type_data=buffer;
        if (!buffer) {
            omf_error("Not enough memory to process types data");
            buffer=omf_debug_type_data;
        }
    }
    omf_debug_type_data=buffer;
    omf_debug_type_bytes+=growth;
    memcpy(buffer+offset,data,length);
}
#pragma function(memcpy)

/* Original [0004a960,0004aaad). The pointer directory owns exactly the
 * parsed record entries. Type-map slots are defined by their converters. */
void omf_convert_debug_types(void)
{
    unsigned short index;
    unsigned char *record;
    if (!omf_debug_type_bytes) return;
    omf_debug_type_count=0;
    omf_debug_type_records=malloc(64);
    if (!omf_debug_type_records)
        omf_error("Not enough memory to processes OMF type data");
    index=0;
    record=omf_debug_type_data;
    while (record<omf_debug_type_data+omf_debug_type_bytes) {
        omf_debug_type_records[index]=record;
        ++index;
        record+=(unsigned long)*(unsigned short *)(record+1)+3;
        if (!(index&15)) {
            omf_debug_type_records=realloc(omf_debug_type_records,
                                           (unsigned long)index*4+64);
            if (!omf_debug_type_records)
                omf_error("Not enough memory to processes OMF type data");
        }
    }
    omf_debug_type_count=index;
    omf_debug_type_map=malloc((unsigned long)index*12);
    if (!omf_debug_type_map)
        omf_error("Not enough memory to process type data");
    for (index=0;index<omf_debug_type_count;++index) {
        if (omf_debug_type_records[index]) omf_convert_debug_type(index,0);
    }
    omf_resolve_debug_types();
    free(omf_debug_type_data);
    omf_debug_type_bytes=0;
    free(omf_debug_type_records);
}

/* Original [0004a4c0,0004a8c1), with the native switch table separately
 * owned at [0004a8c4,0004a958). Indexes here are zero-based directory slots. */
void omf_convert_debug_type(unsigned index,unsigned enclosing)
{
    unsigned char *record=omf_debug_type_records[index];
    OmfDebugType *type,*element;
    IMAGE_AUX_SYMBOL *array,*inner;
    unsigned short source,coff_type,derived;
    unsigned leaf;
    unsigned long bytes;
    int dimension;
    if (!record) return;
    record+=3;
    type=&omf_debug_type_map[index];
    if (*record==0x79 && enclosing==index) return;
    type->type=type->reserved=0;
    type->size=0;
    type->value.tag=0;
    leaf=*record++;
    switch (leaf) {
    default:
        omf_warning("OMF type index %d: unknown type\n",index+0x200);
        break;
    case 0x5c:
        type->type=14;
        type->size=omf_debug_number(&record);
        ++record;
        type->value.bit_offset=*record;
        break;
    case 0x5d:
    case 0x72:
        break;
    case 0x75:
        ++record;
        source=omf_debug_index(&record);
        if (source<=0x1ff) type->type=omf_debug_primitive_type(source);
        else {
            source=(unsigned short)(source-0x200);
            omf_convert_debug_type(source,enclosing);
            *type=omf_debug_type_map[source];
        }
        coff_type=type->type;
        type->type=(unsigned short)((((coff_type&0xfff8)|8)<<2)|
                                   (coff_type&15));
        break;
    case 0x78:
        array=malloc(18);
        if (!array)
            omf_error("Bad return from malloc: out of memory during type translation");
        array->Sym.TagIndex=0;
        array->Sym.Misc.TotalSize=0;
        for (dimension=0;dimension<4;++dimension)
            array->Sym.FcnAry.Array.Dimension[dimension]=0;
        array->Sym.TvIndex=0;
        bytes=omf_debug_number(&record)/8;
        source=omf_debug_index(&record);
        if (source>0x1ff) {
            source=(unsigned short)(source-0x200);
            omf_convert_debug_type(source,enclosing);
            element=&omf_debug_type_map[source];
            type->type=element->type;
            derived=type->type&0x30;
            if (derived==0x30) {
                inner=element->value.array;
                for (dimension=0;dimension<3;++dimension)
                    array->Sym.FcnAry.Array.Dimension[dimension+1]=
                        inner->Sym.FcnAry.Array.Dimension[dimension];
                array->Sym.Misc.TotalSize=bytes;
                if (inner->Sym.Misc.TotalSize)
                    array->Sym.FcnAry.Array.Dimension[0]=
                        (unsigned short)(bytes/inner->Sym.Misc.TotalSize);
                array->Sym.TagIndex=inner->Sym.TagIndex;
            } else if (derived==0x10) {
                array->Sym.FcnAry.Array.Dimension[0]=(unsigned short)(bytes>>2);
                array->Sym.TagIndex=element->value.tag;
            } else {
                array->Sym.FcnAry.Array.Dimension[0]=element->size?
                    (unsigned short)(bytes/element->size):0;
                array->Sym.TagIndex=element->value.tag;
            }
        } else {
            type->type=omf_debug_primitive_type(source);
            array->Sym.FcnAry.Array.Dimension[0]=
                (unsigned short)(bytes/omf_debug_primitive_size(source));
        }
        coff_type=type->type;
        type->type=(unsigned short)(((coff_type<<2)&0xfff0)|
                                   (coff_type&15)|0x30);
        array->Sym.Misc.TotalSize=bytes;
        type->value.array=array;
        break;
    case 0x79:
        omf_convert_debug_structure(8,record,type,index);
        break;
    case 0x7a:
        ++record;
        source=omf_debug_index(&record);
        if (source<=0x1ff) {
            type->type=omf_debug_primitive_type(source);
            type->size=omf_debug_primitive_size(source);
        } else {
            source=(unsigned short)(source-0x200);
            omf_convert_debug_type(source,enclosing);
            *type=omf_debug_type_map[source];
            if ((type->type&0x30)==0x30) {
                coff_type=type->type&15;
                if (coff_type>=8 && coff_type<=10)
                    type->value.tag=type->value.array->Sym.TagIndex;
                else type->value.tag=0;
                coff_type=type->type;
                type->type=(unsigned short)(((coff_type>>2)&0xfff0)|
                                           (coff_type&15));
            }
            coff_type=type->type&15;
            if (coff_type>=8 && coff_type<=9 && !type->value.tag)
                type->value.reference=-(long)source;
        }
        coff_type=type->type;
        type->type=(unsigned short)((((coff_type&0xfff4)|4)<<2)|
                                   (coff_type&15));
        break;
    case 0x7b:
        omf_convert_debug_structure(10,record,type,0);
        break;
    case 0x7f:
        return;
    case 0x80:
        type->type=0;
        break;
    }
    omf_debug_type_records[index]=NULL;
}

int omf_anonymous_type_counter;
/* '.%1dfake' needs at most seventeen bytes for a signed 32-bit counter. */
char omf_anonymous_type_name[12];

#pragma intrinsic(memset,memcpy)
/* Original [00049f00,0004a4bf). Structure/union fields use byte offsets;
 * bitfield positions are converted to bits when their COFF member is emitted. */
void omf_convert_debug_structure(int kind,unsigned char *record,
                                 OmfDebugType *type,unsigned index)
{
    unsigned char *types,*names,*scan,*name,*name_end,*end;
    unsigned short source,storage,member_storage,coff_type,derived,auxiliary_count;
    unsigned long size,tag_auxiliary,auxiliary_index,forward_index;
    long offset,tag;
    int count,remaining,pending,number;
    unsigned length;
    IMAGE_AUX_SYMBOL auxiliary;
    OmfDebugType *field;
    omf_debug_type_records[index]=NULL;
    if (kind==10) {
        omf_debug_number(&record);
        ++record;
        storage=15;
        size=type->size=4;
        coff_type=type->type=10;
    } else {
        size=omf_debug_number(&record)/8;
        type->size=size;
        count=omf_debug_number(&record);
        source=(unsigned short)(omf_debug_index(&record)-0x200);
        types=omf_debug_type_records[source]+4;
        source=(unsigned short)(omf_debug_index(&record)-0x200);
        names=omf_debug_type_records[source]+4;
        if (count>1 && omf_debug_is_union(types,names)) {
            coff_type=type->type=9;
            storage=12;
            member_storage=11;
        } else {
            coff_type=type->type=8;
            storage=10;
            member_storage=8;
        }
        scan=types;
        remaining=count;
        while (remaining-->0) {
            source=omf_debug_index(&scan);
            if (source>0x1ff)
                omf_convert_debug_type((unsigned short)(source-0x200),index);
        }
    }
    ++record;
    length=*record++;
    name=record;
    name_end=record+length;
    record=name_end;
    if (kind==10) source=omf_debug_index(&record);
    if (length) *name_end=0;
    else {
        number=omf_anonymous_type_counter;
        omf_anonymous_type_counter=(int)((unsigned)number+1);
        sprintf(omf_anonymous_type_name,".%1dfake",number);
        name=(unsigned char *)omf_anonymous_type_name;
    }
    type->value.tag=omf_append_symbol((char *)name,0,-2,(unsigned char)storage,
                                     coff_type,1);
    memset(&auxiliary,0,18);
    auxiliary.Sym.Misc.TotalSize=size;
    tag_auxiliary=omf_append_auxiliary(&auxiliary);
    if (kind==10) {
        source=(unsigned short)(source-0x200);
        names=omf_debug_type_records[source]+1;
        omf_debug_type_records[source]=NULL;
        omf_debug_number(&record);
        omf_debug_number(&record);
        end=names+*(unsigned short *)names;
        names+=3;
        while (names<end) {
            ++names;
            length=*names++;
            name=names;
            name_end=names+length;
            names=name_end;
            offset=omf_debug_number(&names);
            *name_end=0;
            omf_append_symbol((char *)name,offset,-1,16,11,0);
        }
    } else {
        while (count--) {
            pending=0;
            auxiliary_count=0;
            memset(&auxiliary,0,18);
            storage=member_storage;
            source=omf_debug_index(&types);
            ++names;
            length=*names++;
            name=names;
            name_end=names+length;
            names=name_end;
            offset=omf_debug_number(&names);
            *name_end=0;
            if (source<=0x1ff) coff_type=omf_debug_primitive_type(source);
            else {
                source=(unsigned short)(source-0x200);
                field=&omf_debug_type_map[source];
                coff_type=field->type;
                if (coff_type==14) {
                    storage=18;
                    offset=(unsigned long)offset*8+field->value.bit_offset;
                    auxiliary_count=1;
                    auxiliary.Sym.Misc.TotalSize=field->size;
                } else {
                    derived=coff_type&0x30;
                    if (derived==0x30) {
                        auxiliary_count=1;
                        auxiliary=*field->value.array;
                    }
                    if ((coff_type&15)>=8 && (coff_type&15)<=10) {
                        auxiliary_count=1;
                        if (derived==0x30) {
                            tag=(long)auxiliary.Sym.TagIndex;
                            if (tag<0) {
                                if ((unsigned long)tag+index==0) {
                                    field->value.array->Sym.TagIndex=type->value.tag;
                                    auxiliary.Sym.TagIndex=field->value.array->Sym.TagIndex;
                                } else {
                                    forward_index=-tag;
                                    pending=1;
                                }
                            }
                        } else {
                            tag=field->value.reference;
                            if (tag<0) {
                                if ((unsigned long)tag+index==0)
                                    field->value.tag=type->value.tag;
                                else {
                                    forward_index=-tag;
                                    pending=1;
                                }
                            }
                            if (!pending) auxiliary.Sym.TagIndex=field->value.tag;
                            auxiliary.Sym.Misc.TotalSize=field->size;
                        }
                    }
                }
            }
            omf_append_symbol((char *)name,offset,-1,(unsigned char)storage,
                              coff_type,(unsigned char)auxiliary_count);
            if (auxiliary_count) auxiliary_index=omf_append_auxiliary(&auxiliary);
            if (pending) omf_register_debug_fixup(forward_index,auxiliary_index*18);
        }
    }
    omf_append_symbol(".eos",size,-1,102,0,1);
    memset(&auxiliary,0,18);
    auxiliary.Sym.TagIndex=type->value.tag;
    auxiliary.Sym.Misc.TotalSize=size;
    omf_append_auxiliary(&auxiliary);
    ((PIMAGE_AUX_SYMBOL)&omf_coff_symbols[tag_auxiliary])->
        Sym.FcnAry.Function.PointerToNextFunction=omf_coff_symbol_count;
}
#pragma function(memset,memcpy)

/* Original [00049db0,00049e34). Advance the caller-owned record cursor;
 * signed numeric leaves retain their original 32-bit return bit pattern. */
long omf_debug_number(unsigned char **record)
{
    unsigned char *cursor=*record;
    unsigned leaf;
    long value;
    if (!(*cursor&0x80)) {
        *record=cursor+1;
        return *cursor;
    }
    leaf=*cursor++;
    *record=cursor;
    switch (leaf) {
    case 0x85:
        value=*(unsigned short *)cursor;
        *record=cursor+2;
        return value;
    case 0x86:
    case 0x8a:
        value=*(long *)cursor;
        *record=cursor+4;
        return value;
    case 0x88:
        value=*(signed char *)cursor;
        *record=cursor+1;
        return value;
    case 0x89:
        value=*(short *)cursor;
        *record=cursor+2;
        return value;
    }
    return 0;
}

/* Original [00049e40,00049e5c). Only AX is a defined return value. */
unsigned short omf_debug_index(unsigned char **record)
{
    unsigned short index;
    if (**record!=0x83) return 0;
    ++*record;
    index=*(unsigned short *)*record;
    *record+=2;
    return index;
}

/* Original [0004ac00,0004aca5). Bitfields cannot be union members here;
 * otherwise the first two name-list offsets distinguish union from struct. */
int omf_debug_is_union(unsigned char *types,unsigned char *names)
{
    unsigned short index=omf_debug_index(&types);
    unsigned char *record;
    long first;
    if (index>0x1ff) {
        index-=0x200;
        record=omf_debug_type_records[index];
        if (record) {
            if (record[3]==0x5c) return 0;
        } else if (omf_debug_type_map[index].type==14) return 0;
    }
    names+=names[1]+2;
    first=omf_debug_number(&names);
    names+=names[1]+2;
    return omf_debug_number(&names)==first;
}

/* Original [00049e60,00049ef7). Map old C6 primitive encodings to COFF;
 * AX is the result, with pointer derivation carried in bit 4. */
unsigned short omf_debug_primitive_type(unsigned type)
{
    unsigned kind,size;
    unsigned short result;
    if (!(type&0x80)) return 0;
    kind=(type&0x1c)>>2;
    size=type&3;
    switch (kind) {
    case 0:
    case 1:
        switch (size) {
        case 0:result=kind?12:2;break;
        case 1:result=kind?13:3;break;
        case 2:result=kind?15:5;break;
        default:return 0;
        }
        break;
    case 2:
        if (!size) result=6;
        else if (size==1) result=7;
        else return 0;
        break;
    default:return 0;
    }
    if (type&0x60) result|=0x10;
    return result;
}

/* Original [0004aba0,0004abfc). Old pointer encodings always occupy a
 * DWORD; real sizes include the ten-byte extended representation. */
unsigned long omf_debug_primitive_size(unsigned type)
{
    unsigned size;
    if (type&0x60) return 4;
    size=type&3;
    if ((type&0x1c)==8) {
        if (size==1) return 8;
        if (size==2) return 10;
        return 4;
    }
    if (!size) return 1;
    if (size==1) return 2;
    return 4;
}

/* Original [0004ad60,0004ad9b). The resolver interprets offset as a row;
 * the separate structure converter's extra scaling remains unchanged. */
void omf_register_debug_fixup(unsigned index,unsigned long offset)
{
    OmfDeferredDebugType *pending=malloc(sizeof(*pending));
    if (!pending) omf_error("Not enough memory to process type data");
    pending->type=index;
    pending->offset=offset;
    pending->next=omf_deferred_debug_types;
    omf_deferred_debug_types=pending;
}

/* Original [0004ada0,0004adf3). The stored offset is scaled by a COFF
 * symbol stride here, including the original constructor's prior scaling. */
void omf_resolve_debug_types(void)
{
    OmfDeferredDebugType *pending=omf_deferred_debug_types;
    while (pending) {
        omf_deferred_debug_types=omf_deferred_debug_types->next;
        *(unsigned long *)((unsigned char *)omf_coff_symbols+
            pending->offset*18)=omf_debug_type_map[pending->type].value.tag;
        free(pending);
        pending=omf_deferred_debug_types;
    }
}

/* C6 symbol bodies after their length/type bytes, sharing the converter's
 * fixed 128-byte record workspace. Name lengths remain signed char reads. */
typedef union OmfC6SymbolRecord {
    unsigned char bytes[128];
    struct {unsigned long offset;short length;} block;
    struct {unsigned long offset;unsigned short type;short length,start,end;
            short reserved;char return_type,name_length,name[1];} procedure;
    struct {unsigned long offset;unsigned short type;
            char name_length,name[1];} stack;
    struct {unsigned long offset;char return_type,name_length,name[1];} label;
    struct {unsigned long offset;unsigned short segment,type;
            char name_length,name[1];} data;
    struct {unsigned short type;char number,name_length,name[1];} reg;
} OmfC6SymbolRecord;
typedef char OmfC6SymbolRecordSize[(sizeof(OmfC6SymbolRecord)==128)?1:-1];
OmfC6SymbolRecord omf_debug_record;

#pragma intrinsic(memcpy,memset,strcmp)
/* Original [00049310,00049bdc), with its fifteen-entry dispatch table at
 * [00049bdc,00049c18). Convert C6 symbols and interleave their COFF lines. */
void omf_convert_debug_symbols(void)
{
    OmfLine *line=omf_line_data,*line_end=line+omf_line_count;
    unsigned char *cursor,*saved;
    OmfC6SymbolRecord following;
    OmfSymbol *symbol;
    OmfDebugFixup *fixup;
    IMAGE_AUX_SYMBOL auxiliary;
    unsigned long auxiliary_stack[64],end_stack[64];
    unsigned long record_offset,offset_base=0,adjustment=0,value,index;
    int depth=0,auxiliary_top=-1,end_top=-1;
    unsigned length,kind;
    unsigned short line_base=0,type,auxiliary_count,storage;
    short section=(short)omf_text_section,symbol_section;
    char name[80],*public_name;

    qsort(omf_debug_fixups,omf_debug_fixup_count,8,omf_compare_debug_fixups);
    if (omf_debug_symbol_data) {
        cursor=omf_debug_symbol_data;
        while ((length=*cursor++)!=0) {
            record_offset=(unsigned long)(cursor-omf_debug_symbol_data-1);
            kind=*cursor++;
            if (length>128) {
                omf_warning("Unexpected: $$SYMBOLS record larger than %d bytes",128);
                length=128;
            }
            memcpy(&omf_debug_record,cursor,length-1);
            cursor+=length-1;
            memset(&auxiliary,0,18);
            name[0]=0;
            switch (kind) {
            case 0x80:
                if (depth==1 && omf_debug_record.block.offset==adjustment) break;
                ++depth;
                omf_append_symbol(".bb",omf_debug_record.block.offset+offset_base,
                                  section,100,0,1);
                while (line<line_end && omf_debug_record.block.offset>line->offset) {
                    omf_append_line(section,line->offset,
                                    (unsigned short)(line->line-line_base));
                    ++line;
                }
                if (line<line_end)
                    auxiliary.Sym.Misc.LnSz.Linenumber=(unsigned short)(line->line-line_base);
                auxiliary_stack[++auxiliary_top]=omf_append_auxiliary(&auxiliary);
                end_stack[++end_top]=omf_debug_record.block.offset+
                    omf_debug_record.block.length+offset_base;
                break;
            case 0x81:
                depth=1;
                name[0]='_';name[1]=0;
                public_name=strncat(name,omf_debug_record.procedure.name,
                                    omf_debug_record.procedure.name_length);
                name[omf_debug_record.procedure.name_length+1]=0;
                section=(short)omf_text_section;
                storage=2;
                symbol=omf_lookup_symbol(public_name);
                if (!symbol) symbol=omf_lookup_symbol(++public_name);
                if (symbol) {
                    storage=symbol->kind==3?2:3;
                    omf_debug_record.procedure.offset=symbol->value;
                    section=symbol->section;
                } else omf_warning("Function from $$SYMBOLS not found: %s",name);
                omf_debug_type_auxiliary(omf_debug_record.procedure.type,&type,&auxiliary);
                index=omf_append_symbol(public_name,omf_debug_record.procedure.offset,
                                       section,(unsigned char)storage,type,1);
                if (symbol) omf_symbol_indices[symbol->ordinal]=index;
                auxiliary.Sym.Misc.LnSz.Linenumber=omf_debug_record.procedure.length;
                auxiliary_stack[++auxiliary_top]=omf_append_auxiliary(&auxiliary);
                omf_append_symbol(".bf",omf_debug_record.procedure.start,section,101,0,1);
                memset(&auxiliary,0,18);
                adjustment=omf_debug_record.procedure.start-omf_debug_record.procedure.offset;
                end_stack[++end_top]=omf_debug_record.procedure.end;
                while (line<line_end && omf_debug_record.procedure.offset>line->offset) {
                    omf_append_line(section,line->offset,
                                    (unsigned short)(line->line-line_base));
                    ++line;
                }
                omf_append_line(section,line->offset,
                                (unsigned short)(line->line-line_base));
                offset_base=omf_debug_record.procedure.offset;
                if (line<line_end) {
                    line_base=line->line;
                    auxiliary.Sym.Misc.LnSz.Linenumber=line_base;
                }
                ++line;
                line_base=0;
                omf_append_auxiliary(&auxiliary);
                break;
            case 0x82:
                if (!depth) break;
                --depth;
                value=end_stack[end_top--];
                omf_append_symbol(depth?".eb":".ef",value,section,
                                  (unsigned char)(depth?100:101),0,1);
                while (line<line_end && value>line->offset) {
                    omf_append_line(section,line->offset,
                                    (unsigned short)(line->line-line_base));
                    ++line;
                }
                if (line<line_end)
                    auxiliary.Sym.Misc.LnSz.Linenumber=(unsigned short)(line->line-line_base);
                index=omf_append_auxiliary(&auxiliary)+1;
                ((PIMAGE_AUX_SYMBOL)&omf_coff_symbols[auxiliary_stack[auxiliary_top--]])->
                    Sym.FcnAry.Function.PointerToNextFunction=index;
                break;
            case 0x84:
                strncpy(name,omf_debug_record.stack.name,omf_debug_record.stack.name_length);
                name[omf_debug_record.stack.name_length]=0;
                auxiliary_count=omf_debug_type_auxiliary(omf_debug_record.stack.type,&type,&auxiliary);
                /* The original uses JAE after comparing an unsigned offset
                 * with zero, so even negative stack offsets take class 9. */
                storage=9;
                saved=cursor;
                length=*cursor++;
                kind=*cursor++;
                if (kind==0x8d) {
                    --length;
                    memcpy(&following,cursor,length);
                    cursor+=length;
                    following.reg.name[(int)following.reg.name_length]=0;
                    if (!strcmp(name,following.reg.name)) {
                        storage=17;
                        omf_debug_record.stack.offset=omf_debug_register(following.reg.number);
                    } else cursor=saved;
                } else cursor=saved;
                omf_append_symbol(name,omf_debug_record.stack.offset,-1,
                                  (unsigned char)storage,type,(unsigned char)auxiliary_count);
                if (auxiliary_count) omf_append_auxiliary(&auxiliary);
                break;
            case 0x85:
                strncpy(name,omf_debug_record.data.name,omf_debug_record.data.name_length);
                name[omf_debug_record.data.name_length]=0;
                auxiliary_count=omf_debug_type_auxiliary(omf_debug_record.data.type,&type,&auxiliary);
                symbol=NULL;
                fixup=omf_find_debug_fixup(record_offset+2);
                if (!fixup) omf_warning("Missing FIXUP for $$SYMBOLS Local: %s",name);
                if (fixup && !fixup->external) {
                    storage=3;
                    symbol_section=fixup->section;
                } else {
                    if (depth) break;
                    symbol=omf_lookup_symbol(name);
                    if (!symbol) {
                        storage=3;
                        symbol_section=(short)omf_data_section;
                    } else {
                        storage=2;
                        if (symbol->kind==1) auxiliary_count=type=0;
                        omf_debug_record.data.offset=symbol->value;
                        symbol_section=symbol->section;
                    }
                }
                if (storage==3 && !symbol_section) break;
                index=omf_append_symbol(name,omf_debug_record.data.offset,symbol_section,
                                       (unsigned char)storage,type,(unsigned char)auxiliary_count);
                if (auxiliary_count) omf_append_auxiliary(&auxiliary);
                if (symbol) omf_symbol_indices[symbol->ordinal]=index;
                break;
            case 0x8b:
                strncpy(name,omf_debug_record.label.name,omf_debug_record.label.name_length);
                name[omf_debug_record.label.name_length]=0;
                omf_append_symbol(name,omf_debug_record.label.offset,(short)omf_text_section,6,0,0);
                break;
            case 0x8d:
                strncpy(name,omf_debug_record.reg.name,omf_debug_record.reg.name_length);
                name[omf_debug_record.reg.name_length]=0;
                value=omf_debug_register(omf_debug_record.reg.number);
                auxiliary_count=omf_debug_type_auxiliary(omf_debug_record.reg.type,&type,&auxiliary);
                omf_append_symbol(name,value,-1,4,type,(unsigned char)auxiliary_count);
                if (auxiliary_count) omf_append_auxiliary(&auxiliary);
                break;
            case 0x8c:
            case 0x8e:
                break;
            default:
                printf("0x%x  ",kind);
                omf_warning("Unknown $$SYMBOLS record type");
                break;
            }
        }
    }
    while (line<line_end) {
        omf_append_line(section,line->offset,(unsigned short)(line->line-line_base));
        ++line;
    }
    free(omf_line_data);
    omf_convert_symbol_types();
    free(omf_debug_fixups);
    if (omf_debug_symbol_data) {
        free(omf_debug_symbol_data);
        omf_debug_symbol_data=NULL;
    }
}
#pragma function(memcpy,memset,strcmp)

/* Original [00049300,0004930d). qsort callback, DWORD subtraction result. */
int omf_compare_debug_fixups(const void *left,const void *right)
{
    return (int)(((const OmfDebugFixup *)left)->offset-
                 ((const OmfDebugFixup *)right)->offset);
}

#pragma intrinsic(memset,memcpy)
/* Original [0004aab0,0004ab9a). Both caller-owned outputs are overwritten
 * before use; only the input type WORD and its defined map entry are read. */
unsigned short omf_debug_type_auxiliary(unsigned short index,
                                       unsigned short *coff_type,
                                       IMAGE_AUX_SYMBOL *auxiliary)
{
    unsigned short derived,base;
    OmfDebugType *type;
    memset(auxiliary,0,18);
    *coff_type=0;
    if (index<=0x1ff) {
        *coff_type=omf_debug_primitive_type(index);
        return 0;
    }
    index-=0x200;
    if (index>=omf_debug_type_count) return 0;
    type=&omf_debug_type_map[index];
    *coff_type=type->type;
    derived=type->type&0x30;
    if (derived==0x30) {
        *auxiliary=*type->value.array;
        return 1;
    }
    base=type->type&15;
    if (base>=8 && base<=10) {
        auxiliary->Sym.TagIndex=type->value.tag;
        if (derived!=0x20) auxiliary->Sym.Misc.TotalSize=type->size;
        return 1;
    }
    return 0;
}
#pragma function(memset,memcpy)

/* Original [00049d00,00049d56), table [00049d58,00049da4).
 * The old converter knows only these register numbers, including no EAX. */
long omf_debug_register(int number)
{
    switch (number) {
    case 8:return 0;
    case 9:return 1;
    case 10:return 2;
    case 11:return 3;
    case 14:return 6;
    case 15:return 7;
    case 26:return 10;
    }
    omf_warning("Unknown OMF register number: %d",number);
    return -1;
}

/* Original [00047b90,00047bd7). Search the sorted eight-byte debug
 * fixups by their unsigned symbol-stream offset. */
OmfDebugFixup *omf_find_debug_fixup(unsigned long offset)
{
    long low=0,high=(long)(omf_debug_fixup_count-1),middle;
    OmfDebugFixup *fixup;
    while (low<=high) {
        middle=(low+high)/2;
        fixup=&omf_debug_fixups[middle];
        if (offset==fixup->offset) return fixup;
        if (offset<fixup->offset) high=middle-1;
        else low=middle+1;
    }
    return NULL;
}

/* Original [0004ad40,0004ad5d). The map pointer is deliberately not reset. */
void omf_finish_debug_types(void)
{
    if (omf_debug_type_map) free(omf_debug_type_map);
    omf_anonymous_type_counter=0;
}
