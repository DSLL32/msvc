#ifndef C2_COFF_OUTPUT_H
#define C2_COFF_OUTPUT_H
#include "output-callbacks.h"
#include <recon.h>

typedef void *(__fastcall *C2OutputAllocator)(unsigned bytes);
typedef void (__fastcall *C2OutputErrorHandler)(int error, char *source, int line);
/* Actual writer45477a supplies the header's DWORD timestamp in EAX, RET0. */
typedef struct C2OutputClockCallback { unsigned long (__cdecl *target)(void); } C2OutputClockCallback;
typedef char C2OutputClockSizeCheck[sizeof(C2OutputClockCallback) == 4 ? 1 : -1];
typedef char C2OutputClockOffsetCheck[offsetof(C2OutputClockCallback, target) == 0 ? 1 : -1];
typedef char C2CoffFileHeaderSizeCheck[sizeof(IMAGE_FILE_HEADER) == 20 ? 1 : -1];
typedef char C2CoffFileHeaderClockCheck[offsetof(IMAGE_FILE_HEADER, TimeDateStamp) == 4 ? 1 : -1];
typedef struct C2OutputTable C2OutputTable;
typedef struct C2OutputList C2OutputList;
typedef struct C2OutputStorage C2OutputStorage;
typedef struct C2OutputChunk C2OutputChunk;
typedef struct C2OutputSectionLink {
    struct C2OutputSectionLink *next;
    C2Section *section;
} C2OutputSectionLink;
extern C2OutputSectionLink *coff_section_buckets[16];
typedef char C2OutputSectionLinkSizeCheck[sizeof(C2OutputSectionLink) == 8 ? 1 : -1];
typedef char C2OutputSectionBucketsSizeCheck[sizeof(coff_section_buckets) == 64 ? 1 : -1];
typedef struct C2OutputStorageEntry {
    struct C2OutputStorageEntry *next;
    struct C2CoffSectionData *data;
} C2OutputStorageEntry;
struct C2OutputStorage {
    C2OutputStorageEntry *first, *last;
    unsigned count, next_index;
};
typedef char C2OutputStorageSizeCheck[sizeof(C2OutputStorage) == 16 ? 1 : -1];
typedef char C2OutputStorageEntrySizeCheck[sizeof(C2OutputStorageEntry) == 8 ? 1 : -1];
typedef char C2OutputStorageIndexCheck[offsetof(C2OutputStorage, next_index) == 12 ? 1 : -1];
typedef struct C2OutputBlock508 C2OutputBlock508;
typedef struct C2OutputBlock308 C2OutputBlock308;
typedef struct C2OutputSection C2OutputSection;
typedef struct C2CoffSectionData C2CoffSectionData;
struct C2OutputSection {
    C2CoffSectionData *data;
    char *name;
    unsigned characteristics, base_offset;
    unsigned short index, flags;
    unsigned char selection, pending21;
    unsigned short association;
};
/* Encountered prefixes only. The allocator requests60/508/308 bytes;
   unvisited tails and the byte28..47 gap have no field/content credit. */
