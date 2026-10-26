#ifndef WPP_STREAM_BOUNDARY_H
#define WPP_STREAM_BOUNDARY_H
#include <stdio.h>
#include <recon.h>
#ifdef RECON_NATIVE_TEST
int wpp_fclose(FILE *stream);
FILE *wpp_fopen(const char *name,const char *mode);
#undef fopen
#define fopen wpp_fopen
#undef fclose
#define fclose wpp_fclose
#endif
#endif
