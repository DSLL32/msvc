#ifndef RC_RESOURCE_H
#define RC_RESOURCE_H
#include "lexer.h"
#include <recon.h>

struct ResourceInfo;
struct ResourceType {
    struct ResourceType *next;
    struct ResourceInfo *resources;
    WCHAR *name;
    WORD ordinal, reserved0e, reserved10, resource_count;
};
struct ResourceInfo {
    DWORD version, characteristics, extended_style, data_offset, data_size;
    struct ResourceInfo *next;
    DWORD reserved18;
    WCHAR *name;
    DWORD reserved20;
    WORD language, memory_flags, ordinal, reserved2a;
};
extern DWORD default_resource_version, default_characteristics;
extern unsigned int resource_count;
extern WORD font_count;
/* The parser stores signed WORD bounds at four-byte-spaced addresses. */
struct KanjiRange {
    short first;
    WORD reserved02;
    short last;
    WORD reserved06;
};
extern struct KanjiRange kanji_ranges[2];
extern BOOL font_directory_written;
extern struct ResourceInfo *string_table_resource;
extern void *dialog_scratch;

void __cdecl initialize_resource_types(void);
struct ResourceType * __stdcall find_resource_type(WCHAR *, WORD);
WCHAR * __stdcall duplicate_wide(WCHAR *);
BOOL __stdcall ansi_to_wide(WCHAR *, char *, unsigned int);
BOOL __stdcall wide_to_ansi(char *, WCHAR *, WORD);
void * __stdcall resource_free(void *);
void __cdecl allocate_resource_data(void);
void __cdecl release_resource_data(void);
void __cdecl begin_resource_data(void);
BOOL __cdecl parse_resource_identifier(void);
char * __cdecl read_identifier(void);
unsigned int __stdcall next_token(unsigned int);
WORD __cdecl advance_argument(void);
extern BOOL argument_had_comma;
WORD __cdecl parse_word_expression(void);
WORD __cdecl parse_language(void);
void __stdcall parser_error(char *, char *);
BOOL __stdcall parse_memory_flag(struct ResourceInfo *);
DWORD __stdcall parse_style(DWORD);
struct ResourceInfo * __stdcall parse_string_table(struct ResourceInfo *);
void __cdecl emit_font_directory(void);
void __stdcall emit_string_tables(struct ResourceInfo *);
void __stdcall parse_raw_data(struct ResourceInfo *);
unsigned int __cdecl read_resource_file(void);
unsigned int __stdcall read_icon_file_header(unsigned int);
unsigned int __cdecl read_bitmap_file_header(void);
void __stdcall put_word(WORD);
BOOL __stdcall parse_menu(int, struct ResourceInfo *);
BOOL __stdcall parse_dialog(struct ResourceInfo *);
void __stdcall write_resource(struct ResourceType *, struct ResourceInfo *);
void __stdcall record_resource(struct ResourceType *, struct ResourceInfo *, void *, long, long);
BOOL __stdcall parse_font(struct ResourceInfo *);
unsigned int __stdcall parse_accelerators(struct ResourceInfo *);
void __stdcall split_icon_file(struct ResourceType *, struct ResourceInfo *, int);
BOOL __cdecl parse_version_info(void);
void __stdcall parse_dialog_include(struct ResourceInfo *);
#endif
