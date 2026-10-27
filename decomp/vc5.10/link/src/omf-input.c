#include <windows.h>
#include "memory.h"
#include "omf.h"
#include <recon.h>

/* Both byte contributions are explicitly unsigned. The original defines
 * all of EAX here, even where its current caller consumes only AX. */
DWORD __cdecl read_omf_word(void)
{
    BYTE bytes[2];
    read_omf_bytes(bytes,2);
    return (DWORD)bytes[0]|((DWORD)bytes[1]<<8);
}

DWORD __stdcall read_omf_bytes(void *buffer,DWORD count)
{
    DWORD received=(DWORD)fread(buffer,1,count,omf_stream);
    if(received!=count)omf_error("Bad read of object file");
    /* The byte count is a full DWORD transport; the record remainder uses
     * only its actual low WORD, matching the original SUB of SI. */
    omf_record_length=(SHORT)((WORD)omf_record_length-(WORD)count);
    return received;
}

char *__stdcall read_omf_string(DWORD unused_formal)
{
    BYTE length;
    char *text;
    /* The complete original consumes no bits of its physical DWORD formal,
     * but its genuine RET4 still removes that caller-provided stack slot. */
    read_omf_bytes(&length,1);
    text=allocate_memory((DWORD)length+1);
    read_omf_bytes(text,length);
    text[length]=0;
    return text;
}

void __cdecl finish_omf_record(void)
{
    if(omf_record_length>1) {
        if(fseek(omf_stream,(LONG)omf_record_length-1,SEEK_CUR))
            omf_error("Bad seek on object file");
        omf_record_length=1;
    }
}

DWORD __cdecl read_omf_byte(void)
{
    BYTE value;
    /* Successful read_omf_bytes leaves the full fread count 1 in EAX;
     * the original replaces AL, so the complete normal result is unsigned.
     * Keep those defined upper bits through native function instrumentation. */
    read_omf_bytes(&value,1);
    return value;
}

DWORD __cdecl read_omf_dword(void)
{
    BYTE bytes[4];
    read_omf_bytes(bytes,4);
    return (DWORD)bytes[0]|((DWORD)bytes[1]<<8)|
           ((DWORD)bytes[2]<<16)|((DWORD)bytes[3]<<24);
}

/* The currently recovered consumers use AX only (or ignore the result).
 * The original two-byte path carries unspecified volatile upper EAX bits;
 * this WORD contract makes no zero-extension or full-register guarantee. */
WORD __cdecl read_omf_index(void)
{
    BYTE first,second;
    read_omf_bytes(&first,1);
    if(first&0x80) {
        read_omf_bytes(&second,1);
        return (WORD)(((WORD)(first&0x7f)<<8)|second);
    }
    return first;
}
