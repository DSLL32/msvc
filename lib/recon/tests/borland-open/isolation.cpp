#include <fcntl.h>
#include <recon.h>
#if defined(RECON_CRT_PROFILE_WATCOM_100A)
typedef char WatcomFlags[O_BINARY==0x200 && O_CREAT==0x20?1:-1];
static int (__cdecl *typed)(const char *,int,...)=open;
#else
#ifdef open
#error host/unselected bare open changed
#endif
typedef char HostFlags[O_CREAT==64 && O_APPEND==1024?1:-1];
#endif
