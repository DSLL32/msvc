#include <mbctype.h>
#include <recon.h>

typedef char MbLeadBit[(_M1==4)?1:-1];
typedef char MbLocaleSelection[(_MB_CP_LOCALE==-4)?1:-1];
typedef char MbTableExtent[(sizeof(_mbctype)==257)?1:-1];

int main(int argc,char **argv)
{
    int (__cdecl *select_codepage)(int)=_setmbcp;
    unsigned char *volatile table=_mbctype;
    if(argc>1) return select_codepage(atoi(argv[1]));
    if(table!=&recon_mbctype[0]) return 1;
    /* The caller reads the actual shared byte at index character+1. */
    if((table[1+'A']&_M1)!=0) return 2;
    return 0;
}
