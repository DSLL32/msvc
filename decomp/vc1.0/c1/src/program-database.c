/* Native program database allocation and file access. */
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "debug-info.h"
#include <recon.h>

/* Original 0x17b70..0x17dc3. Each list owns its own allocation chain;
   failure while opening or validating the file releases both chains. */
ProgramDatabase *open_program_database(char *path, char *mode, unsigned signature)
{
    ProgramDatabase *db;
    PdbBuffer *buffer;
    PdbBlock *block, *next;
    int valid;
    db = malloc(sizeof(ProgramDatabase));
    if (db) {
        db->type_offsets = 0;
        db->type_index = 0;
        buffer = malloc(sizeof(PdbBuffer));
        if (buffer) {
            buffer->available = 0;
            buffer->cursor = (unsigned char *)(buffer + 1);
            buffer->next = 0;
        }
        db->records.head = db->records.tail = buffer;
        db->records.count = 0;
        block = malloc(sizeof(PdbBlock));
        if (block) block = initialize_pdb_block(block, 0, 0);
        db->blocks.head = db->blocks.tail = block;
        db->blocks.count = 0;
        db->handle = -1;
    }
    if (!db) fatal_error(1, "typesrvr.cpp", 401);
    db->writable = mode[0] == 'w';
    switch (mode[1]) {
    case 'c': db->cumulative = 1; break;
    case 'i': db->incremental = 1; break;
    default: db->incremental = db->cumulative = 1; break;
    }
    db->handle = _open(path, _O_BINARY | (db->writable ? _O_RDWR : _O_RDONLY));
    if (db->handle >= 0) {
        valid = _read(db->handle, &db->header, sizeof(PdbHeader)) == sizeof(PdbHeader)
            && db->header.magic == 0x000e0d5c && load_program_database(db);
        if (valid) {
            if (db->writable) return db;
            if (db->handle >= 0) {
                valid = _close(db->handle) >= 0;
                db->handle = -1;
            }
            if (valid) return db;
        }
    } else if (db->writable && create_program_database(db, 0, path, signature)) {
        return db;
    }
    if (db) {
        if (db->handle != -1 && db->handle >= 0) {
            _close(db->handle);
            db->handle = -1;
        }
        if (db->type_offsets) free(db->type_offsets);
        if (db->type_index) free(db->type_index);
        for (block = db->blocks.head; block; block = next) {
            next = block->next;
            free(block);
        }
        destroy_pdb_buffer_list(&db->records);
        free(db);
    }
    return 0;
}

/* Original0x18120..0x1813b. The list member itself survives unchanged;
   its owner releases or discards it after this non-deleting destructor. */
void __fastcall destroy_pdb_buffer_list(PdbBufferList *self)
{
    PdbBuffer *buffer = self->head, *next;
    while (buffer) {
        next = buffer->next;
        free(buffer);
        buffer = next;
    }
}

/* Original 0x18030..0x18048, ECX this and one stack argument, RET 4. */
PdbBlock *__fastcall initialize_pdb_block(PdbBlock *self, unsigned unused_edx, unsigned capacity)
{
    self->available = capacity;
    self->cursor = (unsigned char *)(self + 1);
    self->next = 0;
    return self;
}

/* Original 0x17300..0x17385. The signature passed by begin_program_database
   is written into header byte 48; the companion debug reference uses it too. */
int __fastcall create_program_database(ProgramDatabase *self, unsigned unused_edx, char *path, unsigned signature)
{
    static const PdbHeader initial_header = {
        "Microsoft C/C++ program database 1.00\r\n\032JG", 0x000e0d5c,
        0, 0, 0x1000, 0x1000, 0
    };
    self->handle = _open(path, _O_BINARY | _O_CREAT | _O_RDWR, 0777);
    if (self->handle < 0) return 0;
    self->header = initial_header;
    self->header.timestamp = signature;
    if (_write(self->handle, &self->header, sizeof(PdbHeader)) != sizeof(PdbHeader)) {
        if (self->handle >= 0) {
            _close(self->handle);
            self->handle = -1;
        }
        return 0;
    }
    return load_program_database(self);
}

/* The original inlines this list growth at each allocation site, while
   retaining the block reservation and constructor as separate methods. */
