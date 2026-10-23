#ifndef CODEVIEW_OUTPUT_H
#define CODEVIEW_OUTPUT_H
#include "output.h"
#include <recon.h>
extern int codeview_output_started; /* original84fd8 */
extern struct DebugScope *codeview_scope_cursor; /* original7e8ac */
void __fastcall write_codeview_function(FunctionSymbol *symbol);
void __fastcall write_debug_scopes(FunctionSymbol *symbol);
void write_codeview_signature(void);
void write_codeview_object_name(void);
#endif
