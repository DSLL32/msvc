#include <string.h>
#include <wchar.h>
#include "command.h"
#include "preprocess.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

int rc_source_depth=-1,rc_source_encoding;
WCHAR *rc_source_cursor;
FILE *rc_source_file;
RcSourceFrame rc_source_frames[1024];
/* The first observed marker is an empty mutable wide string. */
WCHAR rc_source_marker[1];

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
