#ifndef LINKER_TYPES_H
#define LINKER_TYPES_H
#include "link.h"

typedef struct TextBuffer {
    char *data;
    unsigned long length, capacity;
} TextBuffer;

typedef struct ArenaBlock {
    struct ArenaBlock *next;
    unsigned long used, available;
} ArenaBlock;

void out_of_memory(void);
void reserve_text(TextBuffer *,unsigned long);
unsigned long append_text(TextBuffer *,const void *,unsigned long);
void release_text(TextBuffer *);
void *allocate_arena(ArenaBlock **,unsigned long);
extern int incremental_file;
int open_file(const char *,int,int);
int close_file(int,unsigned char);
void fatal_message(const char *,int,...);
unsigned read_file(int,void *,unsigned);
long seek_file(int,long,int);
long file_size(int);
void warning_message(const char *,unsigned,...);
unsigned read_coff_header(int,IMAGE_FILE_HEADER *);
unsigned read_section_header(int,IMAGE_SECTION_HEADER *);
char *read_coff_strings(const char *,unsigned long,unsigned long *);
PIMAGE_SYMBOL read_coff_symbols(unsigned long,unsigned long);
char *coff_section_name(const char [8],char *);
#endif
