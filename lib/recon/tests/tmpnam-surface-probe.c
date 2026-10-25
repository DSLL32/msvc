#include <stdio.h>
#include <recon.h>

int main(int argc,char **argv)
{
    char buffer[L_tmpnam];
    char *(__cdecl *make_name)(char *)=tmpnam;
    (void)argv;
    if(argc>1) return make_name(argc==2?NULL:buffer)!=NULL;
    return 0;
}
