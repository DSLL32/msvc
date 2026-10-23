#ifndef C2_COFF_OUTPUT_H
#define C2_COFF_OUTPUT_H
#include "output-callbacks.h"
#include <recon.h>

typedef void *(__fastcall *C2OutputAllocator)(unsigned bytes);
typedef struct C2OutputTable C2OutputTable;
typedef struct C2OutputList C2OutputList;
typedef struct C2OutputStorage C2OutputStorage;

typedef struct C2ObjectOutput {
    unsigned short format, machine;
    long pending04;
    char *source_name;
    FILE *file;
    C2OutputAllocator output_allocator;
    C2PendingCallback output_error;
    C2PendingCallback output_clock;
    unsigned char debug;
} C2ObjectOutput;
typedef char C2ObjectOutputSizeCheck[sizeof(C2ObjectOutput) == 32 ? 1 : -1];
typedef char C2ObjectOutputSourceCheck[offsetof(C2ObjectOutput, source_name) == 8 ? 1 : -1];
typedef char C2ObjectOutputFileCheck[offsetof(C2ObjectOutput, file) == 12 ? 1 : -1];
typedef char C2ObjectOutputAllocatorCheck[offsetof(C2ObjectOutput, output_allocator) == 16 ? 1 : -1];
typedef char C2ObjectOutputErrorCheck[offsetof(C2ObjectOutput, output_error) == 20 ? 1 : -1];
typedef char C2ObjectOutputClockCheck[offsetof(C2ObjectOutput, output_clock) == 24 ? 1 : -1];
typedef char C2ObjectOutputDebugCheck[offsetof(C2ObjectOutput, debug) == 28 ? 1 : -1];
extern FILE *object_output;
extern int object_output_open;
extern void *incremental_data;
extern C2Section *directive_section, *fpo_section;
void *__fastcall allocate_output_bytes(unsigned bytes);
void writer_callback_48511c();
void writer_callback_45477a();
int __fastcall initialize_coff_output(C2ObjectOutput *writer);
extern C2OutputAllocator output_allocate;
extern C2PendingCallback output_error_handler, output_clock_handler;
extern int coff_new_output;
extern unsigned short coff_machine;
extern char *coff_source_name;
extern FILE *coff_output;
extern long coff_pending04;
extern unsigned char coff_debug;
extern C2OutputTable *coff_table;
extern C2OutputStorage *coff_storage;
extern void *coff_symbol_buffer;
extern unsigned coff_symbol_capacity;
extern unsigned short coff_default_flags, coff_flags, coff_pending_e18, coff_section_count;
extern unsigned coff_symbol_count;
extern int coff_pending_c90;
C2OutputTable *__fastcall new_output_table(unsigned short capacity);
void initialize_coff_lists(void);
C2OutputList *__fastcall new_output_list(int kind, void *context);
int read_incremental_output(void);
void restore_incremental_output(void);
void __fastcall add_output_source(char *name);
unsigned query_incremental_stream_size(void);
unsigned __fastcall read_incremental_stream(void *buffer, unsigned bytes);
unsigned char read_incremental_byte(void);
int validate_incremental_options(void);
void prepare_output_486e67(void);
void prepare_output_486a23(void);
void prepare_output_48710f(void);
void write_output_directives(void);
#endif
