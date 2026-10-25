#include <wchar.h>
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

/* Encountered punctuation rows at RVA14cf8..14d60, stride8. The consumer
 * uses a WCHAR and BYTE kind; the remaining bytes are verified zero reserved
 * storage, not a widened kind or surrounding data. Shared by word/symbol readers. */
RcResourcePunctuation rc_resource_punctuation[13]={
    {L',',0,15,{0,0,0}},{L'|',0,12,{0,0,0}},
    {L'(',0,1,{0,0,0}},{L')',0,2,{0,0,0}},
    {L'{',0,13,{0,0,0}},{L'}',0,14,{0,0,0}},
    {L'~',0,16,{0,0,0}},{L'+',0,101,{0,0,0}},
    {L'-',0,102,{0,0,0}},{L'&',0,17,{0,0,0}},
    {L'=',0,18,{0,0,0}},{127,0,127,{0,0,0}},
    {0,0,0,{0,0,0}}
};

WCHAR *__stdcall rc_collect_resource_word(WCHAR *text)
{
    WCHAR *cursor=text;
    RcResourcePunctuation *punctuation;
#ifdef RECON_NATIVE_TEST
    /* Only the proved resource-name owner is available in this native case. */
    if(text!=rc_resource_name)__asm int 3
#endif
    rc_resource_number_character=rc_resource_character;
    for(;;){
#ifdef RECON_NATIVE_TEST
        if(cursor>=rc_resource_name+256)__asm int 3
#endif
        *cursor++=rc_resource_character;
        if((WORD)rc_next_resource_character()<=32)break;
        if(rc_resource_character==L'"'||rc_resource_character==L'\''||
           rc_resource_character==L';'||rc_resource_character==127)break;
        for(punctuation=rc_resource_punctuation;punctuation->character;++punctuation)
            if(punctuation->character==rc_resource_character)goto finish;
    }
finish:
    *cursor=0;
    return cursor;
}

WCHAR __cdecl rc_next_resource_digit(void)
{
#ifdef RECON_NATIVE_TEST
    WCHAR *end;
    unsigned long address=(unsigned long)rc_resource_number_cursor;
    /* The real collector establishes the terminated current name. Do not
     * infer readability from an arbitrary mapped pointer or stale tail. */
    if(address<(unsigned long)(rc_resource_name+1)||
       address>=(unsigned long)(rc_resource_name+257)||
       ((address-(unsigned long)rc_resource_name)&1))__asm int 3
    for(end=rc_resource_name;end<rc_resource_name+257&&*end;++end);
    if(end==rc_resource_name+257||rc_resource_number_cursor>end)__asm int 3
#endif
    rc_resource_number_character=*rc_resource_number_cursor++;
    return rc_resource_number_character;
}

DWORD __cdecl rc_parse_resource_hexadecimal(void)
{
    DWORD value=0;
    WORD digit;
    rc_next_resource_digit();
    while(iswctype(rc_resource_number_character,128)){
        rc_resource_number_character=(WCHAR)towupper(rc_resource_number_character);
        /* The original widened load is narrowed to AX after subtraction;
         * no neighboring high WORD contributes to the result. */
        digit=(WORD)(rc_resource_number_character>=L'A'?
            rc_resource_number_character-55:rc_resource_number_character-L'0');
        value=(value<<4)+digit;
        rc_next_resource_digit();
    }
    return value;
}

void __cdecl rc_skip_resource_space(void)
{
    while(rc_resource_character!=0xe001&&rc_resource_character!=0xe000&&
          rc_resource_character<=32)rc_next_resource_character();
}

int __stdcall rc_read_resource_operator(WCHAR *operation)
{
    const WCHAR *found;
    if(rc_resource_character==0xe001)rc_read_resource_marker(1,0xe001);
    found=wcschr(L"+-|&",rc_resource_character);
    *operation=rc_resource_character;
    if(found){
        rc_next_resource_character();
        rc_skip_resource_space();
        return 1;
    }
    return 0;
}

int __stdcall rc_read_resource_symbol(unsigned flags)
{
    RcResourcePunctuation *punctuation;
    for(punctuation=rc_resource_punctuation;punctuation->character;++punctuation){
        if(punctuation->character==rc_resource_character){
            rc_resource_token.value=0;
            rc_resource_token.kind=punctuation->kind;
            rc_next_resource_character();
            return rc_resource_token.kind>=11;
        }
    }
    rc_collect_resource_word(rc_resource_name);
    if(!rc_resource_name[1]&&(WCHAR)towupper(rc_resource_name[0])==L'L'&&
       rc_resource_character==L'"'){
        rc_resource_token.kind=137;
        return 1;
    }
    rc_resource_token.value=rc_lookup_resource_keyword(rc_resource_name);
    if(rc_resource_token.value!=0xffff)
        rc_resource_token.kind=(BYTE)rc_resource_token.value;
    else{
        if(flags){
            rc_resource_input_fatal_name(2104,rc_resource_name);
            return 0;
        }
        rc_resource_token.kind=0;
    }
    return 1;
}

