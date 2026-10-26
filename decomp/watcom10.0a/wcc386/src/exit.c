/* This old host callback forwards its EAX argument to exit. Callers retain
 * aborts. Omit it on the defining translation unit: Watcom otherwise omits
 * the epilogue after an ordinary CALL and falls through to another public.
 * The compact period build emits CALLexit with a balanced, unreachable RET;
 * the target-specific residual preserves that distinction from originalJMP. */
#define WCC_DEFINE_EXIT
#include <stdlib.h>
#include "command.h"
#include <recon.h>
void MyExit(int status)
{
    exit(status);
}
