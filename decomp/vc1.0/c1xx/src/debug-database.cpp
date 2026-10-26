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
    unsigned char * RECON_THISCALL allocate(unsigned bytes);
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
    int RECON_THISCALL open(char *filename, char *mode, unsigned signature);
    int RECON_THISCALL read_header(void);
    int RECON_THISCALL close_file(void);
    unsigned char * RECON_THISCALL record(unsigned short type);
    unsigned RECON_THISCALL copy_record(unsigned short type, unsigned char *output, unsigned *bytes);
    int RECON_THISCALL create(char *filename, unsigned signature);
    int RECON_THISCALL load(void);
    int RECON_THISCALL commit(void);
    unsigned short RECON_THISCALL intern(unsigned char *record);
};
typedef char DatabaseHeaderMustBe64[(sizeof(DatabaseHeader) == 64) ? 1 : -1];
typedef char DatabaseBlockMustBe12[(sizeof(DatabaseBlock) == 12) ? 1 : -1];
typedef char DatabaseLookupMustBe12[(sizeof(DatabaseLookup) == 12) ? 1 : -1];
typedef char DatabasePoolMustBe12[(sizeof(DatabasePool) == 12) ? 1 : -1];
typedef char DatabaseMustBe116[(sizeof(DebugDatabase) == 116) ? 1 : -1];
typedef char DatabaseFileMustBe100[(offsetof(DebugDatabase, file) == 100) ? 1 : -1];
typedef char DatabaseOffsetsFlagMustBe112[(offsetof(DebugDatabase, want_offsets) == 112) ? 1 : -1];

static const DatabaseHeader initial_database_header = {
    "Microsoft C/C++ program database 1.00\r\n\032JG", 0xe0d5c, 0, 0,
    0x1000, 0x1000, 0
};


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
/* Original0x18120..0x1813b: ECX=this, no stack arguments/RET0.
   Pool fields retain their old values after all owned blocks are freed. */
void RECON_THISCALL DatabasePool::release(void)
{
    DatabaseBlock *block = first, *following;
    while (block) {
        following = block->next;
        free(block);
        block = following;
    }
}
/* Original0x17300..0x17385: ECX=this, filename/signature on the
   stack, RET8. The fixed64B file header is a typed template, not arena data. */
int RECON_THISCALL DebugDatabase::create(char *filename, unsigned signature)
{
    file = _open(filename, _O_BINARY | _O_CREAT | _O_RDWR, 0777);
    if (file < 0) return 0;
    header = initial_database_header;
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
/* Original0x18050..0x1806e: ECX=this, bytes/RET4, actual data cursor
   returned in EAX. A zero-byte allocation still returns the current cursor. */
unsigned char * RECON_THISCALL DatabaseBlock::allocate(unsigned bytes)
{
    unsigned char *result;
    if (bytes > remaining) return 0;
    remaining -= bytes;
    result = cursor;
    cursor += bytes;
    return result;
}

/* Original0x17fc0..0x17fc8, cdecl1/RET0. */
unsigned __cdecl debug_database_signature(DebugDatabase *database)
{
    return database->header.timestamp;
}
/* Original0x17fd0..0x17fd8, cdecl1/RET0. */
unsigned __cdecl debug_database_age(DebugDatabase *database)
{
    return database->header.age;
}

/* Original0x17dd0..0x17ddf, cdecl2/RET0. The public C wrapper
   supplies the genuine ECX owner to its one-stack-argument member. */
unsigned short __cdecl debug_database_intern(DebugDatabase *database, unsigned char *record)
{
    return database->intern(record);
}
/* Original0x17630..0x17856: ECX=this, record on stack/RET4, AX index.
   Stored type records have a two-byte hash prefix before the counted leaf. */
unsigned short RECON_THISCALL DebugDatabase::intern(unsigned char *record)
{
    unsigned short hash;
    DatabaseLookup **bucket, *node;
    unsigned char *stored;
    DatabaseBlock *block;
    unsigned bytes, capacity, i;
    int equal;
    if (!want_lookup) fatal_error(1, "typesrvr.cpp", 285);
    bytes = (unsigned short)(*(unsigned short *)record + 2);
    if (bytes >= 0xff00) fatal_error(1, "typesrvr.cpp", 286);
    hash = hash_debug_database_record(record);
    bucket = &type_lookup[hash];
    for (node = *bucket; node; node = node->next) {
        equal = *(unsigned short *)node->record == hash;
        if (equal) {
            for (i = 0; i < bytes; ++i) {
                if (node->record[i + 2] != record[i]) { equal = 0; break; }
            }
        }
        if (equal) return node->type;
    }
    if (!writable) fatal_error(1, "typesrvr.cpp", 297);
    stored = records.allocate(bytes + 2);
    if (stored) {
        *(unsigned short *)stored = hash;
        for (i = 0; i < bytes; ++i) stored[i + 2] = record[i];
    }
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
        if (header.next_type < 0x1000 || header.next_type >= 0xfff0)
            fatal_error(1, "typesrvr.cpp", 158);
        node->next = *bucket;
        node->record = stored;
        node->type = header.next_type++;
    } else node = 0;
    *bucket = node;
    return node->type;
}
/* Original0x17860..0x17944, cdecl1/RET0; eight-way unrolling owns
   jump targets17944..17964. Sum the same defined counted words, then fold. */
