/* Named frontiers of the dump-mode graph. */
#include "dump.h"

void initialize_disassembler(FatalRoutine fatal,WarningRoutine warning) { __asm { int 3 } }

void dump_headers(const char *name,unsigned char archive) { __asm { int 3 } }
void dump_string_table(const char *name) { __asm { int 3 } }
void dump_sections(void) { __asm { int 3 } }
void dump_symbol_map(void) { __asm { int 3 } }
void dump_coff_symbols(void) { __asm { int 3 } }
