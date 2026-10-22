/* COFF file-format reads and reusable symbol/string buffers. */
#include "object.h"
#include "files.h"
#include "link-pass.h"
TextBuffer coff_string_buffer, coff_symbol_buffer;
unsigned char coff_strings_cached, coff_symbols_cached;
char section_name_buffer[9];

unsigned read_coff_header(int file,IMAGE_FILE_HEADER *header)
{
    return read_file(file,header,sizeof(*header));
}

unsigned read_section_header(int file,IMAGE_SECTION_HEADER *header)
{
    return read_file(file,header,sizeof(*header));
}

char *coff_section_name(const char name[8],char *strings)
{
    unsigned long offset;
    strncpy(section_name_buffer,name,8);
    section_name_buffer[8]=0;
    if (section_name_buffer[0]=='/' &&
        sscanf(section_name_buffer+1,"%7lu",&offset)==1)
        return strings+offset;
    return section_name_buffer;
}

char *read_coff_strings(const char *path,unsigned long offset,unsigned long *size)
{
    unsigned long length;
    char *strings=NULL;
    coff_strings_cached=1;
    if (!offset) return NULL;
    length=file_size(input_file);
    if (offset+4 <= length) {
        seek_file(input_file,offset,SEEK_SET);
        read_file(input_file,size,4);
        if (offset+*size <= length) {
            if (offset && *size && *size!=4) {
                reserve_text(&coff_string_buffer,*size+1);
                strings=coff_string_buffer.data;
                seek_file(input_file,offset,SEEK_SET);
                read_file(input_file,strings,*size);
                if (strings[*size-1]) warning_message(path,90,"","");
            } else *size=0;
            return strings;
        }
    }
    warning_message(path,111,"","");
    *size=0;
    return NULL;
}

PIMAGE_SYMBOL read_coff_symbols(unsigned long offset,unsigned long count)
{
    unsigned long bytes=count*IMAGE_SIZEOF_SYMBOL;
    seek_file(input_file,offset,SEEK_SET);
    coff_symbols_cached=1;
    reserve_text(&coff_symbol_buffer,bytes);
    read_file(input_file,coff_symbol_buffer.data,bytes);
    return (PIMAGE_SYMBOL)coff_symbol_buffer.data;
}

/* A single record is reused when symbol caching was not possible. */
IMAGE_SYMBOL streamed_symbol;

PIMAGE_SYMBOL next_coff_symbol(PIMAGE_SYMBOL *cursor,unsigned char auxiliary)
{
    PIMAGE_SYMBOL symbol=*cursor;
    if (symbol) {
        *cursor=symbol+1;
        return symbol;
    }
    read_coff_symbol(input_file,&streamed_symbol);
    return &streamed_symbol;
}

void release_coff_strings(char *strings)
{
    coff_strings_cached=0;
}

void release_coff_symbols(PIMAGE_SYMBOL symbols)
{
    coff_symbols_cached=0;
}

TextBuffer coff_relocation_buffer;
unsigned char coff_relocations_cached;
PIMAGE_RELOCATION allocate_coff_relocations(unsigned long count)
{
    coff_relocations_cached=1;
    reserve_text(&coff_relocation_buffer,count*IMAGE_SIZEOF_RELOCATION);
    return (PIMAGE_RELOCATION)coff_relocation_buffer.data;
}

unsigned read_coff_relocations(int file,PIMAGE_RELOCATION records,unsigned long count)
{
    return read_file(file,records,count*IMAGE_SIZEOF_RELOCATION);
}

void release_coff_relocations(PIMAGE_RELOCATION records)
{
    coff_relocations_cached=0;
}

void read_optional_header(int file,IMAGE_OPTIONAL_HEADER *header,unsigned short size)
{
    if (size) {
        if (size>=sizeof(*header)) size=sizeof(*header);
        read_file(file,header,size);
    }
}

extern unsigned char mac_image,mac_swappable;
void make_section_header(Section *section,IMAGE_SECTION_HEADER *header)
{
    strncpy((char *)header->Name,section->name,8);
    header->Misc.VirtualSize=section->virtual_size;
    header->VirtualAddress=section->rva;
    header->SizeOfRawData=section->raw_size;
    header->PointerToRawData=section->file_offset;
    header->PointerToRelocations=0;
    header->PointerToLinenumbers=section->line_offset;
    header->NumberOfRelocations=0;
    header->NumberOfLinenumbers=section->line_count;
    header->Characteristics=section->characteristics;
    if (mac_image) {
        if (section->characteristics&IMAGE_SCN_CNT_UNINITIALIZED_DATA)
            header->SizeOfRawData=0;
        if (section->characteristics&IMAGE_SCN_CNT_CODE)
            header->Characteristics |= 0x400000;
        if (mac_swappable && (section->characteristics&0x08000000UL))
            header->Characteristics |= 0x1800000;
        header->Misc.VirtualSize=section->raw_size;
    }
}


unsigned write_section_header(int file,IMAGE_SECTION_HEADER *header)
{
    return write_file(file,header,sizeof(*header));
}


unsigned write_coff_header(int file,IMAGE_FILE_HEADER *header)
{
    /* This target tests the probe address and sets both reverse-byte flags. */
    if (is_big_endian)
        header->Characteristics |= IMAGE_FILE_BYTES_REVERSED_LO|IMAGE_FILE_BYTES_REVERSED_HI;
    return write_file(file,header,sizeof(*header));
}


void write_optional_header(int file,IMAGE_OPTIONAL_HEADER *header,unsigned short size)
{
    if (size) write_file(file,header,size);
}


unsigned char skip_coff_auxiliaries(PIMAGE_SYMBOL *cursor,unsigned char count)
{
    unsigned char remaining=count;
    IMAGE_SYMBOL temporary;
    if (*cursor) *cursor+=count;
    else while (remaining) {
        read_coff_symbol(input_file,&temporary);
        --remaining;
    }
    return count;
}

