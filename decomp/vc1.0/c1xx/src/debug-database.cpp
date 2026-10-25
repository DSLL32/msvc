/* Embedded typesrvr.cpp database support. Public frontend boundaries retain
   C linkage; genuine ECX methods use the shared native thiscall annotation. */
extern "C" {
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include "debug-info.h"
#include "options.h"
#include <recon.h>
}

struct DatabaseBlock {
    unsigned remaining;
    unsigned char *cursor;
    DatabaseBlock *next;
    DatabaseBlock * RECON_THISCALL initialize(unsigned capacity);
    unsigned char * RECON_THISCALL allocate(unsigned bytes);
};
struct DatabasePool {
    DatabaseBlock *first, *current;
    unsigned used;
    void RECON_THISCALL release(void);
};
struct DatabaseHeader {
    char signature[44];
    unsigned magic, timestamp, age;
    unsigned short first_type, next_type;
    unsigned type_bytes;
};
struct DatabaseLookup {
    DatabaseLookup *next;
    unsigned char *record;
    unsigned short type;
    /* Original twelve-byte nodes leave this trailing WORD unwritten. */
    unsigned short reserved;
};
struct DebugDatabase {
    DatabaseHeader header;
    unsigned char **type_offsets;
    DatabaseLookup **type_lookup;
    DatabasePool records, lookup_nodes;
    DatabaseBlock *scan_block;
    int file, writable, want_lookup, want_offsets;
    int RECON_THISCALL create(char *filename, unsigned signature);
    int RECON_THISCALL load(void);
};
typedef char DatabaseHeaderMustBe64[(sizeof(DatabaseHeader) == 64) ? 1 : -1];
typedef char DatabaseBlockMustBe12[(sizeof(DatabaseBlock) == 12) ? 1 : -1];
typedef char DatabaseLookupMustBe12[(sizeof(DatabaseLookup) == 12) ? 1 : -1];
typedef char DatabasePoolMustBe12[(sizeof(DatabasePool) == 12) ? 1 : -1];
typedef char DatabaseMustBe116[(sizeof(DebugDatabase) == 116) ? 1 : -1];
typedef char DatabaseFileMustBe100[(offsetof(DebugDatabase, file) == 100) ? 1 : -1];
typedef char DatabaseOffsetsFlagMustBe112[(offsetof(DebugDatabase, want_offsets) == 112) ? 1 : -1];

/* Original0x17b70..0x17dc3, cdecl3. Allocate the type-server owners,
   select the requested mode, then load an existing or newly created database. */
DebugDatabase *__cdecl open_debug_database(char *filename, char *mode, unsigned signature)
{
    DebugDatabase *database = (DebugDatabase *)malloc(sizeof(DebugDatabase));
    DatabaseBlock *block, *next;
    int valid, closed;
    if (database) {
        database->type_offsets = 0;
        database->type_lookup = 0;
        block = (DatabaseBlock *)malloc(sizeof(DatabaseBlock));
        if (block) {
            block->remaining = 0;
            block->cursor = (unsigned char *)(block + 1);
            block->next = 0;
        }
        database->records.first = database->records.current = block;
        database->records.used = 0;
        block = (DatabaseBlock *)malloc(sizeof(DatabaseBlock));
        if (block) block = block->initialize(0);
        database->lookup_nodes.first = database->lookup_nodes.current = block;
        database->lookup_nodes.used = 0;
        database->file = -1;
    }
    if (!database) fatal_error(1, "typesrvr.cpp", 401);
    database->writable = mode[0] == 'w';
    switch (mode[1]) {
    case 'c': database->want_offsets = 1; break;
    case 'i':
        database->want_lookup = 1;
        /* Original defect: +112 is never written in this mode before load.
           Two genuine same-input catalog observations found zero. Use that
           documented value; this is not a promise about arbitrary malloc data. */
        database->want_offsets = 0;
        break;
    default: database->want_offsets = database->want_lookup = 1; break;
    }
    /* Mode c still leaves want_lookup unwritten in the original; that mode's
       actual value/behavior has not been qualified by the catalog workload. */
    database->file = _open(filename, _O_BINARY | (database->writable ? _O_RDWR : 0));
    if (database->file >= 0) {
        valid = _read(database->file, database, sizeof(DatabaseHeader)) == sizeof(DatabaseHeader) &&
            database->header.magic == 0xe0d5c && database->load();
        if (valid) {
            if (!database->writable) {
                if (database->file >= 0) {
                    closed = _close(database->file) >= 0;
                    database->file = -1;
                } else closed = 1;
                valid = closed;
            }
            if (valid) return database;
        }
    } else if (database->writable && database->create(filename, signature))
        return database;
    if (database) {
        if (database->file != -1 && database->file >= 0) {
            _close(database->file);
            database->file = -1;
        }
        if (database->type_offsets) free(database->type_offsets);
        if (database->type_lookup) free(database->type_lookup);
        block = database->lookup_nodes.first;
        while (block) {
            next = block->next;
            free(block);
            block = next;
        }
        database->records.release();
        free(database);
    }
    return 0;
}

