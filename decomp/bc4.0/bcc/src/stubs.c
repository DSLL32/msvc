#include "bcc.h"
#include <recon.h>

void interrupt_handler(int sig) { (void)sig; BCC_TRAP(); }
void initialize_memory(void) { BCC_TRAP(); }
void __stdcall expand_response_files(char ***argv, int *argc) { (void)argv; (void)argc; BCC_TRAP(); }
void __stdcall initialize_options(char **argv, int argc) { (void)argv; (void)argc; BCC_TRAP(); }
void finish_options(void) { BCC_TRAP(); }
char * __stdcall add_extension(const char *extension, char *path) { (void)extension; (void)path; BCC_TRAP(); return 0; }
void reverse_lists(void) { BCC_TRAP(); }
void __stdcall report_no_input(int code) { (void)code; BCC_TRAP(); }
void __stdcall inspect_input(const char *path) { (void)path; BCC_TRAP(); }
void __stdcall inspect_v_option(int option, const char *text) { (void)option; (void)text; BCC_TRAP(); }
void finish_input_scan(void) { BCC_TRAP(); }
int __stdcall compile_inputs(char **argv, int argc) { (void)argv; (void)argc; BCC_TRAP(); return 0; }
void release_memory(void) { BCC_TRAP(); }
int __stdcall run_linker(const char *tail, const char *program) { (void)tail; (void)program; BCC_TRAP(); return 0; }
void __stdcall terminate_program(int status) { (void)status; BCC_TRAP(); }