struct C2OutputBlock508 {
    unsigned count;
    C2OutputBlock508 *last; /* Circular successor; the historical field name is retained. */
    IMAGE_RELOCATION entries[50];
};
typedef char C2CoffRelocationSizeCheck[sizeof(IMAGE_RELOCATION) == 10 ? 1 : -1];
typedef char C2CoffRelocationsOffsetCheck[offsetof(C2OutputBlock508, entries) == 8 ? 1 : -1];
typedef char C2CoffRelocationsBlockSizeCheck[sizeof(C2OutputBlock508) == 508 ? 1 : -1];
struct C2OutputBlock308 { unsigned count; C2OutputBlock308 *last; };
struct C2CoffSectionData {
    C2CoffSectionData *next;
    C2OutputSection *section;
    unsigned symbol_index;
    C2OutputList *output;
    C2OutputBlock508 *relocations;
    C2OutputBlock308 *line_numbers;
    unsigned size;
    unsigned virtual_address, virtual_size;
    unsigned raw_data_offset, relocation_offset, line_number_offset;
    unsigned short storage_index, flags;
    unsigned relocation_count;
    unsigned short line_number_count;
};
typedef char C2CoffSectionDataOutputCheck[offsetof(C2CoffSectionData, output) == 12 ? 1 : -1];
typedef char C2CoffSectionDataSizeCheck[offsetof(C2CoffSectionData, size) == 24 ? 1 : -1];
typedef char C2CoffSectionDataIndexCheck[offsetof(C2CoffSectionData, storage_index) == 48 ? 1 : -1];
typedef char C2CoffRelocationCountCheck[offsetof(C2CoffSectionData, relocation_count) == 52 ? 1 : -1];
typedef char C2CoffSectionRawOffsetCheck[offsetof(C2CoffSectionData, raw_data_offset) == 36 ? 1 : -1];
typedef char C2CoffSectionLineCountCheck[offsetof(C2CoffSectionData, line_number_count) == 56 ? 1 : -1];
typedef char C2CoffSectionDataFlagsCheck[offsetof(C2CoffSectionData, flags) == 50 ? 1 : -1];
typedef char C2OutputSectionSizeCheck[sizeof(C2OutputSection) == 24 ? 1 : -1];
typedef char C2OutputSectionNameCheck[offsetof(C2OutputSection, name) == 4 ? 1 : -1];
typedef char C2OutputSectionCharacteristicsCheck[offsetof(C2OutputSection, characteristics) == 8 ? 1 : -1];
typedef char C2OutputSectionIndexCheck[offsetof(C2OutputSection, index) == 16 ? 1 : -1];
typedef char C2OutputSectionFlagsCheck[offsetof(C2OutputSection, flags) == 18 ? 1 : -1];
typedef char C2OutputSectionSelectionCheck[offsetof(C2OutputSection, selection) == 20 ? 1 : -1];
typedef char C2OutputSectionAssociationCheck[offsetof(C2OutputSection, association) == 22 ? 1 : -1];
typedef struct C2CoffSource C2CoffSource;
typedef struct C2CoffRecord C2CoffRecord;
struct C2CoffSource {
    char *name;
    unsigned next_source;
};
struct C2CoffRecord {
    unsigned kind;
    union { unsigned pending04, serial; };
    union { C2CoffSource *source; C2CoffSectionData *section; IMAGE_SYMBOL *symbol; } value;
};
typedef char C2ImageAuxiliarySizeCheck[sizeof(IMAGE_AUX_SYMBOL) == 18 ? 1 : -1];
typedef char C2ImageSymbolSizeCheck[sizeof(IMAGE_SYMBOL) == 18 ? 1 : -1];
typedef char C2ImageSymbolValueCheck[offsetof(IMAGE_SYMBOL, Value) == 8 ? 1 : -1];
typedef char C2ImageSymbolTypeCheck[offsetof(IMAGE_SYMBOL, Type) == 14 ? 1 : -1];
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

/* The writer encounters500 payload bytes; the final four allocation bytes
   are still uninterpreted and are not part of this prefix. */
