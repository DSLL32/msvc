/* Macintosh section/debug-resource records and swapper metadata. */
#include <stdlib.h>
#include <string.h>
#include "link-pass.h"
#include "mac.h"
#include <recon.h>

#pragma intrinsic(memcpy,strlen)

#pragma pack(2)
typedef struct MacSectionDebug {
    unsigned char type[4];
    unsigned short section,resource_id,data,unused;
} MacSectionDebug;
#pragma pack()
typedef char MacSectionDebugIs12[sizeof(MacSectionDebug)==12?1:-1];
MacSectionDebug *mac_debug_records;
unsigned short mac_debug_section_index=1;
unsigned short mac_debug_record_count;
extern unsigned short mac_section_count,mac_code_section_count;
extern Contribution *mac_debug,*mac_swap;
extern char *output_name;
extern unsigned long mac_jump_table_size,mac_max_code_size;

/* Original [00037b70,00037b8d). Contents are initialized by the section writer. */
void finalize_mac_debug(void)
{
    mac_debug_records=malloc(mac_section_count*sizeof(MacSectionDebug));
}

/* Original [00037b90,00037c7b). The file's resource IDs and section numbers
   are big-endian; the image section traversal counter advances for every call. */
void write_mac_section_debug(Section *section)
{
    MacSectionDebug *record=&mac_debug_records[mac_debug_record_count];
    unsigned long kind=section->characteristics&0x1e0;
    if (kind==0x20 && section->raw_size && strncmp(section->name,".jtable",8)) {
        memcpy(record->type,"CODE",4);
        record->section=MAC_BIG_SHORT(mac_debug_section_index);
        record->resource_id=MAC_BIG_SHORT(section->mac_resource_id>>16);
        record->data=0;
    } else {
        if ((kind!=0x80 && kind!=0x40) || !section->raw_size) {
            ++mac_debug_section_index;
            return;
        }
        memcpy(record->type,"DATA",4);
        record->section=MAC_BIG_SHORT(mac_debug_section_index);
        record->resource_id=0;
        record->data=0x100;
    }
    record->unused=0;
    ++mac_debug_record_count;
    ++mac_debug_section_index;
}

/* Original [00037c80,00037d95). Preserve the absolute DOS output spelling. */
void write_mac_debug(Image *image)
{
    char *path=_fullpath(NULL,output_name,0);
    unsigned short version=0x200,count=MAC_BIG_SHORT(mac_section_count);
    unsigned long timestamp=MAC_BIG_LONG(image->file.TimeDateStamp);
    seek_file(output_file,mac_debug->file_offset,0);
    write_file(output_file,&version,2);
    write_file(output_file,&timestamp,4);
    write_file(output_file,&count,2);
    write_file(output_file,mac_debug_records,mac_section_count*sizeof(MacSectionDebug));
    write_file(output_file,path,strlen(path)+1);
    free(path);
}

/* Original [00036870,0003696c). The leading version WORD is native-order;
   the remaining count and size fields follow the Mac big-endian format. */
void write_mac_swapper(void)
{
    unsigned short version=2,count=MAC_BIG_SHORT(mac_code_section_count-1);
    unsigned long entries=MAC_BIG_LONG(mac_jump_table_size>>3);
    unsigned long maximum=MAC_BIG_LONG(mac_max_code_size);
    seek_file(output_file,mac_swap->file_offset,0);
    write_file(output_file,&version,2);
    write_file(output_file,&count,2);
    write_file(output_file,&entries,4);
    write_file(output_file,&maximum,4);
}
