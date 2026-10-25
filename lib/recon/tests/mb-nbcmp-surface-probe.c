#include <mbstring.h>
#include <recon.h>
int main(int argc,char **argv)
{
    int (__cdecl *compare)(const unsigned char *,const unsigned char *,size_t)=_mbsnbcmp;
    (void)argv;
    if(argc==1)return 0;
    if(argc==2)return compare(NULL,NULL,0);
    if(argc==3)return compare((const unsigned char *)"__imp_??_E",(const unsigned char *)"__imp_??_",9);
    return compare((const unsigned char *)"\200",(const unsigned char *)"a",1);
}
