#include <string.h>
#include <wchar.h>
#include "command.h"
#include "directives.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

int rc_source_depth=-1,rc_source_encoding;
WCHAR *rc_source_cursor;
FILE *rc_source_file;
RcSourceFrame rc_source_frames[1024];
/* Bounded contextual include-name storage within the observed loader-zero
 * span. The original C array size is unknown; its next field starts516B later.
 * Two bytes between this514B model and that field remain unclaimed. */
WCHAR rc_source_marker[257];

int __stdcall rc_open_source(WCHAR *path,int first)
{
    RcSourceFrame *source;
    WCHAR *buffer;
    RcMacro marker;
    unsigned saved_line;

    if(!path) rc_source_file=stdin;
    else{
        WideCharToMultiByte(GetACP(),0,path,-1,rc_cpp_path_buffer,257,NULL,NULL);
        rc_source_file=fopen(rc_cpp_path_buffer,"rb");
        if(!rc_source_file){
            if(first!=1) return 0;
            rc_error_template=rc_message(1005);
            rc_format(rc_error_text,2048,rc_error_template,rc_cpp_path_buffer);
            rc_input_fatal(1005);
            return 0;
        }
    }
    if(++rc_source_depth>=1024){
        rc_error_template=rc_message(1014);
        rc_format(rc_error_text,2048,rc_error_template,1024);
        rc_input_fatal(1014);
    }
    source=&rc_source_frames[rc_source_depth];
    buffer=rc_allocate(0x2006);
    if(!buffer){
        strcpy(rc_error_text,rc_message(1002));
        rc_input_fatal(1002);
        return 0;
    }
    source->capacity=0x1002;
    source->saved_cursor=rc_source_cursor;
    source->saved_line=rc_preparing_line;
    source->chars_read=0;
    source->seek_result=0;
    source->file=rc_source_file;
    source->buffer=buffer;
    source->encoding=rc_detect_encoding(rc_source_file);
    if(!source->encoding){
        rc_error_template=rc_message(4413);
        rc_format(rc_error_text,2048,rc_error_template,path);
        rc_input_warning(4413);
        source->encoding=1;
    }
    rc_source_encoding=source->encoding;
    rc_source_cursor=buffer;
    rc_refill_source();
    source->filename=rc_duplicate_source_name(rc_filename);
    wcsncpy(rc_filename_storage,path,257);
    rc_preparing_line=0;
    if(rc_cpp_E){
        rc_emit_source_line();
        rc_write_preprocessed(L"\r\n",4,1,rc_cpp_output_file);
    }
    saved_line=rc_preparing_line;
    rc_preparing_line=rc_source_depth;
    marker.next=NULL;
    marker.name=L"!";
    marker.text=rc_source_marker;
    marker.parameters=0;
    marker.flags=0;
    rc_list_macro(&marker);
    if(rc_source_depth>0){
        marker.next=NULL;
        marker.name=L"$";
        marker.text=rc_filename;
        marker.parameters=0;
        marker.flags=0;
        rc_list_macro(&marker);
    }
    rc_preparing_line=saved_line;
    rc_scan_line_start();
    return 1;
}

int __cdecl rc_refill_source(void)
{
    RcSourceFrame *source=&rc_source_frames[rc_source_depth];
    WCHAR *buffer=source->buffer,*cursor;
    unsigned char *text,*byte;
    size_t count;
    int index;

    if(rc_source_cursor-buffer<source->chars_read){
        if(rc_source_cursor[-1]!=26) return 0;
        --rc_source_cursor;
        return 1;
    }
    rc_source_cursor=buffer;
    if(source->encoding==1){
        text=rc_allocate(source->capacity+1);
        if(!text){
            strcpy(rc_error_text,rc_message(1002));
            rc_input_fatal(1002);
        }
        source->seek_result=fseek(rc_source_file,0,SEEK_CUR);
        count=fread(text,1,source->capacity,rc_source_file);
        index=0;
        byte=text;
        while(index<(int)count){
            if(IsDBCSLeadByteEx(rc_codepage,*byte)){
                ++index;
                ++byte;
            }
            ++index;
            ++byte;
        }
        if(index>(int)count){
            fseek(rc_source_file,-1,SEEK_CUR);
            text[--count]=0;
        }
        source->chars_read=MultiByteToWideChar(rc_codepage,1,(char *)text,count,
                                             buffer,source->capacity);
        rc_release(text);
    }else{
        count=fread(buffer,2,source->capacity,rc_source_file);
        source->chars_read=count;
        if(source->encoding==3){
            cursor=buffer;
            for(index=0;index<(int)count;++index,++cursor)
                *cursor=(*cursor>>8)|(*cursor<<8);
        }
    }
    if(count){
        buffer[source->chars_read]=0;
        return 0;
    }
    *buffer=0;
    return 1;
}

