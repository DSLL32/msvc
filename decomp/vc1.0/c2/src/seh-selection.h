#ifndef SEH_SELECTION_H
#define SEH_SELECTION_H
#include "exception-scope.h"
#include "instruction-selection.h"
#include <recon.h>
extern BackendSymbol *seh_finally_stack, *seh_registration, *seh_saved_stack;
extern BackendSymbol *seh_scope_table;
void initialize_exception_scope_table(void);
#endif
