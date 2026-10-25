/* LF_TYPESERVER resolution and its linked type database provider. */
#include "pdb.h"
#include <assert.h>
#include <time.h>
#include <recon.h>

TypeServer *current_type_server;
char type_server_name[512], previous_type_server_name[512];
unsigned short type_server_generation, cached_type_count;
char type_server_default_paths[] = ".;";

void import_type_server(const unsigned char *record)
{
    char filename[256], first_failed[516], leaf[520], search[1024];
    char *paths, *separator, *out;
    const char *in, *original_name = (const char *)record+11;
    unsigned count, i;
    int mismatch = 0;
    count = record[10];
    for (i = 0; i < count; ++i) type_server_name[i] = original_name[i];
    type_server_name[count] = 0;
    if ((int)type_server_generation-(int)type_module_count == -1 &&
        !_stricmp(previous_type_server_name, type_server_name)) {
        if (type_server_signature(current_type_server) != *(const unsigned long *)(record+2))
            fatal_error(36, module_name(current_module), type_server_name);
        if (type_server_age(current_type_server) < *(const unsigned long *)(record+6))
            fatal_error(35, module_name(current_module), type_server_name);
        type_server_generation = type_module_count;
        local_type_count = cached_type_count;
        module_types_indexed = 1;
        if (previous_type_count > cached_type_count) {
            count = (previous_type_count-cached_type_count)*sizeof(*local_type_records);
            out = (char *)&local_type_records[cached_type_count];
            for (i = 0; i < count; ++i) out[i] = 0;
        }
        previous_type_count = 0;
        return;
    }
    in = type_server_name; out = previous_type_server_name;
    do { *out++ = *in; } while (*in++);
    type_server_generation = type_module_count;
    if (current_type_server) type_server_close(current_type_server);
    current_type_server = type_server_open(type_server_name, "rc");
    leaf[0] = '\\';
    _splitpath(type_server_name, NULL, NULL, leaf+1, filename);
    out = leaf;
    while (*out) ++out;
    in = filename;
    do { *out++ = *in; } while (*in++);
    in = getenv("LIB");
    if (in) {
        search[0] = '.'; search[1] = ';'; search[2] = 0;
        out = search+2;
        do { *out++ = *in; } while (*in++);
        paths = search;
    } else paths = type_server_default_paths;
    first_failed[0] = 0;
    for (;;) {
        if (current_type_server) {
            mismatch = type_server_signature(current_type_server) != *(const unsigned long *)(record+2);
            if (!mismatch) break;
        }
        if (mismatch && !first_failed[0]) {
            in = type_server_name; out = first_failed;
            do { *out++ = *in; } while (*in++);
        }
        if (!*paths) break;
        separator = strchr(paths, ';');
        if (separator) *separator = 0;
        in = paths; out = type_server_name;
        do { *out++ = *in; } while (*in++);
        if (separator) paths = separator+1;
        else *paths = 0;
        out = type_server_name;
        while (*out) ++out;
        in = leaf;
        do { *out++ = *in; } while (*in++);
        current_type_server = type_server_open(type_server_name, "rc");
    }
    if (!current_type_server || mismatch) {
        if (first_failed[0]) fatal_error(36, module_name(current_module), first_failed);
        for (i = 0; i < record[10]; ++i) type_server_name[i] = original_name[i];
        type_server_name[record[10]] = 0;
        fatal_error(34, type_server_name, NULL);
    }
    if (type_server_age(current_type_server) < *(const unsigned long *)(record+6))
        fatal_error(35, module_name(current_module), type_server_name);
    load_type_server_records(type_server_name);
}

TypeServer *type_server_open(const char *name, const char *mode)
{
    TypeServer *server = malloc(sizeof(*server));
    TypeServerBlock *block, *next;
    if (server) {
        server->type_offsets = NULL;
        server->type_hash = NULL;
        block = malloc(sizeof(*block));
        if (block) {
            block->available = 0;
            block->cursor = (unsigned char *)(block+1);
            block->next = NULL;
        }
        server->type_storage.first = block;
        server->type_storage.last = block;
        server->type_storage.count = 0;
        block = malloc(sizeof(*block));
        if (block) {
            block->available = 0;
            block->cursor = (unsigned char *)(block+1);
            block->next = NULL;
        }
        server->index_storage.first = block;
        server->index_storage.last = block;
        server->index_storage.count = 0;
        server->descriptor = -1;
    }
    if (!server) _assert("ptps", "typesrvr.cpp", 0x191);
    if (type_server_open_file(server, name, mode)) return server;
    if (server) {
        if (server->descriptor != -1 && server->descriptor >= 0) {
            _close(server->descriptor);
            server->descriptor = -1;
        }
        if (server->type_offsets) free(server->type_offsets);
        if (server->type_hash) free(server->type_hash);
        for (block = server->index_storage.first; block; block = next) {
            next = block->next;
            free(block);
        }
        for (block = server->type_storage.first; block; block = next) {
            next = block->next;
            free(block);
        }
        free(server);
    }
    return NULL;
}

