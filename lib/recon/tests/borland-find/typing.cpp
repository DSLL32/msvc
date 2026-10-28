#include <dir.h>
#include <recon.h>
typedef char size_check[sizeof(ffblk)==272?1:-1];
static int (__cdecl *typed)(const char*,ffblk*,int)=findfirst;