/* The resource keyword table is pointer/WORD/reserved-WORD, stride eight.
 * Preserve the original order (including BEGIN before BEDIT) and spelling
 * MESAGETABLE: the search stops as soon as the input sorts below a row. */
typedef struct RcResourceKeyword {
    const WCHAR *text;
    WORD token,reserved;
} RcResourceKeyword;
typedef char RcResourceKeywordLayout[(sizeof(RcResourceKeyword)==8&&
    offsetof(RcResourceKeyword,token)==4&&
    offsetof(RcResourceKeyword,reserved)==6)?1:-1];
static const RcResourceKeyword rc_resource_keywords[] = {
    { L"ALT", 110, 0 },
    { L"ASCII", 111, 0 },
    { L"AUTO3STATE", 46, 0 },
    { L"AUTOCHECKBOX", 44, 0 },
    { L"AUTORADIOBUTTON", 48, 0 },
    { L"BEGIN", 13, 0 },
    { L"BEDIT", 57, 0 },
    { L"BITMAP", 67, 0 },
    { L"BLOCK", 114, 0 },
    { L"BUTTON", 95, 0 },
    { L"CAPTION", 135, 0 },
    { L"CHARACTERISTICS", 130, 0 },
    { L"CHECKBOX", 41, 0 },
    { L"CHECKED", 73, 0 },
    { L"CLASS", 100, 0 },
    { L"COMBOBOX", 91, 0 },
    { L"CONTROL", 134, 0 },
    { L"CTEXT", 63, 0 },
    { L"DEFPUSHBUTTON", 43, 0 },
    { L"DISCARDABLE", 0xfff2, 0 },
    { L"DLGINCLUDE", 136, 0 },
    { L"DLGINIT", 138, 0 },
    { L"EDIT", 64, 0 },
    { L"EDITTEXT", 60, 0 },
    { L"END", 14, 0 },
    { L"EXSTYLE", 247, 0 },
    { L"FILEFLAGS", 123, 0 },
    { L"FILEFLAGSMASK", 122, 0 },
    { L"FILEOS", 124, 0 },
    { L"FILESUBTYPE", 126, 0 },
    { L"FILETYPE", 125, 0 },
    { L"FILEVERSION", 120, 0 },
    { L"FIXED", 0xfff0, 0 },
    { L"FONT", 94, 0 },
    { L"GRAYED", 74, 0 },
    { L"GROUPBOX", 51, 0 },
    { L"HEDIT", 58, 0 },
    { L"HELP", 79, 0 },
    { L"ICON", 66, 0 },
    { L"IEDIT", 59, 0 },
    { L"IMPURE", 0xfff6, 0 },
    { L"INACTIVE", 75, 0 },
    { L"LANGUAGE", 131, 0 },
    { L"LISTBOX", 90, 0 },
    { L"LOADONCALL", 0xfff3, 0 },
    { L"LTEXT", 61, 0 },
    { L"MENU", 70, 0 },
    { L"MENUBARBREAK", 76, 0 },
    { L"MENUBREAK", 77, 0 },
    { L"MENUITEM", 71, 0 },
    { L"MESAGETABLE", 96, 0 },
    { L"MOVEABLE", 0xfff1, 0 },
    { L"NOINVERT", 103, 0 },
    { L"NONSHARED", 0xfff6, 0 },
    { L"NOT", 104, 0 },
    { L"OWNERDRAW", 50, 0 },
    { L"POPUP", 78, 0 },
    { L"PRELOAD", 0xfff4, 0 },
    { L"PRODUCTVERSION", 121, 0 },
    { L"PURE", 0xfff5, 0 },
    { L"PUSHBOX", 52, 0 },
    { L"PUSHBUTTON", 42, 0 },
    { L"RADIOBUTTON", 40, 0 },
    { L"RCDATA", 92, 0 },
    { L"RTEXT", 62, 0 },
    { L"SCROLLBAR", 93, 0 },
    { L"SEPARATOR", 72, 0 },
    { L"SHARED", 0xfff5, 0 },
    { L"SHIFT", 106, 0 },
    { L"STATE3", 45, 0 },
    { L"STATIC", 65, 0 },
    { L"STYLE", 133, 0 },
    { L"USERBUTTON", 47, 0 },
    { L"VALUE", 113, 0 },
    { L"VERSION", 132, 0 },
    { L"VIRTKEY", 112, 0 },
    { NULL, 0, 0 }
};

WORD __stdcall rc_lookup_resource_keyword(const WCHAR *text)
{
    const RcResourceKeyword *keyword=rc_resource_keywords;
    int order;
    while(keyword->text){
        order=_wcsicmp(text,keyword->text);
        if(!order)return keyword->token;
        if(order<0)break;
        ++keyword;
    }
    return 0xffff;
}
