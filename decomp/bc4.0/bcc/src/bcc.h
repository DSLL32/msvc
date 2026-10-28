#ifndef BCC_H
#define BCC_H
#include <windows.h>
#include <signal.h>
#include <dos.h>
#include <recon.h>

typedef ULONG (WINAPI *ExceptionHandler)(EXCEPTION_RECORD *, void *, CONTEXT *, void *);
extern ExceptionHandler _SetUserHandler(ExceptionHandler);
extern struct date build_date;
extern struct time build_time;
extern int error_count;
extern unsigned char option_82, option_a6, option_a7;
extern char *output_path;

void initialize_exception_handler(void);
ULONG WINAPI handle_exception(EXCEPTION_RECORD *, void *, CONTEXT *, void *);
void interrupt_handler(int);
char * __stdcall lookup_message(unsigned char);
void __stdcall print_text(const char *);
void print_usage(void);
void initialize_memory(void);
void __stdcall expand_response_files(char ***, int *);
void __stdcall initialize_options(char **, int);
void finish_options(void);
char * __stdcall add_extension(const char *, char *);
void reverse_lists(void);
void __stdcall report_no_input(int);
void __stdcall inspect_input(const char *);
void __stdcall inspect_v_option(int, const char *);
void finish_input_scan(void);
int __stdcall compile_inputs(char **, int);
void release_memory(void);
int __stdcall run_linker(const char *, const char *);
void __stdcall terminate_program(int);

#ifdef __linux__
#define BCC_TRAP() __asm__ volatile ("int3")
#else
#define BCC_TRAP() __emit__(0xcc)
#endif
#endif
