/* Option helpers not yet entered by an accepted frontier. */
#include "options.h"
void exclude_default_library(const char *name,LibraryList *libraries) { __asm { int 3 } }
void exclude_all_default_libraries(const char *name,LibraryList *libraries) { __asm { int 3 } }
unsigned char read_switch_number(ParsedSwitch *option,unsigned short index,unsigned long *value) { __asm { int 3 } return 0; }
void initialize_order(FatalRoutine fatal,WarningRoutine warning) { __asm { int 3 } }
long crt_filelength(int file) { __asm { int 3 } return 0; }
unsigned crt_fread(void *buffer,unsigned size,unsigned count,FILE *stream) { __asm { int 3 } return 0; }
