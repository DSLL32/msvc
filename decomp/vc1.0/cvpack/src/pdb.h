#ifndef CVPACK_PDB_H
#define CVPACK_PDB_H
#include "types.h"
#include "image.h"
#include <recon.h>

typedef struct TypeServerBlock {
    unsigned long available;
    unsigned char *cursor;
    struct TypeServerBlock *next;
} TypeServerBlock;

typedef struct TypeServerArena {
    TypeServerBlock *first, *last;
    unsigned long count;
} TypeServerArena;

typedef struct TypeServer {
    char banner[44];
    unsigned long magic, signature, age;
    unsigned short first_type, last_type;
    unsigned long type_bytes;
    unsigned char **type_offsets;
    struct TypeServerRecord **type_hash;
    TypeServerArena type_storage, index_storage;
    TypeServerBlock *read_block;
    int descriptor, writing, incremental, loaded;
} TypeServer;
typedef char TypeServerSize[(sizeof(TypeServer)==116)?1:-1];
extern TypeServer *current_type_server;
TypeServer *type_server_open(const char *, const char *);
int type_server_close(TypeServer *);
unsigned long type_server_signature(TypeServer *);
unsigned long type_server_age(TypeServer *);
void load_type_server_records(const char *);
int __stdcall type_server_open_file(TypeServer *, const char *, const char *);
typedef struct TypeServerHeader {
    char banner[44];
    unsigned long magic, signature, age;
    unsigned short first_type, last_type;
    unsigned long type_bytes;
} TypeServerHeader;
typedef char TypeServerHeaderSize[(sizeof(TypeServerHeader)==64)?1:-1];
int __stdcall type_server_read_records(TypeServer *);
typedef struct TypeServerRecord {
    struct TypeServerRecord *next;
    unsigned char *record;
    unsigned short type;
} TypeServerRecord;
void *type_server_fill_alloc(unsigned long, unsigned char);
TypeServerRecord *__stdcall type_server_record_init(TypeServerRecord *, TypeServerRecord *, unsigned char *, unsigned short);
#endif
