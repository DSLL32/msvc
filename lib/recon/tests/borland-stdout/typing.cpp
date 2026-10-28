#include <stdio.h>
#include <unistd.h>
#include <recon.h>
static int (__cdecl *print_call)(const char *,...)=printf;
#ifdef RECON_BORLAND_400_STDOUT
static void (__cdecl *quick_call)(int)=_exit;
#else
#ifdef _exit
#error unselected quick exit remap
#endif
#endif
void type_check(void) {(void)print_call;
#ifdef RECON_BORLAND_400_STDOUT
(void)quick_call;
#endif
}
