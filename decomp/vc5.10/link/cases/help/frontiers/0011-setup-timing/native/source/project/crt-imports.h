#ifndef LINK_CRT_IMPORTS_H
#define LINK_CRT_IMPORTS_H
#include <stdio.h>

/* LINK imports the CRT data array itself. The supplied SDK's newer /MD
 * convenience macro calls __p__iob instead, which adds a different vertex. */
#ifdef _DLL
#undef _iob
_CRTIMP extern FILE _iob[];
#endif
#include <recon.h>
#endif
