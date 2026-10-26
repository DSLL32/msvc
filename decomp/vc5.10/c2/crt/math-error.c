/* The shipped LIBC matherr.obj and original 446d1c return zero without
 * consuming their exception argument. This is existing CRT compatibility,
 * not application reconstruction credit. The application default tail also
 * shares this physical leaf: its code-free fastcall alias has no inputs,
 * consumes no exception object and returns the same full EAX zero/RET0.
 * The canonical callback retains its cdecl exception-pointer signature. */
#include "c2-crt.h"
#include <recon.h>

int c2_math_error(struct _exception *exception)
{
    return 0;
}
