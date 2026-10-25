#ifndef VC510_RC_H
#define VC510_RC_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

typedef int (WINAPI *DiagnosticCallback)(HWND, unsigned, char *);
/* Only NULL is observed for this slot in the first case. */
typedef void *ResourceCallback;
/* The six stack slots and cleanup are evidenced by both original modules. */
int WINAPI RC(HWND, unsigned, DiagnosticCallback,
                                   ResourceCallback, int, char **);
BOOL WINAPI Handler(DWORD);
int main(int, char **, char **);
int WINAPI rc_diagnostic(HWND, unsigned, char *);
#endif
