/* Decode genuine COFF short-import records and collect archive export names. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dumper.h"
#include "files.h"
#include "full-link-functions.h"
#include "definition-buffer-construct.h"
#include "definition-buffer-set.h"
#include "definition-buffer-cleanup.h"
#include <recon.h>

extern "C" {
extern FILE *map_output;
extern BYTE file_access_statistics;
extern DWORD dump_archive_export_capacity;
char *__stdcall duplicate_argument(const char *);
char *__stdcall display_symbol_name(const char *,DWORD);
const char *__stdcall dumper_machine_name(DWORD);
void __cdecl record_file_access(void);
}
#pragma intrinsic(strlen,memcpy,memset)
extern "C" void __stdcall dump_short_import(int file,const CoffImportHeader *header)
{
    DefinitionBuffer buffer;
    char *name,*display,*dll,*copy,*suffix;
    const char *kind,*name_type,*date;
    FileRecord *record;
    DumperArchiveExport *entries,*entry;
    DWORD count,capacity,index;
    BYTE decorated_machine;
    if((LONG)header->data_size>0)buffer.Grow(header->data_size);
    buffer.end=buffer.data;
    name=(char *)buffer.data;
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    if(record->flags&8)mapped_read(record,name,header->data_size);
    else buffered_read(record,name,header->data_size);
    if(dumper_context->options.dump_exports) {
        copy=duplicate_argument(name);
        count=dump_archive_export_count+1;
        if(count>dump_archive_export_capacity) {
            capacity=dump_archive_export_capacity*3/2;
            if(count>capacity)capacity=count;
            entries=(DumperArchiveExport *)operator new(capacity*sizeof(*entries));
            if(entries) {
                if(dump_archive_exports) {
                    for(index=0;index<dump_archive_export_count;++index)
                        entries[index]=dump_archive_exports[index];
                    operator delete(dump_archive_exports);
                }
                dump_archive_exports=entries;
                dump_archive_export_capacity=capacity;
            } else goto headers;
        }
        dump_archive_export_count=count;
        entry=dump_archive_exports+count-1;
        entry->name=copy;
        entry->ordinal=header->ordinal_hint;
        entry->has_ordinal=(BYTE)(header->name_type==0);
        memset(entry->reserved09,0,sizeof(entry->reserved09));
    }
headers:
    if(!dumper_context->options.dump_headers)return;
    dll=name+strlen(name)+1;
    display=display_symbol_name(name,0);
    switch(header->kind) {
    case 0:kind="code";break;
    case 1:kind="data";break;
    case 2:kind="const";break;
    default:kind="unknown";break;
    }
    switch(header->name_type) {
    case 0:name_type="ordinal";break;
    case 1:name_type="name";break;
    case 2:name_type="no prefix";break;
    case 3:name_type="undecorate";break;
    default:name_type="unknown";break;
    }
    fprintf(map_output,"\n  Version      : %hX\n  Machine      : %hX (%s)\n  TimeDateStamp: %08lX",
        header->version,header->machine,dumper_machine_name(header->machine),header->timestamp);
    date=ctime((const time_t *)&header->timestamp);
    if(date)fprintf(map_output," %s",date);
    else fputc('\n',map_output);
    fprintf(map_output,"  SizeOfData   : %08lX\n  DLL name     : %s\n  Symbol name  : %s\n  Type         : %s\n  Name type    : %s\n",
        header->data_size,dll,display,kind,name_type);
    if(display!=name)free(display);
    decorated_machine=(BYTE)(header->machine==0x14c || header->machine==0x1a2 || header->machine==0x1a6);
    switch(header->name_type) {
    case 0:
        fprintf(map_output,"  Ordinal      : %hu\n",header->ordinal_hint);
        break;
    case 1:
        fprintf(map_output,"  Hint         : %hu\n  Name         : %s\n",header->ordinal_hint,name);
        break;
    case 2:case 3:
        if(*name=='?' || *name=='@' || (decorated_machine && *name=='_'))++name;
        if(header->name_type==3) {
            suffix=strchr(name,'@');
            if(suffix)*suffix=0;
        }
        fprintf(map_output,"  Hint         : %hu\n  Name         : %s\n",header->ordinal_hint,name);
        break;
    }
}
