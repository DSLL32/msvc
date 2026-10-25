#include <wctype.h>
#include "command.h"
#include "resource-input.h"
#include <recon.h>

/* Distinct DWORD stores at original RVAs 25d3c, 27f78, 27f7c and 27f84.
 * The first field's role beyond initialization remains unobserved. */
DWORD rc_resource_status,rc_resource_file_index,rc_resource_line;
FILE *rc_resource_input_file;

int __stdcall rc_initialize_input(FILE *file)
{
    rc_resource_status=0;
    rc_resource_file_index=0;
    rc_resource_line=1;
    rc_resource_input_file=file;
    rc_next_resource_character();
    return 1;
}

/* Separate WORD at original RVA27f8c, filled by the underlying reader. */
WCHAR rc_resource_character;

unsigned __cdecl rc_next_resource_character(void)
{
    if(rc_read_resource_character()!=127 && rc_resource_character==L';'){
        while(rc_read_resource_character()!=127){
            if(rc_resource_character==L'\n')break;
        }
    }
    return rc_resource_character;
}

/* Whole-token backups use the exact field widths copied by the original
 * token copier. Unidentified field roles and untouched string tails stay
 * unknown; a local backup is deliberately not initialized. */
RcResourceToken rc_resource_token;
int rc_resource_line_start=1;
DWORD rc_resource_total_lines,rc_resource_recording;
char rc_resource_filename[MAX_PATH];

unsigned __cdecl rc_read_resource_character(void)
{
    WCHAR filename[MAX_PATH],*cursor,lower;
    RcResourceToken saved_token;
    unsigned dot=0,skip=0,escape=0,saved_start,previous_lines;

read_again:
    rc_resource_character=(WCHAR)rc_read_resource_code_unit();
    if(rc_resource_character==L'\t'||rc_resource_character==L' ')
        goto check_skip;
    if(!rc_resource_character){
        rc_resource_character=127;
        goto finish;
    }
    if(rc_resource_character==L'\r'){
        rc_resource_character=L' ';
        goto check_skip;
    }
    if(rc_resource_character==L'\n'){
        ++rc_resource_line;
        previous_lines=rc_resource_total_lines++;
        rc_resource_line_start=1;
        if(!(previous_lines&31))rc_notify_progress(2,rc_resource_total_lines);
        goto check_skip;
    }
    if(rc_resource_character==0xfeff)goto read_again;
    if(rc_resource_character==0xe000){
#ifdef RECON_NATIVE_TEST
        /* APSTUDIO directive records are not yet an encountered input. */
        __asm int 3
#endif
        saved_start=rc_resource_line_start;
        rc_read_apstudio_directive(1,rc_resource_character);
        rc_resource_line_start=saved_start;
        goto read_again;
    }
    if(rc_resource_character!=L'#'||!rc_resource_line_start){
        rc_resource_line_start=0;
        goto check_skip;
    }
    dot=0;
    lower=(WCHAR)rc_read_resource_code_unit();
    if(lower==L'p'){
        if(rc_read_resource_code_unit()!=L'r'||rc_read_resource_code_unit()!=L'a'||
           rc_read_resource_code_unit()!=L'g'||rc_read_resource_code_unit()!=L'm'||
           rc_read_resource_code_unit()!=L'a')goto malformed;
        rc_copy_resource_token(&saved_token,&rc_resource_token);
        rc_scan_resource_token(0);
        rc_scan_resource_token(0);
        rc_scan_resource_token(0x8000);
        rc_codepage=rc_resource_token.value;
        while(rc_resource_character!=L'\n')
            rc_resource_character=(WCHAR)rc_read_resource_code_unit();
        rc_copy_resource_token(&rc_resource_token,&saved_token);
        goto read_again;
    }
    if(lower!=L'l'||rc_read_resource_code_unit()!=L'i'||
       rc_read_resource_code_unit()!=L'n'||rc_read_resource_code_unit()!=L'e')
        goto malformed;
    rc_resource_line=0;
    do{
        if(rc_resource_character>=L'0'&&rc_resource_character<=L'9')
            rc_resource_line=rc_resource_line*10+rc_resource_character-L'0';
        rc_resource_character=(WCHAR)rc_read_resource_code_unit();
    }while(rc_resource_character!=L'"'&&rc_resource_character!=L'\n');
    if(rc_resource_character==L'\n')goto read_again;
    cursor=filename;
filename_read:
    rc_resource_character=(WCHAR)rc_read_resource_code_unit();
    lower=towlower(rc_resource_character);
    if(!lower||lower==L'\n')goto malformed;
    if(lower==L'"')goto filename_check;
    if(lower==L'\\'){
        if(escape){escape=0;goto filename_check;}
        escape=1;
        skip=0;dot=0;
    }else if(lower==L'.'){
        dot=1;escape=0;
    }else if(lower==L'c'||lower==L'h'){
        escape=0;skip=dot;dot=0;
    }else{skip=0;dot=0;}
#ifdef RECON_NATIVE_TEST
    if(cursor>=filename+MAX_PATH-1){ __asm int 3 }
#endif
    *cursor++=rc_resource_character;
filename_check:
    if(rc_resource_character!=L'"')goto filename_read;
    *cursor=0;
    WideCharToMultiByte(rc_codepage,0,filename,-1,rc_resource_filename,
                        MAX_PATH,NULL,NULL);
    do rc_resource_character=(WCHAR)rc_read_resource_code_unit();
    while(rc_resource_character!=L'\n');
    goto read_again;
malformed:
    rc_resource_input_fatal(2101);
    goto filename_check;
check_skip:
    if(skip)goto read_again;
finish:
    if(rc_resource_recording){
#ifdef RECON_NATIVE_TEST
        /* Resource-text capture storage has not been characterized. */
        __asm int 3
#endif
        *(WCHAR *)rc_reserve_resource_bytes(2)=rc_resource_character;
    }
    return rc_resource_character;
}

/* Actual initialized .data sentinel at RVA14d60; the loader-zeroed read
 * buffer occupies exactly 8194 bytes at RVA25f70. */
int rc_resource_buffer_index=4097;
WCHAR rc_resource_read_buffer[4097];

unsigned __cdecl rc_read_resource_code_unit(void)
{
    unsigned bytes,character;
#ifdef RECON_NATIVE_TEST
    if(rc_resource_buffer_index<0){ __asm int 3 }
#endif
    if(rc_resource_buffer_index>=4097){
        rc_resource_buffer_index=0;
        bytes=rc_read_bytes(rc_resource_input_file,rc_resource_read_buffer,8194);
#ifdef RECON_NATIVE_TEST
        if(bytes>sizeof(rc_resource_read_buffer)){ __asm int 3 }
#endif
        if(bytes<8194){
            fclose(rc_resource_input_file);
            rc_resource_read_buffer[bytes/2]=0;
        }
    }
    character=rc_resource_read_buffer[rc_resource_buffer_index];
    if(character)++rc_resource_buffer_index;
    return character;
}
