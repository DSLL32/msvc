#include <wchar.h>
#include <recon.h>
typedef char wchar16_width[sizeof(WCHAR)==2?1:-1];
int main(int argc,char **argv)
{
    WCHAR left[]={'M','O','V','E','A','B','L','E',0},right[]={'m','o','v','e','a','b','l','e',0};
    int (__cdecl *compare)(const WCHAR *,const WCHAR *)=_wcsicmp;
    (void)argv;
    if(argc==1)return 0;
    if(argc==2)return compare(NULL,NULL);
    if(argc==3)return compare(left,right);
    if(argc==4)left[0]=right[0]=0;
    else left[0]=0x100;
    return compare(left,right);
}
