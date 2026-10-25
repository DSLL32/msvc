#include <wctype.h>
#include <recon.h>

typedef char wchar16_width[sizeof(WCHAR)==2?1:-1];

int main(int argc,char **argv)
{
    static const WCHAR inputs[]={0,'A',0x80,0xffff};
    WCHAR (__cdecl *convert)(WCHAR)=towlower;
    (void)argv;
    if(argc>1) return convert(inputs[argc-2])!=0;
    return 0;
}
