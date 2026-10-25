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
struct DebugDatabase {
    DatabaseHeader header;
    unsigned *type_offsets;
    void *type_lookup;
    DatabasePool records, lookup_nodes;
    DatabaseBlock *scan_block;
    int file, writable, want_lookup, want_offsets;
    int RECON_THISCALL create(char *filename, unsigned signature);
    int RECON_THISCALL load(void);
};
typedef char DatabaseHeaderMustBe64[(sizeof(DatabaseHeader) == 64) ? 1 : -1];
typedef char DatabaseBlockMustBe12[(sizeof(DatabaseBlock) == 12) ? 1 : -1];
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
    case 'i': database->want_lookup = 1; break;
    default: database->want_offsets = database->want_lookup = 1; break;
    }
    /* The original does not initialize the other flag for c/i modes. Its
       actual lifetime and later consumers remain a separate reached-state
       question; do not clear the object or assume malloc supplies zeroes. */
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
int RECON_THISCALL DebugDatabase::load(void)
{
    __asm { int 3 }
    return 0;
}
