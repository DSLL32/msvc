#ifndef SOURCE_INPUT_H
#define SOURCE_INPUT_H
#include "global-ir.h"
#include <recon.h>
typedef struct SourceFile {
    unsigned capacity;
    int handle;
    unsigned saved_line;
    char *filename, *saved_cursor, *buffer;
    unsigned index;
    int available;
    long offset;
    int saved_packing;
    unsigned input_kind;
    int saved_conditional_depth;
    struct SourceFile *next;
} SourceFile;
extern SourceFile *active_input, *root_input, *free_inputs;
extern int source_handle;
extern char source_storage[3 * 4099];
extern char *source_text_cursor;
extern unsigned source_line_number;
extern int conditional_depth;
extern int macro_expansion_active;
#define macro_saved_input_cursor (macro_expansion_frames[1].source_cursor)
extern unsigned browser_file_cookie;
extern long browser_saved_position;
extern FILE *browser_file;
char *normalize_input_name(char *filename);
int release_input_handle(void);
int __stdcall process_pch_include(char *filename);
int __stdcall search_source_include(char *filename);
void __stdcall search_include_directories(char *filename);
void __stdcall record_pch_open_include(char *filename);
char *__stdcall copy_string(char *text, int arena);
int __stdcall open_input_handle(char *filename);
long __stdcall seek_work_stack(WorkingStack *stack, long offset, int origin);
extern int final_newline_emitted;
extern unsigned char forced_include_active;
void browser_include_end(void);
void report_ide_source_end(void);
int __stdcall read_work_stack(WorkingStack *stack, void *buffer, unsigned bytes);
int fill_source_buffer(void);
void emit_line_directive(void);
char *__stdcall quote_source_filename(char *destination, char *filename);
void report_ide_source(void);
void __stdcall record_line_information(unsigned flags);
void browser_include_begin(void);
void browser_open_file(void);
typedef char SourceFileSizeMustBe52[(sizeof(SourceFile) == 52) ? 1 : -1];
#endif
