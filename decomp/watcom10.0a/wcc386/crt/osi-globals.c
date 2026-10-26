/* The DOS/OSI CRT exports this optional loader-supplied copyright pointer.
 * The original Win32 loader passesNULL; the NT SDK CRT lacks this OSI cell.
 * Keep the actual spelling and initial state for the old App consumer. */
#include "osi-globals.h"
#include <recon.h>
char *_Copyright;

#ifndef __linux__
/* The OSI startup writes the loader's AH to __OS. The period NT SDK lacks
 * that cell; this Win32 build has the observed NT value. Native storage is
 * already supplied by the shared Watcom profile (__OS->recon_watcom_os). */
char __OS=OS_NT;
#endif

/* OSI loader PGMPARMS+12 supplies a pointer to its zero-initialized BSS
 * BreakFlag (original loader43000c), copied by cstart into78ffe.
 * NT CLIB lacks that OSI scalar, so retain a real owned read/clear byte.
 * The current regular-file case has no console events; event delivery is
 * not modeled or claimed by this initial-state/lifetime adaptation. */
static char BreakFlag;
char *_BreakFlagPtr=&BreakFlag;
