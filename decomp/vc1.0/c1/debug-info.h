#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H
#include "global-ir.h"
typedef struct DebugRecordFormat {
    char *fields;
    unsigned flags;
} DebugRecordFormat;
extern DebugRecordFormat debug_record_formats[13];
extern unsigned *debug_record_buffer;
extern unsigned debug_type_bytes;
void begin_program_database(void);
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
ProgramDatabase *open_program_database(char *path, char *mode, unsigned signature);
void initialize_pdb_record_buffer(void);
void write_program_database_reference(void);
unsigned program_database_signature(ProgramDatabase *self);
unsigned program_database_age(ProgramDatabase *self);
char *__stdcall append_codeview_name(char *destination, char *source);
unsigned short copy_codeview_name(char *destination, char *source);
unsigned short pad_codeview_record(unsigned char *destination, unsigned short bytes);
#endif
