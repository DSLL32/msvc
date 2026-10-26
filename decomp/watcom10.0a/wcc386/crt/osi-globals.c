/* The DOS/OSI CRT exports this optional loader-supplied copyright pointer.
 * The original Win32 loader passesNULL; the NT SDK CRT lacks this OSI cell.
 * Keep the actual spelling and initial state for the old App consumer. */
#include <recon.h>
char *_Copyright;
