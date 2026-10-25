/* Unreconstructed application boundaries retain their original call slots. */
#include "files.h"
#include "types.h"
#include "image.h"
#include "pack.h"
#include "widen.h"
#include "source-index.h"
#include "writer.h"
#include <recon.h>






void decode_legacy_lines(void *lines, unsigned size) { __asm { int 3 } }
void rewrite_legacy_lines(void *lines, CodeViewDirectoryEntry *entry, void *module_buffer, ModuleInfo *info) { __asm { int 3 } }
void rewrite_register16_symbol(void) { __asm { int 3 } }
void rewrite_data16_symbol(void) { __asm { int 3 } }
void rewrite_global_data16_symbol(void) { __asm { int 3 } }
void rewrite_procedure16_symbol(void) { __asm { int 3 } }
void rewrite_mips_procedure_symbol(void) { __asm { int 3 } }

const char *module_name(ModuleInfo *info) { __asm { int 3 } return NULL; }





void convert_legacy_record(unsigned index) { __asm { int 3 } }




void read_legacy_module_types(void *types, unsigned size) { __asm { int 3 } }
