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
void __stdcall reserve_byte_buffer(ByteBuffer *buffer,unsigned bytes) { __asm { int 3 } }
