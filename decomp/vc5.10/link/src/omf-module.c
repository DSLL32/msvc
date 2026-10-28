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
        /* Original truncation: this helper consumes the low WORD as a byte
         * count and copies only from the first pointer. Passing comment_count
         * therefore preserves just that prefix, not all the declared strings.
         * The remaining .comment storage is the fresh section's defined zero. */
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

void __cdecl read_omf_linnum(void)
{
    WORD segment,line;
    DWORD offset;
    read_omf_index(); /* group index is ignored */
    segment=read_omf_index();
    while(omf_record_length>1) {
        line=(WORD)read_omf_word();
        offset=omf_record_is32?read_omf_dword():(read_omf_word()&0xffff);
        if(!omf_module_name_changed && segment==omf_default_line_segment) {
            omf_line_buffer[omf_line_count].offset=offset;
            omf_line_buffer[omf_line_count].line=line;
            ++omf_line_count;
            if(!(omf_line_count&63))
                omf_line_buffer=reallocate_memory(omf_line_buffer,omf_line_count*8+512);
        }
    }
}

void __cdecl read_omf_linsym(void)
{
    read_omf_byte(); /* flags */
    read_omf_index(); /* public-name index */
    /* The original consumes these COMDAT line pairs without retaining them.
     * Ordinary LINNUM records use the separate line-buffer/list path. */
    while(omf_record_length>1) {
        read_omf_word();
        if(omf_record_is32)read_omf_dword();
        else read_omf_word();
    }
}