struct C2OutputChunk {
    C2OutputChunk *next;
    unsigned char data[500];
};
typedef char C2OutputChunkPayloadCheck[offsetof(C2OutputChunk, data) == 4 ? 1 : -1];
typedef char C2OutputChunkPrefixCheck[sizeof(C2OutputChunk) == 504 ? 1 : -1];
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
    C2OutputErrorHandler output_error;
    C2OutputClockCallback output_clock;
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
extern unsigned incremental_buffer_left, incremental_stream_append;
extern unsigned incremental_extended_counts;
extern int incremental_header_output;
extern unsigned char *incremental_buffer_cursor;
extern C2Section *directive_section, *fpo_section;
void *__fastcall allocate_output_bytes(unsigned bytes);
void __fastcall writer_callback_48511c(int error, char *source, int line);
unsigned long writer_callback_45477a(void);
int __fastcall initialize_coff_output(C2ObjectOutput *writer);
extern C2OutputAllocator output_allocate;
extern C2OutputErrorHandler output_error_handler;
extern C2OutputClockCallback output_clock_handler;
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
void __fastcall write_coff_symbol_relocation(unsigned offset, C2Symbol *symbol,
    unsigned short type);
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
unsigned __fastcall coff_alignment_characteristics(unsigned alignment);
void __fastcall or_coff_section_characteristics(unsigned short section, unsigned flags);
void finish_coff_sections(void);
void *__fastcall next_output_table_entry(C2OutputTable *table);
void finish_coff_image(void);
void prepare_incremental_sections(void);
void prepare_incremental_symbols(void);
void __fastcall reserve_coff_headers(unsigned short count);
unsigned __fastcall output_table_count(C2OutputTable *table);
void __fastcall grow_incremental_headers(unsigned bytes);
void __fastcall write_coff_section_data(C2OutputSection *section);
unsigned __fastcall coff_relocation_bytes(C2CoffSectionData *data);
unsigned __fastcall coff_line_number_bytes(C2CoffSectionData *data);
unsigned __fastcall reserve_incremental_section(unsigned bytes, unsigned previous_offset);
void __fastcall write_coff_section_relocations(C2CoffSectionData *data, unsigned base);
void __fastcall write_coff_section_lines(C2CoffSectionData *data, unsigned base);
void __fastcall write_incremental_ulong(unsigned value);
void write_incremental_header_metadata(void);
void __fastcall write_coff_symbol_record(C2CoffRecord *record);
void __fastcall write_coff_section_header(C2CoffSectionData *data);
void __fastcall encode_coff_section_name(unsigned char *destination, char *name);
void __fastcall flush_coff_list(C2OutputList *list);
void __fastcall write_incremental_byte(unsigned char value);
void write_incremental_options(void);
void write_incremental_objects(void);
void write_incremental_sections(void);
void write_incremental_global_symbols(void);
void write_incremental_symbol_uses(void);
int __fastcall replace_incremental_stream(const void *data, unsigned bytes);
int __fastcall append_incremental_stream(const void *data, unsigned bytes);
void __fastcall write_output_bytes(const void *data, unsigned bytes);
void __fastcall write_output_byte(unsigned char value);
unsigned short __fastcall create_coff_section(C2Section *section);
unsigned short __fastcall select_coff_section(unsigned short section);
unsigned __fastcall query_coff_position(unsigned short section);
unsigned __fastcall output_list_position(C2OutputList *list);
void __fastcall write_output_fill(unsigned char value, unsigned count);
void __cdecl write_output_word(unsigned short value);
void __cdecl write_output_long(unsigned value);
extern int function_option_23;
extern int c2_constant_strings, coff_directive_option;
unsigned short __fastcall add_coff_section(char *name, unsigned flags, int flag8, int reuse);
unsigned short __fastcall add_associative_coff_section(char *name, unsigned flags,
    unsigned char selection, unsigned short association, int flag8, int reuse);
unsigned char __fastcall coff_selection_code(unsigned char selection);
int __fastcall initialize_coff_section(unsigned short section, unsigned flags);
void *__fastcall lookup_output_table(C2OutputTable *table, unsigned index);
void __fastcall initialize_coff_symbols(C2CoffSectionData *data);
unsigned __fastcall register_coff_storage(C2CoffSectionData *data);
extern int deferred_compilation;
void __fastcall record_output_section(C2Section *section);
void __fastcall mark_debug_section(unsigned section);
extern int coff_reuse_index;
extern C2OutputSection *coff_reuse_sections[8];
unsigned __fastcall append_output_table(C2OutputTable *table, void *item);
#endif
