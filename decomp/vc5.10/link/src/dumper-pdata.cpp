/* Inspect the serialized procedure records; no target code is executed. */
#include <windows.h>
#include <stdio.h>
#include "dumper.h"
#include "files.h"
#include "input.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern FILE *map_output;
extern BYTE file_access_statistics;
void __cdecl record_file_access(void);

BOOL __stdcall uses_compact_pdata(LinkContext *context)
{
    WORD machine=context->file_header.Machine;
    /* These Windows CE machine values are used by LINK but have no public
     * spelling in this shipped WINNT.H. */
    return machine==IMAGE_FILE_MACHINE_POWERPC || machine==0x1a2 /* SH3 */ ||
        machine==0x1a6 /* SH4 */ || machine==0x1c0 /* ARM */;
}

typedef struct DumperProcedure {
    DWORD begin,end,exception_handler,exception_data,prolog_end;
} DumperProcedure;
typedef char DumperProcedureSize[(sizeof(DumperProcedure)==20)?1:-1];

#define PDATA_READ(data,bytes) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[current_input_file]; \
    if(record->flags&8)mapped_read(record,(data),(bytes)); \
    else buffered_read(record,(data),(bytes)); \
} while(0)

void __stdcall dump_object_pdata(LinkContext *context,IMAGE_SECTION_HEADER *section,
                                DWORD section_index)
{
    DWORD count,offset,compact[2],packed,shift;
    FileRecord *record;
    DumperProcedure procedure;
    if(uses_compact_pdata(context)) {
        count=section->SizeOfRawData/8;
        fprintf(map_output,"\nFunction Table #%d (%lu)\n\n",(SHORT)section_index,count);
        fprintf(map_output,"         Begin    End      Prolog   Excpt 32bit\n\n");
        offset=0;
        while(count--) {
            PDATA_READ(compact,sizeof(compact));
            packed=compact[1];
            shift=((packed>>30)&1)+1;
            fprintf(map_output,"%08lX %08lX %08lX %08lX   %c     %c\n",offset,
                compact[0],compact[0]+(((packed>>8)&0x3fffff)<<shift),
                compact[0]+((packed&0xff)<<shift),
                packed&0x80000000 ? 'Y':'N',packed&0x40000000 ? 'Y':'N');
            offset+=8;
        }
    } else {
        count=section->SizeOfRawData/sizeof(procedure);
        fprintf(map_output,"\nFunction Table #%d (%lu)\n\n",(SHORT)section_index,count);
        fprintf(map_output,"              Begin       End    Excptn   ExcpDat PrologEnd\n\n");
        offset=0;
        while(count--) {
            PDATA_READ(&procedure,sizeof(procedure));
            fprintf(map_output,"  %08lX %08lX  %08lX  %08lX  %08lX  %08lX\n",offset,
                procedure.begin,procedure.end,procedure.exception_handler,
                procedure.exception_data,procedure.prolog_end);
            offset+=sizeof(procedure);
        }
    }
}
#undef PDATA_READ
}
