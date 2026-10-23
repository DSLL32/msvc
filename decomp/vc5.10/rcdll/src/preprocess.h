#ifndef RECONSTRUCTED_RC_PREPROCESS_H
#define RECONSTRUCTED_RC_PREPROCESS_H
#include <windows.h>
#include <stdio.h>
#include <recon.h>

typedef WCHAR *(__cdecl *RcNextArgument)(void);
typedef struct RcOptionList {
    int remaining;
    WCHAR *items[100];
} RcOptionList;
typedef struct RcOption {
    const WCHAR *pattern;
    void *destination;
    signed char keep_scanning;
    unsigned char action;
    unsigned short reserved;
} RcOption;
typedef struct RcLetterOption {
    int letter,action;
    int *destination;
} RcLetterOption;

typedef struct RcMacro {
    struct RcMacro *next;
    WCHAR *name,*text;
    signed char parameters;
    unsigned char flags;
    unsigned short reserved;
} RcMacro;
typedef struct RcSourceFrame {
    unsigned capacity;
    FILE *file;
    unsigned saved_line;
    WCHAR *filename,*saved_cursor,*buffer;
    int chars_read,encoding,seek_result;
} RcSourceFrame;
extern int rc_source_depth,rc_source_encoding;
extern WCHAR *rc_source_cursor;
extern FILE *rc_source_file;
extern RcSourceFrame rc_source_frames[1024];

extern RcMacro *rc_macro_buckets[256];
extern int rc_macro_listing_enabled;

extern int rc_cpp_remaining,rc_cpp_active,rc_cpp_E,rc_cpp_P;
extern int rc_cpp_nesting_level;
extern int rc_cpp_extensions,rc_cpp_ZE;
extern WCHAR **rc_cpp_arguments,*rc_cpp_bad_argument;
extern WCHAR *rc_cpp_option_argument;
extern WCHAR *rc_cpp_input,*rc_cpp_output;
extern RcOptionList rc_cpp_defines,rc_cpp_undefines,rc_cpp_includes;
extern RcOption rc_cpp_options[];
extern WCHAR rc_macro_name[512];
extern unsigned rc_macro_name_length;
extern unsigned char rc_macro_hash;
extern char rc_cpp_path_buffer[257];
extern FILE *rc_cpp_output_file;

WCHAR **__stdcall rc_widen_arguments(int,char **);
int __cdecl rc_preprocessor_main(int,WCHAR **);
WCHAR *__cdecl rc_next_argument(void);
int __stdcall rc_scan_options(const RcOption *,WCHAR *,RcNextArgument,int);
void __stdcall rc_apply_option(const RcOption *,WCHAR *,int);
int __stdcall rc_match_option(const WCHAR *,const WCHAR *);
int __stdcall rc_option_number(WCHAR *);
extern unsigned short rc_character_tokens[256];
void __stdcall rc_set_character_token(unsigned short,unsigned short);
extern unsigned short rc_character_classes[257];
void __stdcall rc_set_character_class(unsigned short,unsigned short);
unsigned char __stdcall rc_hash_macro_name(const WCHAR *);
RcMacro *__cdecl rc_find_macro(void);
int __stdcall rc_macro_text_differs(const WCHAR *,const WCHAR *,unsigned);
WCHAR *__stdcall rc_copy_macro_text(const WCHAR *,unsigned);
void __stdcall rc_list_macro(RcMacro *);
void __stdcall rc_define_macro(const WCHAR *,unsigned,int);
void __cdecl rc_undefine_macro(void);
int __stdcall rc_open_source(WCHAR *,int);
void __stdcall rc_open_preprocessor(WCHAR *,WCHAR *,RcOptionList *,RcOptionList *);
int __cdecl rc_refill_source(void);
WCHAR *__stdcall rc_duplicate_source_name(const WCHAR *);
void __cdecl rc_emit_source_line(void);
WCHAR *__stdcall rc_quote_source_filename(WCHAR *,const WCHAR *);
void __stdcall rc_write_preprocessed(const void *,unsigned,unsigned,FILE *);
void __cdecl rc_scan_line_start(void);
void __cdecl rc_run_preprocessor(void);
void __cdecl rc_close_preprocessor(void);
#endif
