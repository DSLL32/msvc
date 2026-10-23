#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H
#include "global-ir.h"
#include <stddef.h>
#include <recon.h>
typedef struct DebugRecordFormat {
    char *fields;
    unsigned flags;
} DebugRecordFormat;
extern DebugRecordFormat debug_record_formats[13];
extern unsigned *debug_record_buffer;
extern unsigned debug_type_bytes;
/* Legacy CodeView type fixups retain twelve-byte records. The queue kind
   selects the payload; kind zero carries a complete CType descriptor. */
typedef struct DebugTypeRecord {
    unsigned char kind, reserved;
    unsigned short index;
    union { CType *type; FrontendSymbol *symbol; void *pointer; } value;
    unsigned auxiliary;
} DebugTypeRecord;
typedef char DebugTypeRecord_size[sizeof(DebugTypeRecord) == 12 ? 1 : -1];
extern DebugTypeRecord *debug_type_records;
extern unsigned debug_type_record_count;
void __stdcall write_legacy_pointer_type(CType *type, unsigned short nested);
void __stdcall write_legacy_array_type(CType *type, unsigned short nested);
void __stdcall write_legacy_function_type(CType *type, unsigned short result, unsigned short arguments);
void __stdcall write_legacy_qualified_type(CType *type);
void __stdcall write_legacy_type_record(int bytes);
extern unsigned short legacy_tag_auxiliary;
void __stdcall write_legacy_enum_type(FrontendSymbol *tag);
void __stdcall write_legacy_record_type(FrontendSymbol *tag, unsigned short fields, unsigned short auxiliary);
void __stdcall write_legacy_field_list(FrontendSymbol *tag, unsigned count, int mode);
void __stdcall write_legacy_argument_list(ParameterInfo *parameters);
void __stdcall write_legacy_argument_slice(CType **parameters, unsigned first, unsigned limit);
void reset_debug_type_records(void);
extern unsigned short next_debug_type_index;
unsigned short emit_pdb_type(CType *type);
unsigned short pdb_type_index(CType *type);
unsigned short pdb_shape_type_index(CType *type, TypeShape *shape);
unsigned short pdb_primitive_pointer_index(CType *type, TypeShape *shape);
CType *remove_pdb_type_shape(CType *type);
unsigned short pdb_pointer_type_index(CType *type, unsigned short nested);
void __stdcall materialize_based_symbol(FrontendSymbol *symbol);
unsigned short write_codeview_symbol(unsigned char *destination, FrontendSymbol *symbol);
unsigned short pdb_array_type_index(CType *type, unsigned short nested);
unsigned short write_codeview_number(unsigned char *destination, unsigned number);
unsigned short intern_pdb_record(unsigned short bytes);
void check_pdb_record_size(unsigned bytes);
unsigned short pdb_function_type_index(CType *type, unsigned short result);
unsigned short pdb_argument_list_index(ParameterInfo *parameters);
extern unsigned short debug_index_workspace[2000];
extern unsigned short *debug_index_cursor;
unsigned short *get_debug_index_cursor(void);
void set_debug_index_cursor(unsigned short *cursor);
void append_debug_index(unsigned short index);
unsigned codeview_calling_convention(unsigned short attributes);
unsigned pdb_argument_count(ParameterInfo *parameters);
unsigned short pdb_qualified_type_index(CType *type);
CType *remove_pdb_type_qualifiers(CType *type);
unsigned short pdb_base_type_index(CType *type);
unsigned short pdb_enum_type_index(FrontendSymbol *tag);
unsigned short pdb_record_type_index(FrontendSymbol *tag);
unsigned short pdb_complete_record_type_index(FrontendSymbol *tag);
unsigned short pdb_complete_enum_type_index(FrontendSymbol *tag);
unsigned short pdb_field_list_index(FrontendSymbol *tag, unsigned *count, unsigned short *properties);
unsigned short build_pdb_field_list(unsigned pass, FrontendSymbol *tag, unsigned *count, unsigned short *properties);
unsigned short write_pdb_member(unsigned pass, FrontendSymbol *member, unsigned char *destination);
unsigned short pdb_bitfield_type_index(FrontendSymbol *member);
unsigned short read_debug_index(void);
unsigned short write_codeview_signed_number(unsigned char *destination, int number);
unsigned short write_codeview_signed_leaf(unsigned char *destination, int number);
unsigned short pad_pdb_field_record(unsigned pass, unsigned char *destination, unsigned short bytes);
unsigned short pdb_primitive_type_index(unsigned short flags);
unsigned short __stdcall primitive_pointer_debug_index(CType *type);
unsigned short __stdcall primitive_debug_index(CType *type);
unsigned short __stdcall legacy_primitive_type_index(unsigned short flags);
unsigned __stdcall queue_debug_record(unsigned char kind);
void __stdcall emit_function_debug_type(CType *type);
unsigned char __stdcall prepare_legacy_argument_types(ParameterInfo *parameters);
void begin_program_database(void);
void finish_program_database(void);
void write_debug_record(unsigned record, ...);
/* The database starts with its 64-byte on-disk header. List nodes carry
   inline record storage following their twelve-byte headers. */
