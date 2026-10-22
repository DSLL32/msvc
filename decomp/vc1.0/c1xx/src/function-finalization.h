#ifndef FUNCTION_FINALIZATION_H
#define FUNCTION_FINALIZATION_H
#include "function-definitions.h"
#include <recon.h>
void __stdcall finish_function_body(int recovering);
void reset_function_symbol_tables(void);
extern CppSymbol *initializer_data_symbol;
void __stdcall emit_member_and_base_destructors(CppSymbol *tag);
void __stdcall emit_vbase_destructor_epilogue(CppSymbol *tag);
void finish_function_output(void);
void reset_materialized_value_index(void);
void finish_label_symbols(void);
int __stdcall serialize_scope(SymbolScope *scope);
void finish_inline_function(void);
typedef struct FunctionDirectoryEntry {
    CppSymbol *symbol;
    unsigned expression_offset, symbol_offset, complexity;
    unsigned short parameter_count, reserved;
    unsigned flags;
} FunctionDirectoryEntry;
typedef struct FunctionDirectoryBlock {
    struct FunctionDirectoryBlock *next;
    FunctionDirectoryEntry entries[20];
} FunctionDirectoryBlock;
extern FunctionDirectoryBlock *function_directory, *function_directory_tail;
extern unsigned function_directory_index;
typedef char FunctionDirectoryEntrySizeMustBe24[(sizeof(FunctionDirectoryEntry) == 24) ? 1 : -1];
typedef char FunctionDirectoryBlockSizeMustBe484[(sizeof(FunctionDirectoryBlock) == 484) ? 1 : -1];
typedef char FunctionDirectoryFlagsOffsetMustBe20[(offsetof(FunctionDirectoryEntry, flags) == 20) ? 1 : -1];
void __stdcall record_function_definition(CppSymbol *function);
void __stdcall emit_function_symbols(CppSymbol *symbols);
int defer_global_symbol(CppSymbol *symbol);
#endif
