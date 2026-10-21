#ifndef RCPP_RECONSTRUCTION_H
#define RCPP_RECONSTRUCTION_H
#include <windows.h>
#define rc_malloc malloc
#define rc_environment _aenvptr
#define rc_envp _environ
#define rc_runtime_abort _amsg_exit
#define rc_setenvp _setenvp
extern char *rc_command_line, *rc_environment;
extern unsigned rc_os_build, rc_os_version, rc_os_major, rc_os_minor;
extern unsigned rc_base_build, rc_base_major, rc_base_minor;
extern unsigned rc_base_version, rc_major_version, rc_minor_version;
extern int rc_argc;
extern char **rc_argv, **rc_envp;
extern HANDLE rc_process_heap;
void rc_heap_initialize(void);
extern unsigned char _osfile[64];
#define rc_handle_flags _osfile
extern HANDLE _osfhnd[64];
#define rc_handles _osfhnd
void _ioinit(void);
extern char *rc_program_name;
extern int (__cdecl *rc_new_handler)(unsigned);
void *rc_malloc(unsigned);
void rc_runtime_abort(int);
void rc_parse_command(char *, char **, char *, int *, int *);
void rc_setargv(void);
void rc_setenvp(void);
typedef void (__cdecl *RcInitializer)(void);
extern RcInitializer rc_fp_initialize;
void rc_initterm(RcInitializer *, RcInitializer *);
void rc_initialize_c(void);
int rc_main(int, char **, char **);
void rc_exit(int);
void rc_cexit(int);
int rc_exception_filter(unsigned, EXCEPTION_POINTERS *);
void rcpp_start(void);
#endif
