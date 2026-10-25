#ifndef C2_FUNCTION_READER_H
#define C2_FUNCTION_READER_H
#include "code-records.h"
#include <recon.h>

C2ScopeHash *new_function_scope(void);
/* Each24-byte map associates a contiguous internal line span with source
 * lines in one filename. The cache walks older maps through previous. */
typedef struct C2InputLineMap {
    struct C2InputLineMap *next, *previous;
    unsigned first_line, last_line, source_line;
    char *filename;
} C2InputLineMap;
typedef char C2InputLineMapSizeCheck[sizeof(C2InputLineMap) == 24 ? 1 : -1];
typedef char C2InputLineMapPreviousCheck[offsetof(C2InputLineMap, previous) == 4 ? 1 : -1];
typedef char C2InputLineMapNameCheck[offsetof(C2InputLineMap, filename) == 20 ? 1 : -1];
extern C2InputLineMap *input_first_line_map;
extern C2InputLineMap *input_line_cache;
extern char *input_line_filename;
extern unsigned input_last_line;
extern int input_line_needs_record;
unsigned reset_function_line_input(void);
C2Record *new_statement_boundary(void);
C2SmallNode *__fastcall allocate_small_node(unsigned char kind);
void __fastcall create_function_flow(C2Function *function, C2Record *first,
                                     C2Record *last, int pool);
typedef struct C2InputStack C2InputStack;
C2Record *__fastcall read_function_code(C2Function *function, C2InputStack **pending,
                                   C2Record *first, int limit, int outermost);
void __fastcall validate_function_code(C2Record *first);
void finish_function_input(void);
void __fastcall finish_function_debug(C2Function *function);
void __fastcall finish_optimized_function_input(C2Function *function);
FILE *__fastcall read_pdb_symbol_il(unsigned serial, unsigned index);
FILE *__fastcall read_pdb_expression_symbols(unsigned serial, unsigned index);
FILE *__fastcall read_pdb_expression_il(unsigned serial, unsigned index);
#endif
