#ifndef LINK_ARCHIVE_H
#define LINK_ARCHIVE_H
#include "image.h"
extern unsigned long archive_data_offset,archive_member_size;
void read_archive_index(Library *,unsigned short,Image *);
PIMAGE_ARCHIVE_MEMBER_HEADER read_archive_header(void);
char *get_archive_member_name(Library *,PIMAGE_ARCHIVE_MEMBER_HEADER);
unsigned long read_big_endian_dword(const void *);
void dump_archive_header(Library *,IMAGE_ARCHIVE_MEMBER_HEADER,unsigned long);
void read_optional_header(int,IMAGE_OPTIONAL_HEADER *,unsigned short);
#endif
