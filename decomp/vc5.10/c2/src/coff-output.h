#ifndef C2_COFF_OUTPUT_H
#define C2_COFF_OUTPUT_H
#include "output-callbacks.h"
#include <recon.h>

typedef struct C2ObjectOutput {
    unsigned short format, machine;
    long pending04;
    char *source_name;
    FILE *file;
    C2PendingCallback output_allocator;
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
void writer_callback_41eb8a();
void writer_callback_48511c();
void writer_callback_45477a();
int __fastcall initialize_coff_output(C2ObjectOutput *writer);
unsigned query_incremental_stream_size(void);
unsigned __fastcall read_incremental_stream(void *buffer, unsigned bytes);
unsigned char read_incremental_byte(void);
int validate_incremental_options(void);
void prepare_output_486e67(void);
void prepare_output_486a23(void);
void prepare_output_48710f(void);
void write_output_directives(void);
#endif
