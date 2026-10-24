#ifndef C2_COFF_OUTPUT_H
#define C2_COFF_OUTPUT_H
#include "output-callbacks.h"
#include <recon.h>

typedef void *(__fastcall *C2OutputAllocator)(unsigned bytes);
typedef struct C2OutputTable C2OutputTable;
typedef struct C2OutputList C2OutputList;
typedef struct C2OutputStorage C2OutputStorage;
typedef struct C2OutputChunk C2OutputChunk;
typedef struct C2OutputBlock508 C2OutputBlock508;
typedef struct C2OutputBlock308 C2OutputBlock308;
typedef struct C2OutputSection C2OutputSection;
typedef struct C2CoffSource C2CoffSource;
typedef struct C2CoffRecord C2CoffRecord;
struct C2CoffSource {
    char *name;
    unsigned next_source;
};
struct C2CoffRecord {
    unsigned kind;
    unsigned pending04;
    union { C2CoffSource *source; } value;
};
typedef char C2CoffRecordSizeCheck[sizeof(C2CoffRecord) == 12 ? 1 : -1];
typedef char C2CoffRecordValueCheck[offsetof(C2CoffRecord, value) == 8 ? 1 : -1];
typedef char C2CoffSourceSizeCheck[sizeof(C2CoffSource) == 8 ? 1 : -1];

typedef struct C2OutputPage {
    struct C2OutputPage *next;
    void *items[1];
} C2OutputPage;
struct C2OutputTable {
    unsigned count;
    unsigned short capacity;
    unsigned used;
    unsigned iterator_index, iterator_used;
    C2OutputPage *iterator_page;
    C2OutputPage *first, *last;
};
typedef char C2OutputTableSizeCheck[sizeof(C2OutputTable) == 32 ? 1 : -1];
typedef char C2OutputTableCapacityCheck[offsetof(C2OutputTable, capacity) == 4 ? 1 : -1];
typedef char C2OutputTableUsedCheck[offsetof(C2OutputTable, used) == 8 ? 1 : -1];
typedef char C2OutputTableFirstCheck[offsetof(C2OutputTable, first) == 24 ? 1 : -1];
typedef char C2OutputPageSizeCheck[sizeof(C2OutputPage) == 8 ? 1 : -1];

/* The encountered chunk prefix only; complete allocation/layout stays pending. */
struct C2OutputChunk {
    C2OutputChunk *next;
};
struct C2OutputList {
    int buffered, count_only;
    unsigned completed_bytes, used_bytes, write_count;
    C2OutputChunk *last;
};
typedef char C2OutputListSizeCheck[sizeof(C2OutputList) == 24 ? 1 : -1];
typedef char C2OutputListCountOnlyCheck[offsetof(C2OutputList, count_only) == 4 ? 1 : -1];
typedef char C2OutputListLastCheck[offsetof(C2OutputList, last) == 20 ? 1 : -1];

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
extern C2OutputList *coff_output_list, *coff_current_list;
extern C2OutputChunk *coff_free_chunks;
extern C2OutputBlock508 *coff_free_blocks_508;
extern C2OutputBlock308 *coff_free_blocks_308;
extern unsigned long coff_pending_499690;
extern C2CoffRecord **coff_symbol_buffer;
extern unsigned coff_symbol_capacity, coff_symbol_total, coff_next_symbol;
extern unsigned short coff_default_flags, coff_flags, coff_pending_e18, coff_section_count;
extern C2OutputSection *coff_current_section;
extern int coff_pending_c90;
C2OutputTable *__fastcall new_output_table(unsigned short capacity);
void initialize_coff_lists(void);
void grow_coff_symbol_buffer(void);
C2OutputChunk *new_output_chunk(void);
C2OutputList *__fastcall new_output_list(int buffered, int count_only);
int read_incremental_output(void);
void restore_incremental_output(void);
void __fastcall add_output_source(char *name);
unsigned __fastcall add_coff_symbol(C2CoffRecord *record);
unsigned __fastcall add_incremental_symbol(C2CoffRecord *record);
unsigned __fastcall reserve_incremental_symbols(unsigned count, unsigned short section, int reuse);
extern unsigned coff_reserved_symbols;
extern C2CoffSource *coff_previous_source;
unsigned query_incremental_stream_size(void);
unsigned __fastcall read_incremental_stream(void *buffer, unsigned bytes);
unsigned char read_incremental_byte(void);
int validate_incremental_options(void);
void prepare_output_486e67(void);
void prepare_output_486a23(void);
void prepare_output_48710f(void);
void write_output_directives(void);
void __fastcall write_output_bytes(const void *data, unsigned bytes);
void __fastcall write_output_byte(unsigned char value);
unsigned short __fastcall create_coff_section(C2Section *section);
unsigned short __fastcall select_coff_section(unsigned short section);
unsigned __fastcall query_coff_position(unsigned short section);
void __fastcall write_output_fill(unsigned char value, unsigned count);
extern int function_option_23;
extern int c2_constant_strings, coff_directive_option;
unsigned short __fastcall add_coff_section(char *name, unsigned flags, int flag8, int reuse);
unsigned short __fastcall add_associative_coff_section(char *name, unsigned flags,
    unsigned char selection, unsigned short association, int flag8, int reuse);
unsigned char __fastcall coff_selection_code(unsigned char selection);
int __fastcall initialize_coff_section(unsigned short section, unsigned flags);
void __fastcall record_output_section(C2Section *section);
void __fastcall mark_debug_section(unsigned section);
#endif