unsigned short __cdecl hash_debug_database_record(unsigned char *record)
{
    unsigned bytes = (unsigned short)(*(unsigned short *)record + 2);
    unsigned sum = 0, count = bytes >> 2, i;
    if (bytes & 2) {
        sum = *(unsigned short *)record;
        record += 2;
    }
    for (i = 0; i < count; ++i) sum += ((unsigned *)record)[i];
    sum ^= sum >> 11;
    return ((sum >> 16) ^ sum) & 0xfff;
}
/* Original0x18070..0x1811c: ECX=this, bytes/RET4. This entry
   inlines the small block operations rather than calling those other methods. */
unsigned char * RECON_THISCALL DatabasePool::allocate(unsigned bytes)
{
    unsigned capacity;
    unsigned char *result;
    DatabaseBlock *block;
    if (bytes <= current->remaining) {
        current->remaining -= bytes;
        result = current->cursor;
        current->cursor += bytes;
    } else result = 0;
    if (!result) {
        capacity = (bytes + 0x1fff) & ~0xfffU;
        block = (DatabaseBlock *)malloc(capacity + sizeof(DatabaseBlock));
        if (block) {
            block->remaining = capacity;
            block->cursor = (unsigned char *)(block + 1);
            block->next = 0;
            current->next = block;
        } else current->next = 0;
        current = current->next;
        if (bytes <= current->remaining) {
            current->remaining -= bytes;
            result = current->cursor;
            current->cursor += bytes;
        } else result = 0;
        if (!result) fatal_error(1, "typesrvr.cpp", 58);
    }
    used += bytes;
    return result;
}

/* Original0x17a70..0x17b32: ECX=this, no stack arguments/RET0.
   Append only blocks allocated since load, then publish the updated header. */
int RECON_THISCALL DebugDatabase::commit(void)
{
    DatabaseBlock *block = records.first;
    unsigned bytes;
    if (!writable) fatal_error(1, "typesrvr.cpp", 363);
    while (block != scan_block) block = block->next;
    if (_lseek(file, header.type_bytes + sizeof(DatabaseHeader), SEEK_SET) < 0) return 0;
    while ((block = block->next) != 0) {
        bytes = (unsigned)(block->cursor - (unsigned char *)(block + 1));
        if (_write(file, block + 1, bytes) != bytes) return 0;
    }
    header.type_bytes = records.used;
    ++header.age;
    if (_lseek(file, 0, SEEK_SET) < 0) return 0;
    return _write(file, this, sizeof(DatabaseHeader)) == sizeof(DatabaseHeader);
}

