#include <recon.h>

static void *__cdecl allocate(size_t bytes)
{ (void)bytes;_exit(42);return NULL; }
static void __cdecl release(void *pointer)
{ (void)pointer;_exit(43); }

int main(int argc,char **argv)
{
    char output[64];
    char *(__cdecl *demangle)(char *,const char *,int,void *(__cdecl *)(size_t),
                             void (__cdecl *)(void *),unsigned short)=__unDName;
    (void)argv;
    if(argc==1)return 0;
    if(argc==2)return demangle(NULL,"??_Einput",0,allocate,release,0x800)!=NULL;
    if(argc==3)return demangle(output,"?input@@",sizeof(output),allocate,release,0)!=NULL;
    return demangle(NULL,NULL,-1,NULL,NULL,0xffff)!=NULL;
}