/* Original0x18030..0x18048: ECX=this, one stack DWORD/RET4,
   returning the actual block owner in EAX. */
DatabaseBlock * RECON_THISCALL DatabaseBlock::initialize(unsigned capacity)
{
    remaining = capacity;
    cursor = (unsigned char *)(this + 1);
    next = 0;
    return this;
}
void RECON_THISCALL DatabasePool::release(void)
{
    __asm { int 3 }
}
/* Original0x17300..0x17385: ECX=this, filename/signature on the
   stack, RET8. The fixed64B file header is a typed template, not arena data. */
int RECON_THISCALL DebugDatabase::create(char *filename, unsigned signature)
{
    static const DatabaseHeader initial = {
        "Microsoft C/C++ program database 1.00\r\n\032JG", 0xe0d5c, 0, 0,
        0x1000, 0x1000, 0
    };
    file = _open(filename, _O_BINARY | _O_CREAT | _O_RDWR, 0777);
    if (file < 0) return 0;
    header = initial;
    header.timestamp = signature;
    if (_write(file, &header, sizeof(header)) != sizeof(header)) {
        if (file >= 0) {
            _close(file);
            file = -1;
        }
        return 0;
    }
    return load();
}
/* Original0x17390..0x1762c, ECX=this/RET0. Load type bytes into the
   records pool and optionally construct the offset and length-hash indexes. */
int RECON_THISCALL DebugDatabase::load(void)
{
    unsigned short limit = writable ? 0xfff0 : header.next_type;
    unsigned short type;
    unsigned bytes, capacity;
    unsigned char *record;
    DatabaseBlock *block;
    DatabaseLookup **bucket, *node;
    if (want_offsets) {
        type_offsets = (unsigned char **)malloc((limit - 0x1000) * 4);
        if (!type_offsets) return 0;
    }
    if (want_lookup) {
        type_lookup = (DatabaseLookup **)malloc(0x4000);
        if (type_lookup) {
            unsigned i;
            for (i = 0; i < 0x1000; ++i) type_lookup[i] = 0;
        }
        if (!type_lookup) return 0;
    }
    bytes = header.type_bytes;
    record = records.current->allocate(bytes);
    if (!record) {
        capacity = (bytes + 0x1fff) & ~0xfffU;
        block = (DatabaseBlock *)malloc(capacity + sizeof(DatabaseBlock));
        if (block) records.current->next = block->initialize(capacity);
        else records.current->next = 0;
        records.current = records.current->next;
        record = records.current->allocate(bytes);
        if (!record) fatal_error(1, "typesrvr.cpp", 58);
    }
    records.used += bytes;
    if (!record) return 0;
    if (_read(file, record, header.type_bytes) != header.type_bytes) return 0;
    for (type = header.first_type; type < header.next_type; ++type) {
        if (type_offsets) type_offsets[type - 0x1000] = record;
        if (type_lookup) {
            bucket = &type_lookup[*(unsigned short *)record];
            node = (DatabaseLookup *)lookup_nodes.current->allocate(sizeof(DatabaseLookup));
            if (!node) {
                capacity = (sizeof(DatabaseLookup) + 0x1fff) & ~0xfffU;
                block = (DatabaseBlock *)malloc(capacity + sizeof(DatabaseBlock));
                if (block) lookup_nodes.current->next = block->initialize(capacity);
                else lookup_nodes.current->next = 0;
                lookup_nodes.current = lookup_nodes.current->next;
                node = (DatabaseLookup *)lookup_nodes.current->allocate(sizeof(DatabaseLookup));
                if (!node) fatal_error(1, "typesrvr.cpp", 58);
            }
            lookup_nodes.used += sizeof(DatabaseLookup);
            if (node) {
                node->next = *bucket;
                node->record = record;
                node->type = type;
            } else node = 0;
            *bucket = node;
        }
        record += (unsigned short)(*(unsigned short *)(record + 2) + 2) + 2;
    }
    scan_block = records.current;
    records.current->remaining = 0;
    return 1;
}
unsigned char * RECON_THISCALL DatabaseBlock::allocate(unsigned bytes)
{
    __asm { int 3 }
    return 0;
}