typedef struct PdbHeader {
    char signature[44];
    unsigned magic;
    unsigned timestamp;
    unsigned age;
    unsigned short first_type, last_type;
    unsigned record_bytes;
} PdbHeader;
typedef struct PdbBuffer {
    unsigned available;
    unsigned char *cursor;
    struct PdbBuffer *next;
} PdbBuffer;
typedef struct PdbBufferList {
    PdbBuffer *head, *tail;
    unsigned count;
} PdbBufferList;
typedef PdbBuffer PdbBlock;
typedef PdbBufferList PdbBlockList;
typedef struct PdbTypeEntry {
    struct PdbTypeEntry *next;
    unsigned char *record;
    unsigned short index;
} PdbTypeEntry;
typedef struct ProgramDatabase {
    PdbHeader header;
    unsigned char **type_offsets;
    PdbTypeEntry **type_index;
    PdbBufferList records;
    PdbBlockList blocks;
    PdbBuffer *loaded_tail;
    int handle, writable, incremental, cumulative;
} ProgramDatabase;
typedef char PdbHeader_size[sizeof(PdbHeader) == 64 ? 1 : -1];
typedef char PdbBuffer_size[sizeof(PdbBuffer) == 12 ? 1 : -1];
typedef char PdbBlock_size[sizeof(PdbBlock) == 12 ? 1 : -1];
typedef char ProgramDatabase_size[sizeof(ProgramDatabase) == 116 ? 1 : -1];
/* Offsets established by native record readers, writers and allocation sites. */
typedef char PdbHeader_fields[(offsetof(PdbHeader, timestamp) == 48 &&
    offsetof(PdbHeader, age) == 52 && offsetof(PdbHeader, first_type) == 56 &&
    offsetof(PdbHeader, last_type) == 58 && offsetof(PdbHeader, record_bytes) == 60) ? 1 : -1];
typedef char PdbBuffer_fields[(offsetof(PdbBuffer, cursor) == 4 && offsetof(PdbBuffer, next) == 8) ? 1 : -1];
typedef char PdbTypeEntry_layout[(sizeof(PdbTypeEntry) == 12 && offsetof(PdbTypeEntry, index) == 8) ? 1 : -1];
typedef char PdbBufferList_layout[(sizeof(PdbBufferList) == 12 && offsetof(PdbBufferList, count) == 8) ? 1 : -1];
typedef char ProgramDatabase_fields[(offsetof(ProgramDatabase, type_offsets) == 64 &&
    offsetof(ProgramDatabase, type_index) == 68 && offsetof(ProgramDatabase, records) == 72 &&
    offsetof(ProgramDatabase, blocks) == 84 && offsetof(ProgramDatabase, loaded_tail) == 96 &&
    offsetof(ProgramDatabase, handle) == 100 && offsetof(ProgramDatabase, writable) == 104 &&
    offsetof(ProgramDatabase, incremental) == 108 && offsetof(ProgramDatabase, cumulative) == 112) ? 1 : -1];
typedef char DebugTypeRecord_fields[(offsetof(DebugTypeRecord, index) == 2 &&
    offsetof(DebugTypeRecord, value) == 4 && offsetof(DebugTypeRecord, auxiliary) == 8) ? 1 : -1];
typedef char FrontendDebug_fields[(sizeof(CType) == 16 && offsetof(CType, field0c) == 12 &&
    offsetof(FrontendSymbol, metadata) == 36 && offsetof(ParameterInfo, auxiliary) == 4 &&
    offsetof(ParameterInfo, types) == 8) ? 1 : -1];
/* These methods use ECX for this and callee-cleaned stack arguments.
   The unused EDX formal expresses that ABI in period C: EDX is volatile
   and carries no input to the original methods. */
PdbBlock *__fastcall initialize_pdb_block(PdbBlock *self, unsigned unused_edx, unsigned capacity);
unsigned char *__fastcall reserve_pdb_block(PdbBlock *self, unsigned unused_edx, unsigned bytes);
int __fastcall load_program_database(ProgramDatabase *self);
int __fastcall create_program_database(ProgramDatabase *self, unsigned unused_edx, char *path, unsigned signature);
void __fastcall destroy_pdb_buffer_list(PdbBufferList *self);
extern ProgramDatabase *program_database;
extern unsigned char *pdb_record_buffer;
extern unsigned char *pdb_record_cursor;
unsigned short __fastcall insert_program_database_type(ProgramDatabase *self, unsigned unused_edx, unsigned char *record);
unsigned short hash_pdb_record(unsigned char *record);
unsigned char *__fastcall reserve_pdb_record_bytes(PdbBufferList *self, unsigned unused_edx, unsigned bytes);
unsigned short intern_program_database_type(ProgramDatabase *self, unsigned char *record);
int __fastcall write_program_database(ProgramDatabase *self);
int commit_program_database(ProgramDatabase *self);
int close_program_database(ProgramDatabase *self);
ProgramDatabase *open_program_database(char *path, char *mode, unsigned signature);
void initialize_pdb_record_buffer(void);
void write_program_database_reference(void);
unsigned program_database_signature(ProgramDatabase *self);
unsigned program_database_age(ProgramDatabase *self);
char *__stdcall append_codeview_name(char *destination, char *source);
unsigned short copy_codeview_name(char *destination, char *source);
unsigned short pad_codeview_record(unsigned char *destination, unsigned short bytes);
#endif