/* Header for a newly created1.00 type database; existing headers are read intact. */
const TypeServerHeader empty_type_server_header = {
    "Microsoft C/C++ program database 1.00\r\n\x1aJG",
    0x000e0d5c, 0, 0, 0x1000, 0x1000, 0
};

int __stdcall type_server_open_file(TypeServer *server, const char *name, const char *mode)
{
    int okay;
    unsigned i;
    server->writing = *mode == 'w';
    if (mode[1] == 'c') {
        server->loaded = 1;
        /* Original defect: rc mode never initializes this control DWORD.
           Two independent natural SUPERPAD/PDB runs read zero throughout
           its lifetime. Use that observed value, as explicitly authorized. */
        if (*mode == 'r') server->incremental = 0;
    }
    else {
        if (mode[1] != 'i') server->loaded = 1;
        server->incremental = 1;
    }
    server->descriptor = _open(name, server->writing ? 0x8002 : 0x8000);
    if (server->descriptor >= 0) {
        okay = _read(server->descriptor, server, 64) == 64 &&
            server->magic == 0x000e0d5c && type_server_read_records(server);
    } else {
        if (!server->writing) return 0;
        server->descriptor = _open(name, 0x8102, 0777);
        if (server->descriptor < 0) return 0;
        for (i = 0; i < sizeof(empty_type_server_header); ++i)
            ((unsigned char *)server)[i] = ((const unsigned char *)&empty_type_server_header)[i];
        server->signature = time(NULL);
        if (_write(server->descriptor, server, 64) != 64) {
            if (server->descriptor >= 0) {
                _close(server->descriptor);
                server->descriptor = -1;
            }
            return 0;
        }
        okay = type_server_read_records(server);
    }
    if (!okay) return 0;
    if (!server->writing && server->descriptor >= 0) {
        okay = _close(server->descriptor) >= 0;
        server->descriptor = -1;
        if (!okay) return 0;
    }
    return 1;
}

/* Original [0002bd00,0002bd1d): link the genuine stored record into its bucket. */
TypeServerRecord *__stdcall type_server_record_init(TypeServerRecord *node,
    TypeServerRecord *next, unsigned char *record, unsigned short type)
{
    node->next = next;
    node->record = record;
    node->type = type;
    return node;
}

/* Original [0002bd20,0002bd46): byte-fill the allocated hash bucket table. */
#pragma intrinsic(memset)
void *type_server_fill_alloc(unsigned long size, unsigned char value)
{
    void *data = malloc(size);
    if (data) memset(data, value, size);
    return data;
}