WCHAR *__stdcall rc_duplicate_source_name(const WCHAR *path)
{
    WCHAR *copy;
    size_t length=wcslen(path);
    copy=rc_allocate(length*2+2);
    return wcscpy(copy,path);
}

void __cdecl rc_emit_source_line(void)
{
    WCHAR *end;
    swprintf(rc_macro_name,L"#line %d ",rc_preparing_line+1);
    rc_write_preprocessed(rc_macro_name,wcslen(rc_macro_name)*2,1,rc_cpp_output_file);
    end=rc_quote_source_filename(rc_macro_name,rc_filename);
    rc_write_preprocessed(rc_macro_name,(end-rc_macro_name)*2,1,rc_cpp_output_file);
}

void __stdcall rc_write_preprocessed(const void *data,unsigned size,unsigned count,FILE *file)
{
    if(fwrite(data,size,count,file)!=count)
        rc_fatal(rc_message(1122));
}

WCHAR *__stdcall rc_quote_source_filename(WCHAR *destination,const WCHAR *path)
{
    WCHAR *cursor=destination;
    *cursor++=L'"';
    while((*cursor=*path)!=0){
        if(rc_character_token(*path)==3)
            *++cursor=L'\\';
        ++path;
        ++cursor;
    }
    *cursor=L'"';
    return cursor+1;
}

void __cdecl rc_scan_line_start(void)
{
    WCHAR character;
    unsigned previous_line;

    ++rc_preparing_line;
    for(;;){
        character=*rc_source_cursor++;
        switch(rc_character_token(character)){
        case 1:
        case 44:
            break;
        case 2:
            if(!rc_scan_comment()) goto unread;
            break;
        case 3:
            if(rc_source_cursor[-1] && rc_source_cursor[-1]!=26){
                if(!rc_splice_line()) goto unread;
            }else if(rc_refill_source()) return;
            break;
        case 5:
            previous_line=rc_prepared_lines++;
            if(!(previous_line&63)) rc_notify_progress(1,rc_prepared_lines);
            ++rc_preparing_line;
            if(rc_cpp_active)
                rc_write_preprocessed(L"\r",2,1,rc_cpp_output_file);
            /* The original emits CR, then the source's LF. */
        case 0:
            if(rc_cpp_active)
                rc_write_preprocessed(rc_source_cursor-1,2,1,rc_cpp_output_file);
            else{
                while(rc_character_class(*rc_source_cursor++)&0x40) ;
                --rc_source_cursor;
            }
            break;
        case 11:
            rc_handle_directive();
            break;
        default:
        unread:
            --rc_source_cursor;
            return;
        }
    }
}

/* Close the exhausted source and restore the including source, if any. */
unsigned short __cdecl rc_open_source_frame(void)
{
    RcMacro marker;
    RcSourceFrame *source;
    unsigned saved_line;

    if(rc_source_depth==-1)return 0;
    if(rc_source_file)fclose(rc_source_file);
    --rc_source_depth;
    saved_line=rc_preparing_line;
    rc_preparing_line=rc_source_depth;
    marker.next=NULL;
    marker.name=L"!";
    marker.text=L"";
    marker.parameters=0;
    marker.flags=0;
    rc_list_macro(&marker);
    rc_preparing_line=saved_line;
    ++rc_source_depth;
    source=&rc_source_frames[rc_source_depth];
    rc_copy_wide_end(rc_filename_storage,source->filename);
    saved_line=rc_preparing_line;
    rc_preparing_line=source->saved_line;
    rc_source_cursor=source->saved_cursor;
    rc_release(source->buffer);
    if(--rc_source_depth<0){
        rc_preparing_line=saved_line;
        return 0;
    }
    source=&rc_source_frames[rc_source_depth];
    rc_source_file=source->file;
    rc_source_encoding=source->encoding;
    if(rc_cpp_E){
        rc_write_preprocessed(L"\r\n",4,1,rc_cpp_output_file);
        rc_emit_source_line();
    }
    return 1;
}
