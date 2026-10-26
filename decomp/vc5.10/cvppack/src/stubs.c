/* Unreconstructed application boundaries retain their original call slots. */
#include "legacy-types.h"
#include "legacy-symbols.h"
#include "files.h"
#include "types.h"
#include "image.h"
#include "pack.h"
#include "widen.h"
#include "source-index.h"
#include "writer.h"
#include <recon.h>






void rewrite_mips_procedure_symbol(void) { __asm { int 3 } }

const char *module_name(ModuleInfo *info) { __asm { int 3 } return NULL; }













void convert_legacy_cobol0(TypeRecord *record) { __asm { int 3 } }
void convert_legacy_cobol1(TypeRecord *record) { __asm { int 3 } }
