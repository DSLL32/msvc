#ifndef LINK_RESOURCES_H
#define LINK_RESOURCES_H
#include "pass-one.h"
#include <recon.h>
int convert_resources(int,char **);
void resource_usage(int error);
void * resource_allocate(unsigned short size);
long resource_seek(int file,long position,int origin);
unsigned short resource_read(int file,void *buffer,unsigned short count);
void initialize_resource_buffers(void);
int convert_resource_file(int input,int output,unsigned long size);
typedef struct ResourceName {
    unsigned long kind;
    union { unsigned short id; struct ResourceName *next; } value;
    unsigned long string_offset;
    unsigned short encoded_size,length;
    unsigned short text[1];
} ResourceName;
typedef struct ResourceRecord {
    unsigned long size,header_size,data_version;
    unsigned short flags,language;
    unsigned long version,characteristics;
} ResourceRecord;
typedef struct ResourceEntry {
    struct ResourceEntry *next;
    ResourceName *name;
    unsigned long position;
    ResourceName *type;
    struct ResourceEntry *file_next;
    ResourceRecord *record;
    unsigned long directory_offset;
    unsigned short sequence,language_count;
} ResourceEntry;
typedef struct ResourceType {
    struct ResourceType *next;
    ResourceName *name;
    unsigned long reserved;
    ResourceEntry *id_entries,*named_entries;
    unsigned short id_count,named_count;
} ResourceType;
ResourceName * read_resource_name(int file);
ResourceEntry * insert_resource(ResourceName *type,ResourceName *name,ResourceRecord *record);
void resource_copy_ansi_identifier(unsigned char **output,const unsigned char **input);
unsigned long convert_legacy_image_group(const unsigned char *input,unsigned long size,unsigned char *output);
int resource_name_to_ansi(char *output,const unsigned short *input,unsigned long count);
void set_resource_directory(IMAGE_RESOURCE_DIRECTORY *directory,unsigned long characteristics,unsigned long version,unsigned long stamp,unsigned short named,unsigned short ids);
void set_resource_data(IMAGE_RESOURCE_DATA_ENTRY *entry,unsigned long offset,unsigned long size,unsigned long code_page,unsigned long reserved);
int copy_resource_data(int input,int output,unsigned long size);
int convert_legacy_resource(int input,int output,ResourceRecord *record,unsigned short type,unsigned short name);
unsigned long resource_write(int file,const void *buffer,unsigned long count);
unsigned resource_unicode_to_ansi_byte(unsigned short);
void *resource_reallocate(void *allocation,unsigned short size);
int resource_release(void *allocation);
int detect_resource_encoding(FILE *);
int resource_byte_order(void);
#endif
