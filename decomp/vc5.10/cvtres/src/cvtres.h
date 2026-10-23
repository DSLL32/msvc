#ifndef CVTRES_H
#define CVTRES_H
#include <windows.h>
#include <stdio.h>
#include <recon.h>

typedef struct ResourceIdentifier ResourceIdentifier;
typedef struct ResourceEntry ResourceEntry;
typedef struct ResourceType ResourceType;
struct ResourceIdentifier {
    DWORD kind;
    union { WORD ordinal; ResourceIdentifier *next; } value;
    DWORD name_offset;
    WORD byte_count, length;
    WCHAR text[2];
};
typedef struct ResourceHeader {
    DWORD data_size, header_size, data_version;
    WORD memory_flags, language;
    DWORD version, characteristics;
} ResourceHeader;
struct ResourceEntry {
    ResourceEntry *next;
    ResourceIdentifier *name;
    DWORD offset;
    ResourceIdentifier *type;
    ResourceEntry *next_file;
    ResourceHeader *header;
    DWORD data_entry_offset;
    WORD symbol_index, language_count;
};
struct ResourceType {
    ResourceType *next;
    ResourceIdentifier *identifier;
    DWORD reserved;
    ResourceEntry *ids, *names;
    WORD id_count, name_count;
};
typedef struct ResourcePadding {
    DWORD zero;
    WORD reserved, name_bytes;
} ResourcePadding;
extern ResourcePadding resource_padding;
extern ResourceIdentifier *resource_strings;
extern ResourceEntry *resource_files;
extern ResourceType *resource_id_types, *resource_named_types;
extern WORD name_entry_count, name_directory_count, type_count;
extern WORD id_type_count, named_type_count, resource_count, language_entry_count;
extern BYTE *directory_buffer;
extern const char *resource_type_names[21];
extern unsigned short target_machine;
extern unsigned short resource_machine_kind;
extern char *input_name, *output_name;
extern int verbose, windows_ce, windows_ce_emulation, windows_ce_noconvert;
extern int writable_resources;

void __stdcall usage(int status);
char *__stdcall load_message(unsigned int id);
void error(unsigned short code, ...);
void warning(unsigned short code, ...);
long __stdcall tell_file(FILE *file);
long __stdcall seek_file(FILE *file, long offset, int origin);
void *__stdcall allocate_block(unsigned long size);
int __stdcall read_bytes(FILE *file, void *buffer, unsigned long size);
int __stdcall write_bytes(FILE *file, const void *buffer, unsigned long size);
unsigned long __stdcall copy_bytes(FILE *input, FILE *output, unsigned long size);
ResourceIdentifier *__stdcall read_identifier(FILE *file);
ResourceEntry *__stdcall insert_resource(ResourceIdentifier *type, ResourceIdentifier *name, ResourceHeader *header);
IMAGE_RESOURCE_DIRECTORY *__stdcall init_directory(IMAGE_RESOURCE_DIRECTORY *directory, DWORD characteristics, DWORD version, DWORD timestamp, WORD names, WORD ids);
IMAGE_RESOURCE_DATA_ENTRY *__stdcall init_data_entry(IMAGE_RESOURCE_DATA_ENTRY *entry, DWORD offset, DWORD size, DWORD codepage, DWORD reserved);
int __stdcall convert_ce_image(int ce, FILE *input, FILE *output, ResourceEntry *entry, int type);
int __stdcall convert_resources(FILE *input, FILE *output, unsigned long size,
                                int writable, unsigned long timestamp);
#endif
