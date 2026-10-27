#include <windows.h>
#include <string.h>
#include "memory.h"
#include "arguments.h"
#include "options.h"
#include "omf.h"
#include <recon.h>

char *omf_module_name;
#pragma intrinsic(strcmp,strlen,memset)
void __cdecl read_omf_theadr(void)
{
    char *name=read_omf_string(0),*cursor;
    BYTE auxiliary[18];
    WORD length;
    DWORD auxiliary_count;
    unsigned i;
    omf_module_count=(WORD)(omf_module_count+1);
    if((SHORT)omf_module_count>1) {
        omf_module_name_changed=(BYTE)(strcmp(omf_module_name,name)!=0);
        return;
    }
    omf_module_name=name;
    length=(WORD)(strlen(name)+1);
    auxiliary_count=((DWORD)length+17)/18;
    append_omf_symbol(".file",omf_symbol_count,(DWORD)IMAGE_SYM_DEBUG,
                      IMAGE_SYM_CLASS_FILE,auxiliary_count);
    /* Preserve the actual NUL-terminated loop, independently of the count
     * computed above, including its exact-multiple-of-18 edge behavior. */
    cursor=name;
    while(*cursor) {
        memset(auxiliary,0,sizeof(auxiliary));
        for(i=0;i<sizeof(auxiliary) && *cursor;++i)auxiliary[i]=(BYTE)*cursor++;
        append_omf_auxiliary(auxiliary);
    }
}

char *omf_comments[256]; /*478650, actual bounded comment pointers*/
void __stdcall read_omf_comment(const char *input)
{
    BYTE kind;
    SHORT length;
    char *text;
    WORD from,to;
    unsigned bucket;
    OmfNameEntry *entry;
    read_omf_byte(); /* comment attributes */
    kind=(BYTE)read_omf_byte();
    switch(kind) {
    case 0xa0:
        omf_compile_flags=1;
        return;
    case 0xa4:
        if((SHORT)omf_comment_count>=256) {
            link_warning(input,4085,256);
            return;
        }
        text=allocate_memory((DWORD)(LONG)omf_record_length);
        length=omf_record_length;
        read_omf_bytes(text,(DWORD)((LONG)length-1));
        text[length-1]=0;
        omf_comments[(SHORT)omf_comment_count]=text;
        omf_comment_count=(WORD)(omf_comment_count+1);
        omf_comment_bytes+=(LONG)length;
        return;
    case 0xa8: case 0xa9:
        while(omf_record_length>1) {
            from=(WORD)omf_external_map[(SHORT)read_omf_index()];
            to=(WORD)omf_external_map[(SHORT)read_omf_index()];
            for(bucket=0;bucket<211;++bucket) {
                for(entry=omf_symbol_hash[bucket];entry;entry=entry->next)
                    if(entry->external_index==from)break;
                if(entry && !entry->section) {
                    entry->weak_default_index=to;
                    entry->kind=(WORD)(kind==0xa8?5:6);
                }
            }
        }
        break;
    }
}

void __cdecl read_omf_modend(void)
{
    DWORD section;
    WORD i;
    if(omf_comment_count) {
        section=create_omf_section(".comment",1,0,omf_comment_bytes,0x840);
        /* Preserve the actual pointer-to-pointer call and low-WORD count;
         * its separate body remains a named pending frontier. */
        write_omf_indirect_section_data(section,0,omf_comments,omf_comment_count);
        for(i=0;(SHORT)i<(SHORT)omf_comment_count;++i) {
            release_memory(omf_comments[(SHORT)i]);
            omf_comments[(SHORT)i]=NULL;
        }
    }
}

void __cdecl write_omf_lines(void)
{
    OmfLine *line=omf_line_buffer,*end=line+omf_line_count;
    for(;line<end;++line)
        append_omf_line(omf_default_line_segment,line->offset,line->line);
}
