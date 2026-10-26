#include <string.h>
#include <stdlib.h>
extern int __attribute__((stdcall)) ImportedExport(int,int (__attribute__((stdcall)) *)(int));
extern int ImportedCounter;
extern int shared_calls;
static int __attribute__((stdcall)) callback(int n) { return n+4; }
int main(int argc,char **argv,char **environment)
{
    char **p;
    if (argc!=3 || strcmp(argv[1],"one") || strcmp(argv[2],"two") || argv[3]) return 61;
    for(p=environment;*p && strcmp(*p,"ENTRY_TOKEN=present");++p) {}
    if (!*p || ImportedCounter!=18) return 62;
    if (ImportedExport(5,callback)!=27 || ImportedCounter!=27) return 63;
    return shared_calls>0?0:64;
}
int original_startup(int argc,char **argv,char **environment)
{
    /* The private partial link must rewrite this internal App-main reference. */
    return main(argc,argv,environment);
}
