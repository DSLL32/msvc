#include <fcntl.h>
#include <io.h>
#include <recon.h>
static int (__cdecl *typed)(const char*,int,...)=open;
typedef char Flags[O_BINARY==0x8000 && O_APPEND==0x800 && O_CREAT==0x100 && O_TRUNC==0x200 && O_DENYNONE==0x40?1:-1];
