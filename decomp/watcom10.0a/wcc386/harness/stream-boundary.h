#ifndef WCC_STREAM_BOUNDARY_H
#define WCC_STREAM_BOUNDARY_H
#include <stdio.h>
#include <recon.h>
#ifdef RECON_NATIVE_TEST
int wcc_fclose(FILE *stream);
FILE *wcc_fopen(const char *name,const char *mode);
#undef fopen
#define fopen wcc_fopen
#undef fclose
#define fclose wcc_fclose
#endif
#endif
