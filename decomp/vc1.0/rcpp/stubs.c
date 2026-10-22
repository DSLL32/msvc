/* Named function frontiers; remove each body as its function is recovered. */
#include "macros.h"
int rc_exception_filter(unsigned code, EXCEPTION_POINTERS *info) { __asm { int 3 } return 0; }

void rc_runtime_abort(int error) { __asm int 3 }

BOOL WINAPI rc_control_handler(DWORD type) { __asm { int 3 } return 0; }

void rc_skip_line(void) { __asm int 3 }
