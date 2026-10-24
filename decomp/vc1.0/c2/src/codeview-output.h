#ifndef CODEVIEW_OUTPUT_H
#define CODEVIEW_OUTPUT_H
#include "output.h"
#include <recon.h>
extern int codeview_output_started; /* original84fd8 */
extern struct DebugScope *codeview_scope_cursor; /* original7e8ac */
void __fastcall write_codeview_function(FunctionSymbol *symbol);
void __fastcall write_debug_scopes(FunctionSymbol *symbol);
void __fastcall write_debug_symbol(SymbolHeader *symbol);
void __fastcall write_codeview_block(FunctionSymbol *symbol, unsigned offset, unsigned bytes);
void __fastcall write_codeview_frame_variable(SymbolHeader *symbol);
void __fastcall write_codeview_register(SymbolHeader *symbol);
unsigned short __fastcall codeview_register_number(SymbolHeader *symbol);
void __fastcall write_codeview_local_data(SymbolHeader *symbol);
void __fastcall write_codeview_global_data(SymbolHeader *symbol);
void __fastcall write_codeview_label(SymbolHeader *symbol);
void __fastcall write_codeview_constant(SymbolHeader *symbol);
void __fastcall write_codeview_typedef(SymbolHeader *symbol);
void write_codeview_compile_record(void);
void __fastcall read_codeview_initializers(FILE *file);
void write_codeview_signature(void);
void write_codeview_object_name(void);
#endif
