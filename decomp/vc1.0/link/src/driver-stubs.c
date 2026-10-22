/* Link-driver operations that the recorded cases have not yet entered. */
#include "image.h"
void close_trace_file(void) { __asm { int 3 } }

External **get_externals_by_symbol(SymbolTable *table) { __asm { int 3 } return 0; }
