/* Named frontiers of the editor and its built-in bind/rebase modes. */
#include "editor.h"
#include "editor-image.h"
#include <recon.h>



unsigned short *WINAPI fallback_process_relocations(void *page,unsigned long count,unsigned short *words,long delta) { __asm { int 3 } return NULL; }
void editor_rebase_error(const char *format,const void *value) { __asm { int 3 } }

void editor_bind_error(const char *format,const void *value) { __asm { int 3 } }
