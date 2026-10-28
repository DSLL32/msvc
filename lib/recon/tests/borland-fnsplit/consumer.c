#include <dir.h>
#include <string.h>
#include <recon.h>
int use_split(const char *p,char *d,char *r,char *n,char *e){return fnsplit(p,d,r,n,e);}
int library_extension(const char *p){char ext[MAXEXT];return (fnsplit(p,0,0,0,ext)&EXTENSION) && !stricmp(ext,".lib");}
