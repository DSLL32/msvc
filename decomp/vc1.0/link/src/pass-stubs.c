/* Pass-one operations not yet entered by accepted cases. */
#include "pass-one.h"
#include "archive.h"
#include "resources.h"
#include <recon.h>
void end_undefined_enumerator(UndefinedEnumerator *scan) { __asm { int 3 } }
void resource_usage(int error) { __asm { int 3 } }
int resource_name_to_ansi(char *output,const unsigned short *input,unsigned long count) { __asm { int 3 } return 0; }
void resource_error_prefix(void) { __asm { int 3 } }
