#include <dir.h>
#include <recon.h>
static int (__cdecl *typed)(const char*,char*,char*,char*,char*)=fnsplit;
typedef char Flags[WILDCARDS==1 && EXTENSION==2 && FILENAME==4 && DIRECTORY==8 && DRIVE==16?1:-1];
typedef char Limits[MAXPATH==260 && MAXDIR==256 && MAXEXT==256 && MAXDRIVE==3?1:-1];
