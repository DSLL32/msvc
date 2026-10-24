#ifndef LINK_H
#define LINK_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <recon.h>
typedef struct Tool { char *name; unsigned short length, padding; int kind; int (*run)(int, char **); } Tool;
extern Tool tools[];
extern char *executable_path, *program_name, *tool_name;
extern int tool_kind, debug_display;
extern char debug_flags[];
extern unsigned char banner_pending;
extern char abort_file[];
extern char *abort_phase;
extern unsigned char debug_requested;
BOOL WINAPI control_handler(DWORD);
void link_cleanup(void);
extern char debug_flags[100];
void set_debug_flag(int,unsigned char);
void debug_initialize(char *, char *);
int debug_enabled(int);
void files_initialize(int,int,int,int,int,int,void (*)(char *,int,char *));
void file_failure(char *,int,char *);
void print_banner(void);
int linker_main(int,char **);
int library_main(int,char **);
int dumper_main(int,char **);
int editor_main(int,char **);
int helper_main(int,char **);
int driver_usage(void);
#endif
