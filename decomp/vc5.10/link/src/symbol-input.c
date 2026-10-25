#include <windows.h>
#include "input.h"
#include "full-link-functions.h"
#include <recon.h>

extern const char *default_phase;
extern BYTE file_access_statistics;
BYTE module_symbols_active,module_symbols_mapped;
ByteBuffer symbol_read_buffer;
void __stdcall reserve_byte_buffer(ByteBuffer *,unsigned);

IMAGE_SYMBOL *__stdcall read_module_symbols(DWORD offset,DWORD count,BYTE force_copy)
{
    const char *previous_phase=default_phase;
    DWORD bytes=count*18,error=0;
    IMAGE_SYMBOL *symbols=NULL;
    FileRecord *file;
    default_phase="ReadSymbolTable";
    module_symbols_active=1;
    if (!force_copy)
        symbols=(IMAGE_SYMBOL *)map_file_range(current_input_file,offset,bytes);
    module_symbols_mapped=(symbols!=NULL);
    /* The mapped path deliberately retains the current phase in the original. */
    if (symbols) return symbols;
    reserve_byte_buffer(&symbol_read_buffer,bytes);
    symbols=(IMAGE_SYMBOL *)symbol_read_buffer.data;
    seek_file(current_input_file,offset,0,&error);
    if (file_access_statistics) record_file_access();
    file=file_slots[current_input_file];
    if (file->flags & 8) mapped_read(file,symbols,bytes);
    else buffered_read(file,symbols,bytes);
    default_phase=previous_phase;
    return symbols;
}

BYTE module_strings_active,module_strings_mapped;
ByteBuffer string_read_buffer;
char *__stdcall read_module_string_table(const char *name,DWORD offset,DWORD *size)
{
    char *strings;
    DWORD length,error=0;
    FileRecord *file;
    module_strings_active=1;
    /* With no table the original leaves the caller's output untouched. */
    if (!offset) return NULL;
    module_strings_mapped=0;
    strings=(char *)map_file_range(current_input_file,offset,4);
    if (strings) {
        *size=*(DWORD *)strings;
        if (!*size) return NULL;
        if (*size==4) { *size=0; return NULL; }
        strings=(char *)map_file_range(current_input_file,offset,*size);
        if (strings && !strings[*size-1]) {
            module_strings_mapped=1;
            return strings;
        }
    }
    length=(DWORD)get_file_length(current_input_file);
    if ((DWORD)(offset+4)>length) goto truncated;
    seek_file(current_input_file,offset,FILE_BEGIN,&error);
    if (file_access_statistics) record_file_access();
    file=file_slots[current_input_file];
    if (file->flags & 8) mapped_read(file,size,4);
    else buffered_read(file,size,4);
    if ((DWORD)(offset+*size)>length) goto truncated;
    if (!*size) return NULL;
    if (*size==4) { *size=0; return NULL; }
    reserve_byte_buffer(&string_read_buffer,*size+1);
    strings=(char *)string_read_buffer.data;
    error=0;
    seek_file(current_input_file,offset,FILE_BEGIN,&error);
    if (file_access_statistics) record_file_access();
    file=file_slots[current_input_file];
    if (file->flags & 8) mapped_read(file,strings,*size);
    else buffered_read(file,strings,*size);
    if (strings[*size-1]) link_warning(name,4019);
    return strings;
truncated:
    link_warning(name,4040);
    *size=0;
    return NULL;
}

/* This routine consumes only the public 18-byte IMAGE_SYMBOL prefix, including
 * when the caller supplies an extended StringNode. Short names are borrowed. */
const char *__stdcall get_symbol_name(StringNode *symbol,const void *strings,
    WORD *kind,BYTE *is_import)
{
    const char *name;
    if (!symbol->symbol.N.Name.Short) {
        if (kind) *kind=0;
        name=(const char *)strings+symbol->symbol.N.Name.Long;
    } else {
        if (kind) *kind=1;
        name=(const char *)symbol->symbol.N.ShortName;
    }
    if (is_import) *is_import=(BYTE)((unsigned char)*name==0x7f);
    return name;
}

/* This is the original18-byte BSS owner at4948c8, used only by streamed input. */
static IMAGE_SYMBOL streamed_symbol;
void __stdcall read_file_symbol(int,IMAGE_SYMBOL *);
IMAGE_SYMBOL *__stdcall next_coff_symbol(IMAGE_SYMBOL **cursor)
{
    IMAGE_SYMBOL *symbol=*cursor;
    if(symbol) {
        *cursor=symbol+1;
        return symbol;
    }
    read_file_symbol(current_input_file,&streamed_symbol);
    return &streamed_symbol;
}

BYTE __stdcall emit_symbol_class(BYTE symbol_class,LinkOptions *options)
{
    if(!(options->debug_formats & 1)) return 0;
    switch(symbol_class) {
    case IMAGE_SYM_CLASS_EXTERNAL:
    case IMAGE_SYM_CLASS_STATIC:
    case IMAGE_SYM_CLASS_FAR_EXTERNAL:
    case IMAGE_SYM_CLASS_WEAK_EXTERNAL:
        return 1;
    case IMAGE_SYM_CLASS_FILE:
        return options->debug_level!=1;
    default:
        return 0;
    }
}
