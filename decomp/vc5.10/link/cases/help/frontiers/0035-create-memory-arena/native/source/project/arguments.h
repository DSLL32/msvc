#ifndef LINK_ARGUMENTS_H
#define LINK_ARGUMENTS_H
#include <windows.h>
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct InputModule {
    DWORD reserved0;
    WORD format;
} InputModule;
typedef struct InputArgument {
    char *name;
    char *original_name;
    InputModule *module;
    DWORD reserved0c, reserved10;
    struct InputArgument *next;
} InputArgument;
typedef struct InputArguments {
    InputArgument *first, *last;
    unsigned count;
} InputArguments;
extern BYTE input_prepass;
extern InputArguments input_files;
void __stdcall parse_command_line(int,char **,const char *,const char *);
void __stdcall collect_file_arguments(int,char **,const char *,const char *);
InputModule *__stdcall load_input_module(void *,const char *);
#ifdef __cplusplus
}
#endif
#endif
