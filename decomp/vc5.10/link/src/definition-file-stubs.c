/* Separate pending bodies retain their genuine named entry traps when the
 * definition-file caller is optimized. */
#include <windows.h>
#include "link-driver.h"
#include <recon.h>

void __stdcall collect_incremental_exports(LinkContext *context,const char *name,
                                           IncrementalDefinition *definition)
{ __asm { int 3 } }
void __stdcall warn_existing_export_section(const char *name)
{ __asm { int 3 } }
