/* Pass-one operations not yet entered by accepted cases. */
#include "pass-one.h"
#include "archive.h"
#include "resources.h"
#include <recon.h>
void end_undefined_enumerator(UndefinedEnumerator *scan) { __asm { int 3 } }
void resource_usage(int error) { __asm { int 3 } }
