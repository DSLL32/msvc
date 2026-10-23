#ifndef RECONSTRUCTED_RC_PREPROCESS_H
#define RECONSTRUCTED_RC_PREPROCESS_H
#include <windows.h>
#include <recon.h>

WCHAR **__stdcall rc_widen_arguments(int,char **);
int __cdecl rc_preprocessor_main(int,WCHAR **);
#endif