#define RESERVE_LIST(list, bytes, result) do { \
    PdbBlock *new_block; \
    unsigned capacity; \
    (result) = (void *)reserve_pdb_block((list)->tail, 0, (bytes)); \
    if (!(result)) { \
        capacity = ((bytes) + 8191U) & ~4095U; \
        new_block = malloc(capacity + sizeof(PdbBlock)); \
        if (new_block) new_block = initialize_pdb_block(new_block, 0, capacity); \
        (list)->tail->next = new_block; \
        (list)->tail = new_block; \
        (result) = (void *)reserve_pdb_block((list)->tail, 0, (bytes)); \
        if (!(result)) fatal_error(1, "typesrvr.cpp", 58); \
    } \
    (list)->count += (bytes); \
} while (0)

/* Original 0x17390..0x1762c. Load serialized records, establishing both
   the type-index vector and the hash chains used by incremental insertion. */
int __fastcall load_program_database(ProgramDatabase *self)
{
    unsigned short limit, index;
    unsigned bytes, slot;
    unsigned char *record;
    PdbTypeEntry *entry, **bucket;
    limit = self->writable ? 0xfff0 : self->header.last_type;
    if (self->cumulative) {
        self->type_offsets = malloc(((unsigned)limit - 0x1000) * sizeof(unsigned char *));
        if (!self->type_offsets) return 0;
    }
    if (self->incremental) {
        self->type_index = malloc(16384);
        if (self->type_index)
            for (slot = 0; slot < 4096; ++slot) self->type_index[slot] = 0;
        if (!self->type_index) return 0;
    }
    bytes = self->header.record_bytes;
    RESERVE_LIST(&self->records, bytes, record);
    if (!record) return 0;
    bytes = self->header.record_bytes;
    if (_read(self->handle, record, bytes) != bytes) return 0;
    for (index = self->header.first_type; index < self->header.last_type; ++index) {
        if (self->type_offsets) self->type_offsets[index - 0x1000] = record;
        if (self->type_index) {
            bucket = self->type_index + *(unsigned short *)record;
            RESERVE_LIST(&self->blocks, 12U, entry);
            if (entry) {
                entry->next = *bucket;
                entry->record = record;
                entry->index = index;
            }
            *bucket = entry;
        }
        record += (unsigned short)(*(unsigned short *)(record + 2) + 2) + 2;
    }
    self->loaded_tail = self->records.tail;
    self->records.tail->available = 0;
    return 1;
}

/* Original 0x18050..0x1806e. Reservation is non-growing and unsigned. */
unsigned char *__fastcall reserve_pdb_block(PdbBlock *self, unsigned unused_edx, unsigned bytes)
{
    unsigned char *result;
    if (bytes > self->available) return 0;
    self->available -= bytes;
    result = self->cursor;
    self->cursor += bytes;
    return result;
}

/* Original 0x17fc0..0x17fc8. */
unsigned program_database_signature(ProgramDatabase *self)
{
    return self->header.timestamp;
}

/* Original 0x17fd0..0x17fd8. */
unsigned program_database_age(ProgramDatabase *self)
{
    return self->header.age;
}

/* Original 0x17dd0..0x17ddf. Public cdecl entry for the database
   method's ECX this pointer and one callee-cleaned stack operand. */
unsigned short intern_program_database_type(ProgramDatabase *self, unsigned char *record)
{
    return insert_program_database_type(self, 0, record);
}

/* Original 0x17630..0x17856, ECX this plus one stack operand.
   Intern the complete serialized record in its hash bucket and allocate an
   index only for a new byte sequence. Stored records prepend their hash. */
unsigned short __fastcall insert_program_database_type(ProgramDatabase *self,
    unsigned unused_edx, unsigned char *record)
{
    unsigned short hash, index;
    unsigned bytes, position;
    unsigned char *stored;
    PdbTypeEntry *entry, **bucket;
    if (!self->incremental) fatal_error(1, "typesrvr.cpp", 285);
    bytes = (unsigned short)(*(unsigned short *)record + 2);
    if (bytes >= 0xff00) fatal_error(1, "typesrvr.cpp", 286);
    hash = hash_pdb_record(record);
    bucket = self->type_index + hash;
    for (entry = *bucket; entry; entry = entry->next) {
        if (*(unsigned short *)entry->record != hash) continue;
        stored = entry->record + 2;
        for (position = 0; position < bytes; ++position)
            if (stored[position] != record[position]) break;
        if (position == bytes) return entry->index;
    }
    if (!self->writable) fatal_error(1, "typesrvr.cpp", 297);
    stored = reserve_pdb_record_bytes(&self->records, 0, bytes + 2);
    if (stored) {
        *(unsigned short *)stored = hash;
        for (position = 0; position < bytes; ++position) stored[position + 2] = record[position];
    }
    RESERVE_LIST(&self->blocks, 12U, entry);
    if (entry) {
        index = self->header.last_type;
        if (index < 0x1000 || index >= 0xfff0) fatal_error(1, "typesrvr.cpp", 158);
        self->header.last_type = index + 1;
        entry->next = *bucket;
        entry->record = stored;
        entry->index = index;
    }
    *bucket = entry;
    return entry->index;
}

