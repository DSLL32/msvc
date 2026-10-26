#ifndef WCC_STREAM_BOUNDARY_H
#define WCC_STREAM_BOUNDARY_H
#include <stdio.h>
#include <recon.h>
#ifdef RECON_NATIVE_TEST
int wcc_fclose(FILE *stream);
#undef fclose
#define fclose wcc_fclose
#endif
#endif
