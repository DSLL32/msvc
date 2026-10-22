#ifndef PCH_H
#define PCH_H
#include "initialization.h"
#include "pch-format.h"

/* Encountered PCH event prefix; payload bytes immediately follow it. */
typedef struct PchRecord {
    unsigned kind, payload_bytes, value;
} PchRecord;
extern PchRecord *pch_record_cursor;
extern short pch_include_count;
extern unsigned short pch_include_page_count;
extern int pch_macro_changed, pch_rebuild_on_macro_change;
void pch_record_string(unsigned kind, unsigned value, char *text);
int __stdcall process_pch_string_event(unsigned kind, unsigned value, char *text);
int __stdcall pch_macro_defined(char *name);
int pch_replay_mismatch(void);
extern PchSettingsImage saved_pch_settings;
extern PchCompilerImage pch_compiler_image;
extern unsigned short pch_object_name_bytes, pch_browser_name_bytes;
extern unsigned pch_global_bytes, pch_line_bytes, pch_debug_bytes, pch_browser_bytes;
extern unsigned pch_event_checksum, pch_event_offset, pch_header_reserved;
extern WorkingStack *pch_event_stack;
extern int pch_file_incomplete, pch_replacement_incomplete;
extern FILE *pch_file, *pch_replacement_file;
extern int default_thread_storage;
short __stdcall vector_count(Vector *vector);
Vector *__stdcall copy_vector(Vector *source, int arena,
                              void *(__stdcall *copy_item)(void *, int));
unsigned __stdcall write_pch_event_stack(WorkingStack *stack, FILE *file, unsigned *checksum);
void declare_pch_debug_library(void);
void finish_pch_debug_symbols(void);
void begin_browser_pch_record(void);
extern int pch_replace_expression;
extern void *pch_copy_buffer;
unsigned __stdcall copy_pch_stream(FILE *input, FILE *output, unsigned bytes);
void append_pch_intermediates(void);
void stage_pch_intermediates(void);
void relocate_pch_state(void);
void write_pch_pages(void);
void restore_pch_pointers(void);
#endif
