#ifndef RCDLL_RESOURCE_INPUT_H
#define RCDLL_RESOURCE_INPUT_H
#include <stdio.h>
#include <stddef.h>
#include "rcdll.h"
#include <recon.h>
extern DWORD rc_resource_status,rc_resource_file_index,rc_resource_line;
extern FILE *rc_resource_input_file;
extern WCHAR rc_resource_character;
extern char rc_resource_filename[MAX_PATH];
unsigned __cdecl rc_next_resource_character(void);
unsigned __cdecl rc_read_resource_character(void);
unsigned __cdecl rc_read_resource_code_unit(void);
extern int rc_resource_buffer_index;
extern WCHAR rc_resource_read_buffer[4097];
typedef struct RcResourceToken {
    DWORD unknown0,line,file_index,unknown12;
    union {
        struct { WORD value; BYTE kind,unknown19; };
        DWORD packed_value;
    };
    WCHAR text[248],filename[260];
    WORD unknown1036,unknown1038;
} RcResourceToken;
typedef char RcResourceTokenLayout[
    sizeof(RcResourceToken)==1040 && offsetof(RcResourceToken,value)==16 &&
    offsetof(RcResourceToken,packed_value)==16 && offsetof(RcResourceToken,kind)==18 &&
    offsetof(RcResourceToken,text)==20 && offsetof(RcResourceToken,filename)==516 &&
    offsetof(RcResourceToken,unknown1036)==1036 ? 1 : -1];
extern RcResourceToken rc_resource_token;
void __stdcall rc_copy_resource_token(RcResourceToken *,const RcResourceToken *);
unsigned __stdcall rc_scan_resource_token(unsigned);
void __cdecl rc_read_resource_string(void);
void __cdecl rc_expand_resource_string(void);
extern int rc_resource_parentheses,rc_resource_long_number;
void __cdecl rc_read_resource_number(void);
DWORD __cdecl rc_parse_resource_expression(void);
DWORD __cdecl rc_parse_resource_term(void);
void __cdecl rc_parse_resource_integer(void);
extern WCHAR rc_resource_number_character,*rc_resource_number_cursor;
typedef struct RcResourcePunctuation {
    WCHAR character;
    WORD reserved2;
    BYTE kind,reserved5[3];
} RcResourcePunctuation;
typedef char RcResourcePunctuationLayout[
    sizeof(RcResourcePunctuation)==8&&offsetof(RcResourcePunctuation,kind)==4?1:-1];
extern RcResourcePunctuation rc_resource_punctuation[13];
WCHAR *__stdcall rc_collect_resource_word(WCHAR *);
WCHAR __cdecl rc_next_resource_digit(void);
DWORD __cdecl rc_parse_resource_hexadecimal(void);
DWORD __cdecl rc_parse_resource_octal(void);
DWORD __cdecl rc_parse_resource_decimal(void);
int __stdcall rc_read_resource_operator(WCHAR *);
void __cdecl rc_skip_resource_space(void);
void __cdecl rc_read_resource_integer(void);
int __stdcall rc_read_resource_symbol(unsigned);
WORD __stdcall rc_lookup_resource_keyword(const WCHAR *);
void __stdcall rc_resource_input_fatal_name(unsigned,const WCHAR *);
void __stdcall rc_resource_input_fatal(unsigned);
void __stdcall rc_read_apstudio_directive(int,DWORD);
void *__stdcall rc_reserve_resource_bytes(WORD);
#endif