int __stdcall type_server_read_records(TypeServer *server)
{
    unsigned short limit, index;
    unsigned long count, capacity;
    unsigned char *data;
    TypeServerArena *arena;
    TypeServerBlock *block;
    TypeServerRecord *node, **bucket;

    limit = server->writing ? 0xfff0 : server->last_type;
    if (server->loaded) {
        server->type_offsets = malloc(((unsigned long)limit-0x1000)*4);
        if (!server->type_offsets) return 0;
    }
    if (server->incremental) {
        server->type_hash = type_server_fill_alloc(0x4000, 0);
        if (!server->type_hash) return 0;
    }
    arena = &server->type_storage;
    count = server->type_bytes;
    block = arena->last;
    data = NULL;
    if (count <= block->available) {
        block->available -= count;
        data = block->cursor;
        block->cursor += count;
    }
    if (!data) {
        capacity = (count+0x1fff)&~0xfff;
        block = malloc(capacity+sizeof(*block));
        if (block) {
            block->available = capacity;
            block->cursor = (unsigned char *)(block+1);
            block->next = NULL;
        }
        arena->last->next = block;
        block = arena->last = arena->last->next;
        if (count <= block->available) {
            block->available -= count;
            data = block->cursor;
            block->cursor += count;
        }
        if (!data) _assert("p", "pool.h", 0x3a);
    }
    arena->count += count;
    if (!data) return 0;
    if (_read(server->descriptor, data, server->type_bytes) != server->type_bytes) return 0;
    for (index = server->first_type; index < server->last_type; ++index) {
        if (server->type_offsets) server->type_offsets[index-0x1000] = data;
        if (server->type_hash) {
            bucket = &server->type_hash[*(unsigned short *)data];
            arena = &server->index_storage;
            count = sizeof(*node);
            block = arena->last;
            node = NULL;
            if (count <= block->available) {
                block->available -= count;
                node = (TypeServerRecord *)block->cursor;
                block->cursor += count;
            }
            if (!node) {
                capacity = (count+0x1fff)&~0xfff;
                block = malloc(capacity+sizeof(*block));
                if (block) {
                    block->available = capacity;
                    block->cursor = (unsigned char *)(block+1);
                    block->next = NULL;
                }
                arena->last->next = block;
                block = arena->last = arena->last->next;
                if (count <= block->available) {
                    block->available -= count;
                    node = (TypeServerRecord *)block->cursor;
                    block->cursor += count;
                }
                if (!node) _assert("p", "pool.h", 0x3a);
            }
            arena->count += count;
            *bucket = node ? type_server_record_init(node, *bucket, data, index) : NULL;
        }
        data += (unsigned short)(*(unsigned short *)(data+2)+2)+2;
    }
    server->read_block = server->type_storage.last;
    server->read_block->available = 0;
    return 1;
}

unsigned long type_server_signature(TypeServer *server)
{
    return server->signature;
}

unsigned long type_server_age(TypeServer *server)
{
    return server->age;
}

char loaded_type_server_name[512];

void load_type_server_records(const char *name)
{
    unsigned short first, limit, type, index;
    unsigned long count;
    unsigned char *out, *record;
    const char *in;
    unsigned properties;
    TypeRecord *entry;
    if (!name) {
        if (current_type_server) type_server_close(current_type_server);
        name = loaded_type_server_name;
        current_type_server = type_server_open(name, "rc");
        if (!current_type_server) fatal_error(34, name, NULL);
    } else {
        in = name; out = (unsigned char *)loaded_type_server_name;
        do { *out++ = *in; } while (*in++);
    }
    if (verbose == 1) printf("Reading PDB: %s\n", name);
    if (previous_type_count) {
        count = (unsigned long)previous_type_count*sizeof(*local_type_records);
        out = (unsigned char *)local_type_records;
        while (count--) *out++ = 0;
        previous_type_count = 0;
    }
    first = type_server_first(current_type_server);
    limit = type_server_limit(current_type_server);
    for (type = first; type < limit; ++type) {
        record = type_server_record(current_type_server, type);
        index = local_type_count;
        if (previous_type_count) {
            count = (unsigned long)previous_type_count*sizeof(*local_type_records);
            out = (unsigned char *)local_type_records;
            while (count--) *out++ = 0;
            previous_type_count = 0;
        }
        ++local_type_count;
        if (local_type_count >= local_type_capacity) {
            local_type_capacity += 6000;
            local_type_records = realloc(local_type_records, (unsigned long)local_type_capacity*sizeof(*local_type_records));
            if (!local_type_records) fatal_error(13, NULL, NULL);
            out = (unsigned char *)&local_type_records[index];
            count = 6000*sizeof(*local_type_records);
            while (count--) *out++ = 0;
        }
        entry = &local_type_records[index];
        entry->data = record;
        entry->index = 0;
        entry->flags |= 0x400;
        entry->flags &= ~0x800;
        properties = 0;
        if (record) {
            switch (*(unsigned short *)(record+2)) {
            case 4: case 5: case 6:
                properties = (*(unsigned short *)(record+8)&0x80)>>7;
                break;
            case 7:
                properties = (*(unsigned short *)(record+10)&0x80)>>7;
                break;
            }
        }
        entry->flags = (entry->flags&~0x4000)|(properties<<14);
    }
    cached_type_count = local_type_count;
}

unsigned short type_server_first(TypeServer *server)
{
    return server->first_type;
}

unsigned short type_server_limit(TypeServer *server)
{
    return server->last_type;
}

unsigned char *type_server_record(TypeServer *server, unsigned short type)
{
    if (!server->loaded || type < server->first_type || type >= server->last_type)
        _assert("fGetCVRecords && fHasTi(ti)", "typesrvr.cpp", 0x15d);
    return server->type_offsets[type-0x1000]+2;
}