/* Original 0x17860..0x17944; unrolled-loop dispatch at 0x17944..0x17964.
   Sum the two-byte prefix remainder and complete DWORDs before folding to
   the database's twelve-bit bucket number. Unsigned addition preserves wrap. */
unsigned short hash_pdb_record(unsigned char *record)
{
    unsigned bytes, words, sum = 0;
    bytes = (unsigned short)(*(unsigned short *)record + 2);
    if (bytes & 2) {
        sum = *(unsigned short *)record;
        record += 2;
    }
    words = bytes >> 2;
    while (words--) {
        sum += *(unsigned *)record;
        record += 4;
    }
    sum ^= sum >> 11;
    return (unsigned short)(sum ^ (sum >> 16)) & 0xfff;
}

/* Original 0x18070..0x1811c. This list method inlines its buffer
   constructor and reservation, unlike the separate block-list methods. */
unsigned char *__fastcall reserve_pdb_record_bytes(PdbBufferList *self,
    unsigned unused_edx, unsigned bytes)
{
    PdbBuffer *buffer, *next;
    unsigned capacity;
    unsigned char *result = 0;
    buffer = self->tail;
    if (buffer->available >= bytes) {
        buffer->available -= bytes;
        result = buffer->cursor;
        buffer->cursor += bytes;
    }
    if (!result) {
        capacity = (bytes + 8191U) & ~4095U;
        next = malloc(capacity + sizeof(PdbBuffer));
        if (next) {
            next->available = capacity;
            next->cursor = (unsigned char *)(next + 1);
            next->next = 0;
        }
        self->tail->next = next;
        self->tail = self->tail->next;
        buffer = self->tail;
        if (bytes <= buffer->available) {
            buffer->available -= bytes;
            result = buffer->cursor;
            buffer->cursor += bytes;
        } else result = 0;
        if (!result) fatal_error(1, "typesrvr.cpp", 58);
    }
    self->count += bytes;
    return result;
}

/* Original 0x18020..0x18029. The cdecl wrapper bridges to the writer's
   ECX-only method entry; its tail call has the same graph edge as this call. */
int commit_program_database(ProgramDatabase *self)
{
    return write_program_database(self);
}

/* Original 0x17a70..0x17b32. Append only buffers allocated after loading
   the database, then update the header's byte count and generation age. */
int __fastcall write_program_database(ProgramDatabase *self)
{
    PdbBuffer *buffer;
    unsigned bytes;
    if (!self->writable) fatal_error(1, "typesrvr.cpp", 363);
    buffer = self->records.head;
    while (buffer != self->loaded_tail) buffer = buffer->next;
    if (_lseek(self->handle, self->header.record_bytes + sizeof(PdbHeader), SEEK_SET) < 0) return 0;
    while ((buffer = buffer->next) != 0) {
        bytes = buffer->cursor - (unsigned char *)(buffer + 1);
        if (_write(self->handle, buffer + 1, bytes) != (int)bytes) return 0;
    }
    self->header.record_bytes = self->records.count;
    ++self->header.age;
    if (_lseek(self->handle, 0, SEEK_SET) < 0) return 0;
    return _write(self->handle, self, sizeof(PdbHeader)) == sizeof(PdbHeader);
}

/* Original 0x17ee0..0x17fab. Close first, remembering its status,
   then run the manager's inlined deleting destructor and both list releases. */
int close_program_database(ProgramDatabase *self)
{
    int valid = 1;
    PdbBuffer *buffer, *next;
    if (self->handle >= 0) {
        valid = _close(self->handle) >= 0;
        self->handle = -1;
    }
    if (self) {
        if (self->handle != -1 && self->handle >= 0) {
            _close(self->handle);
            self->handle = -1;
        }
        if (self->type_offsets) free(self->type_offsets);
        if (self->type_index) free(self->type_index);
        for (buffer = self->blocks.head; buffer; buffer = next) {
            next = buffer->next;
            free(buffer);
        }
        for (buffer = self->records.head; buffer; buffer = next) {
            next = buffer->next;
            free(buffer);
        }
        free(self);
    }
    return valid;
}
