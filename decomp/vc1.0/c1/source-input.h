#ifndef SOURCE_INPUT_H
#define SOURCE_INPUT_H
#include "global-ir.h"
typedef struct SourceFile {
    unsigned capacity;
    int handle;
    unsigned saved_line;
    char *filename, *saved_cursor, *buffer;
    unsigned index;
    int available;
    long offset;
    int saved_packing;
    unsigned input_kind, saved_pragma;
    struct SourceFile *next;
} SourceFile;
extern SourceFile *active_input, *root_input, *free_inputs;
extern int source_handle;
extern char source_storage[3 * 4099];
extern char *source_text_cursor;
extern unsigned source_line_number, include_pragma_state;
extern int macro_expansion_active;
extern char *macro_saved_input_cursor;
extern unsigned browser_file_cookie;
extern long browser_saved_position;
extern FILE *browser_file;
char *__stdcall copy_string(char *text, int arena);
char *normalize_input_name(char *filename);
void release_input_handle(void);
int __stdcall open_input_handle(char *filename);
long __stdcall seek_work_stack(WorkingStack *stack, long offset, int origin);
extern unsigned scanner_flags;
extern int final_newline_emitted;
int __stdcall read_work_stack(WorkingStack *stack, void *buffer, unsigned bytes);
int fill_source_buffer(void);
void emit_line_directive(void);
void report_ide_source(void);
void __stdcall record_line_information(unsigned flags);
void browser_include_begin(void);
void browser_open_file(void);
#endif
