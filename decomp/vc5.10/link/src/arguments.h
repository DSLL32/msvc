#ifndef LINK_ARGUMENTS_H
#define LINK_ARGUMENTS_H
#include <windows.h>
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct SwitchValue {
    char *name, *text;
} SwitchValue;
typedef struct SwitchInfo {
    char *name;
    WORD value_count, reserved;
    SwitchValue values[1]; /* Values and their mutable text follow this prefix. */
} SwitchInfo;
typedef struct InputArgument {
    char *name;
    char *original_name;
    SwitchInfo *switch_info;
    DWORD input_tag, reserved10; /* Opaque tag carried into a module/library. */
    struct InputArgument *next;
} InputArgument;
typedef struct InputArguments {
    InputArgument *first, *last;
    unsigned count;
} InputArguments;
extern BYTE input_prepass;
extern InputArguments input_files, library_files, ordinary_files, link_files, option_arguments;
extern InputArgument *argument_tail;
char *__stdcall duplicate_argument(const char *);
void __stdcall release_memory(void *);
InputArgument *__stdcall append_argument(InputArguments *,const char *);
void __stdcall parse_command_line(int,char **,const char *,const char *);
void __stdcall collect_file_arguments(int,char **,const char *,const char *);
SwitchInfo *__stdcall parse_switch(const char *,const char *);
#ifdef __cplusplus
}
#endif
#endif
