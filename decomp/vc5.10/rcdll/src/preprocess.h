#ifndef RECONSTRUCTED_RC_PREPROCESS_H
#define RECONSTRUCTED_RC_PREPROCESS_H
#include <windows.h>
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

extern int rc_cpp_remaining,rc_cpp_active,rc_cpp_E,rc_cpp_P;
extern int rc_cpp_nesting_level;
extern WCHAR **rc_cpp_arguments,*rc_cpp_bad_argument;
extern WCHAR *rc_cpp_option_argument;
extern WCHAR *rc_cpp_input,*rc_cpp_output;
extern RcOptionList rc_cpp_defines,rc_cpp_undefines,rc_cpp_includes;
extern RcOption rc_cpp_options[];

WCHAR **__stdcall rc_widen_arguments(int,char **);
int __cdecl rc_preprocessor_main(int,WCHAR **);
WCHAR *__cdecl rc_next_argument(void);
int __stdcall rc_scan_options(const RcOption *,WCHAR *,RcNextArgument,int);
void __stdcall rc_apply_option(const RcOption *,WCHAR *,int);
int __stdcall rc_match_option(const WCHAR *,const WCHAR *);
void __stdcall rc_open_preprocessor(WCHAR *,WCHAR *,RcOptionList *,RcOptionList *);
void __cdecl rc_run_preprocessor(void);
void __cdecl rc_close_preprocessor(void);
#endif
