#ifndef CV510_CRT_IMPORTS_H
#define CV510_CRT_IMPORTS_H
#include <stdio.h>
/* This executable imports the FILE array itself. The later SDK /MD macro
 * otherwise introduces an accessor call that is absent from the original. */
#ifdef _DLL
#undef _iob
_CRTIMP extern FILE _iob[];
#endif
#include <recon.h>
#endif
