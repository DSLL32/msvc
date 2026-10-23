#ifndef CVPACK_STRIP_H
#define CVPACK_STRIP_H
#include "image.h"
#include <recon.h>

#pragma pack(2)
typedef struct CoffSymbol {
    unsigned char name[8];
    unsigned long value;
    short section;
    unsigned short type;
    unsigned char storage_class, auxiliary_count;
} CoffSymbol;
typedef struct CoffAuxiliary {
    unsigned long tag_index;
    union { unsigned long total_size; struct { unsigned short number, size; } line; } misc;
    unsigned long line_offset, next_symbol;
    unsigned short type_index;
} CoffAuxiliary;
#pragma pack()
typedef char CoffAuxiliarySize[(sizeof(CoffAuxiliary)==18)?1:-1];
typedef char CoffSymbolSize[(sizeof(CoffSymbol)==18)?1:-1];
typedef struct CoffFileName {
    unsigned long symbol;
    char *name;
    struct CoffFileName *next;
} CoffFileName;
typedef char CoffFileNameSize[(sizeof(CoffFileName)==12)?1:-1];

typedef struct CoffFunction {
    char *name;
    unsigned short module, section;
    unsigned long offset, end;
    void *lines;
    unsigned long line_count;
    unsigned short first_line, reserved;
} CoffFunction;
typedef char CoffFunctionSize[(sizeof(CoffFunction)==28)?1:-1];

unsigned short find_first_coff_function(CoffSymbol *,CoffAuxiliary *,unsigned long *);
unsigned short find_next_coff_function(CoffSymbol *,CoffAuxiliary *,unsigned long *);
unsigned short fill_coff_function(CoffFunction *,const CoffSymbol *,const CoffAuxiliary *,unsigned long);
unsigned short read_coff_function_block(const CoffAuxiliary *,CoffSymbol *,CoffAuxiliary *,unsigned long *);
void read_coff_function_lines(CoffFunction *,CoffFunction *,unsigned long *,unsigned short);
void set_coff_function_end(CoffFunction *,unsigned long);
CoffFunction *publish_coff_function(CoffFunction *);

extern unsigned long *strip_section_rvas;
extern CoffFileName *strip_file_names;
unsigned short read_coff_file_name(char *,unsigned long);
void set_coff_function_name(CoffFunction *,const char *);
void assign_coff_function_module(CoffFunction *,unsigned short,unsigned long);
void set_coff_function_section(CoffFunction *,unsigned short);
void set_coff_function_line(CoffFunction *,unsigned short);
void set_coff_function_offset(CoffFunction *,unsigned long);
unsigned short read_coff_auxiliary(CoffAuxiliary *);
unsigned short read_coff_symbol(CoffSymbol *,unsigned long);
unsigned short next_coff_symbol(CoffSymbol *,unsigned long *);
int classify_coff_symbol(const CoffSymbol *);
#endif