/* Original0x18020..0x18029, cdecl1. The original tail transfer
   forwards the real database owner in ECX to its zero-stack-argument method. */
extern "C" int __cdecl debug_database_commit(DebugDatabase *database)
{
    return database->commit();
}
/* Original0x17ee0..0x17fab, cdecl1. The original inlines the
   deleting destructor: close once, release lookup/record owners, then free
   the database while returning only the initial close operation's status. */
extern "C" int __cdecl debug_database_close(DebugDatabase *database)
{
    DatabaseBlock *block, *next;
    int closed;
    if (database->file >= 0) {
        closed = _close(database->file) >= 0;
        database->file = -1;
    } else closed = 1;
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
        block = database->records.first;
        while (block) {
            next = block->next;
            free(block);
            block = next;
        }
        free(database);
    }
    return closed;
}

/* Original0x17080..0x17141, cdecl3. The original inlines its record
   pool allocator, including the typesrvr.cpp failure boundary. */
extern "C" unsigned char *__cdecl reserve_database_record(unsigned extra,
    DebugDatabase *database, unsigned char *record)
{
    unsigned bytes = extra + (unsigned short)(*(unsigned short *)record + 2);
    DatabasePool *pool = &database->records;
    DatabaseBlock *block = pool->current;
    unsigned char *memory;
    unsigned capacity;
    if (block->remaining >= bytes) {
        block->remaining -= bytes; memory = block->cursor; block->cursor += bytes;
    } else memory = 0;
    if (!memory) {
        capacity = (bytes + 0x1fff) & ~0xfffU;
        block = (DatabaseBlock *)malloc(capacity + sizeof(DatabaseBlock));
        if (block) {
            block->remaining = capacity; block->cursor = (unsigned char *)(block+1);
            block->next = 0; pool->current->next = block;
        } else pool->current->next = 0;
        pool->current = block = pool->current->next;
        if (block->remaining >= bytes) {
            block->remaining -= bytes; memory = block->cursor; block->cursor += bytes;
        } else memory = 0;
        if (!memory) fatal_error(1, "typesrvr.cpp", 58);
    }
    pool->used += bytes;
    return memory;
}

/* Original0x17150..0x172b1: ECX=this, filename/mode/signature, RET12.
   Mode-specific unwritten flags retain their original lifetime; unlike the
   separately qualified factory mode, this dormant entry has no observed
   stale-field value and introduces no native initialization. */
int RECON_THISCALL DebugDatabase::open(char *filename, char *mode, unsigned signature)
{
    int valid, closed;
    writable = mode[0] == 'w';
    switch (mode[1]) {
    case 'c': want_offsets = 1; break;
    case 'i': want_lookup = 1; break;
    default: want_offsets = want_lookup = 1; break;
    }
    file = _open(filename, _O_BINARY | (writable ? _O_RDWR : 0));
    if (file >= 0) {
        valid = _read(file, this, sizeof(DatabaseHeader)) == sizeof(DatabaseHeader) &&
            header.magic == 0xe0d5c && load();
        if (!valid) return 0;
        if (!writable) {
            if (file >= 0) { closed = _close(file) >= 0; file = -1; }
            else closed = 1;
            if (!closed) return 0;
        }
        return 1;
    }
    if (!writable) return 0;
    file = _open(filename, _O_BINARY | _O_CREAT | _O_RDWR, 0777);
    if (file < 0) valid = 0;
    else {
        header = initial_database_header;
        header.timestamp = signature;
        if (_write(file, this, sizeof(DatabaseHeader)) == sizeof(DatabaseHeader)) valid = load();
        else {
            if (file >= 0) { _close(file); file = -1; }
            valid = 0;
        }
    }
    return valid != 0;
}

