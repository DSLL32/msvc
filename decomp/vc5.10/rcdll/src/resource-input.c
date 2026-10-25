#include <wctype.h>
#include "command.h"
#include "resource-input.h"
#include "resources.h"
#include <recon.h>

/* Resource status at 25d3c is distinct from the command result at 1853c.
 * File index, line and stream are at 27f78/27f7c/27f84. */
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

unsigned __stdcall rc_scan_resource_token(unsigned flags)
{
read_again:
    while(rc_resource_character!=0xe001&&rc_resource_character!=0xe000&&
          rc_resource_character<=32)rc_next_resource_character();
    if(rc_resource_character==0xe001)rc_read_resource_marker(flags,0xe001);
    rc_resource_token.text[0]=0;
    rc_resource_token.line=rc_resource_line;
    rc_resource_token.file_index=rc_resource_file_index;
    rc_resource_token.kind=127;
    switch(rc_resource_character){
    case L'"':
        rc_read_resource_string();
        break;
    case L'-': case L'~':
        if(flags&0x8000)rc_read_resource_integer();
        else rc_read_resource_number();
        break;
    case 127:
        break;
    default:
        if((rc_resource_character==L'('&&!(flags&0x8000))||
           iswctype(rc_resource_character,4)){
            if(flags&0x8000)rc_read_resource_integer();
            else rc_read_resource_number();
            if(rc_resource_character==0xe001)
                rc_read_resource_marker(flags,0xe001);
        }else{
            if(!rc_read_resource_symbol(flags))goto read_again;
            if(rc_resource_token.kind==137)rc_expand_resource_string();
        }
    }
    return rc_resource_token.kind;
}

/* Actual DWORDs at RVA27f88/27f90. This wrapper explicitly resets both before
 * their first expression consumer; no prior runtime/default equality claimed. */
int rc_resource_parentheses,rc_resource_long_number;

void __cdecl rc_read_resource_number(void)
{
    DWORD value;
    rc_resource_long_number=0;
    rc_resource_parentheses=0;
    value=rc_parse_resource_expression();
    if(rc_resource_parentheses)rc_resource_error(1013);
    if(rc_resource_long_number)rc_resource_token.unknown12=1;
    rc_resource_token.unknown0=value;
    rc_resource_token.value=(WORD)value;
    rc_resource_token.kind=20;
}

DWORD __cdecl rc_parse_resource_expression(void)
{
    DWORD value,right;
    WCHAR operation;
    int found;
    value=rc_parse_resource_term();
    if(rc_resource_character==0xe001){
        rc_read_resource_marker(1,0xe001);
        rc_resource_token.unknown1038=rc_resource_token.value;
    }
    for(;;){
        found=rc_read_resource_operator(&operation);
        if(operation==L')'){
            --rc_resource_parentheses;
            rc_next_resource_character();
            rc_skip_resource_space();
        }
        if(!found)break;
        rc_resource_token.text[0]=0;
        right=rc_parse_resource_term();
        switch(operation){
        case L'&': value&=right;break;
        case L'+': value+=right;break;
        case L'-': value-=right;break;
        case L'|': value|=right;break;
        }
    }
    rc_resource_token.unknown1038=(WORD)value;
    return value;
}

DWORD __cdecl rc_parse_resource_term(void)
{
    if(rc_resource_character==L'('){
        ++rc_resource_parentheses;
        rc_next_resource_character();
        rc_skip_resource_space();
        return rc_parse_resource_expression();
    }
    if(rc_resource_character!=L'-'&&rc_resource_character!=L'~'&&
       !iswctype(rc_resource_character,4)){
        rc_read_resource_symbol(0);
        rc_resource_error_name(2237,rc_resource_name);
        return 0;
    }
    rc_parse_resource_integer();
    if(rc_resource_token.unknown12)rc_resource_long_number=1;
    rc_skip_resource_space();
    return rc_resource_token.unknown0;
}

/* The collector writes the WORD at27f74; this parser initializes the separate
 * cursor at27f80 before digit readers consume it. No prior defaults claimed. */
WCHAR rc_resource_number_character,*rc_resource_number_cursor;

void __cdecl rc_parse_resource_integer(void)
{
    DWORD value;
    int negative=0,complement=0;
    if(rc_resource_character==L'~'){
        while(rc_resource_character!=0xe001&&rc_resource_character!=0xe000&&
              rc_resource_character<=32)rc_next_resource_character();
    }
    rc_collect_resource_word(rc_resource_name);
    rc_resource_number_cursor=rc_resource_name+1;
    rc_resource_token.kind=20;
    if(rc_resource_number_character==L'-'){
        negative=1;
        rc_next_resource_digit();
    }
    if(rc_resource_number_character==L'~'){
        complement=1;
        rc_next_resource_digit();
    }
    if(rc_resource_number_character==L'0'){
        rc_next_resource_digit();
        if(rc_resource_number_character==L'x')
            value=rc_parse_resource_hexadecimal();
        else if(rc_resource_number_character==L'o')
            value=rc_parse_resource_octal();
        else value=rc_parse_resource_decimal();
    }else value=rc_parse_resource_decimal();
    if(rc_resource_number_character==L'L'||rc_resource_number_character==L'l'){
        rc_resource_token.unknown12=1;
        rc_next_resource_digit();
    }else rc_resource_token.unknown12=0;
    if(negative)value=0-value;
    if((WORD)complement)value=~value;
    rc_resource_token.unknown0=value;
    rc_resource_token.value=(WORD)value;
}
