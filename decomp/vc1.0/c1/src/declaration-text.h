#ifndef DECLARATION_TEXT_H
#define DECLARATION_TEXT_H
#include "parser.h"
#include <recon.h>

typedef struct DeclarationTextBuffer {
    short handle, reserved;
    char *bytes;
    unsigned short before, after, capacity, field0e;
} DeclarationTextBuffer;
typedef char DeclarationTextBuffer_size[sizeof(DeclarationTextBuffer) == 16 ? 1 : -1];
extern DeclarationTextBuffer declaration_text_pool[32];
extern unsigned next_declaration_text_handle;
extern int declaration_text_depth;
short begin_declaration_text(void);
short new_declaration_text(void);
extern char *unprintable_declaration_type;
extern int declaration_text_is_function;
short __stdcall build_declaration_text(FrontendSymbol *symbol, CType *type);
char *__stdcall declaration_text_value(short handle);
void __stdcall write_declaration_storage(unsigned storage, FILE *stream);
void __stdcall append_declaration_text(short handle, char *text);
short __stdcall append_type_text(short handle, CType *type);
short __stdcall build_declarator_text(FrontendSymbol *symbol, CType *type);
void __stdcall release_declaration_text(short handle);
void __stdcall prepend_declaration_text(short handle, char *text);
void __stdcall append_declaration_character(short handle, int character);
char *__stdcall reserve_declaration_text(short handle, int side, unsigned bytes);
DeclarationTextBuffer *__stdcall find_declaration_text(short handle);
typedef void (__stdcall *DeclarationTextWriter)(short handle, char *text);
short __stdcall write_declarator_attributes(short handle, DeclarationTextWriter writer,
    unsigned short attributes, FrontendSymbol *base, TypeShape *following);
short __stdcall append_type_shapes(short handle, TypeShape *shape);
short __stdcall build_based_text(unsigned short attributes, FrontendSymbol *base);
short __stdcall prepend_pointer_text(short handle, TypeShape *shape);
void __stdcall prepend_declaration_character(short handle, int character);
short __stdcall append_parameter_text(short handle, ParameterInfo *parameters, unsigned short shape_flags);
short __stdcall build_storage_text(short handle, unsigned storage);
short __stdcall append_storage_attributes(short handle, unsigned storage);
#endif