/* Original0x172c0..0x172f4: ECX=this/RET0, current descriptor cursor. */
int RECON_THISCALL DebugDatabase::read_header(void)
{
    return _read(file, this, sizeof(DatabaseHeader)) == sizeof(DatabaseHeader) &&
        header.magic == 0xe0d5c && load();
}

/* Original0x17970..0x179c1: ECX=this/type WORD in stack DWORD/RET4.
   The native and original method both expose the stored hash-prefix owner. */
unsigned char * RECON_THISCALL DebugDatabase::record(unsigned short type)
{
    if (!want_offsets || header.first_type > type || header.next_type <= type)
        fatal_error(1, "typesrvr.cpp", 349);
    return type_offsets[type-0x1000];
}

/* Original0x179d0..0x17a70: ECX=this, three stack words/RET12.
   Required length remains in EAX and the output word even for a short buffer. */
unsigned RECON_THISCALL DebugDatabase::copy_record(unsigned short type,
    unsigned char *output, unsigned *bytes)
{
    unsigned char *stored;
    unsigned length, copied, i;
    if (!want_offsets || header.first_type > type || header.next_type <= type)
        fatal_error(1, "typesrvr.cpp", 349);
    stored = type_offsets[type-0x1000];
    length = (unsigned short)(*(unsigned short *)(stored+2) + 2);
    copied = length <= *bytes ? length : *bytes;
    for (i = 0; i < copied; ++i) output[i] = stored[i+2];
    *bytes = length;
    return length;
}

/* Original0x17b40..0x17b6d: ECX=this/RET0. Closing an already
   retired descriptor succeeds; failure still retires the stored descriptor. */
int RECON_THISCALL DebugDatabase::close_file(void)
{
    int closed;
    if (file < 0) return 1;
    closed = _close(file) >= 0;
    file = -1;
    return closed;
}

/* Original0x17de0..0x17e34, cdecl2. The public view skips the stored
   two-byte hash prefix; the original method's range check was inlined. */
extern "C" unsigned char *__cdecl debug_database_record(DebugDatabase *database,
    unsigned short type)
{
    if (!database->want_offsets || database->header.first_type > type ||
        database->header.next_type <= type) fatal_error(1, "typesrvr.cpp", 349);
    return database->type_offsets[type-0x1000] + 2;
}

/* Original0x17e40..0x17ee0, cdecl4. Keep the original inlined range
   and copy operations rather than adding an out-of-line member call. */
extern "C" unsigned __cdecl debug_database_copy_record(DebugDatabase *database,
    unsigned short type, unsigned char *output, unsigned *bytes)
{
    unsigned char *stored;
    unsigned length, copied, i;
    if (!database->want_offsets || database->header.first_type > type ||
        database->header.next_type <= type) fatal_error(1, "typesrvr.cpp", 349);
    stored = database->type_offsets[type-0x1000];
    length = (unsigned short)(*(unsigned short *)(stored+2) + 2);
    copied = length <= *bytes ? length : *bytes;
    for (i = 0; i < copied; ++i) output[i] = stored[i+2];
    *bytes = length;
    return length;
}

/* Original0x17fb0..0x17fb6, cdecl0: same magic as the actual file header. */
extern "C" unsigned __cdecl debug_database_magic(void) { return 0xe0d5c; }
/* Original0x17fe0..0x17fe9 and0x17ff0..0x17ff9, cdecl1/AX. */
extern "C" unsigned short __cdecl debug_database_first_type(DebugDatabase *database)
{ return database->header.first_type; }
extern "C" unsigned short __cdecl debug_database_next_type(DebugDatabase *database)
{ return database->header.next_type; }
/* Original0x18000..0x18018, cdecl1/EAX. Hash WORDs are excluded. */
extern "C" unsigned __cdecl debug_database_record_bytes(DebugDatabase *database)
{ return database->records.used + (database->header.first_type-database->header.next_type)*2; }
