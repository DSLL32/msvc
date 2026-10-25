#include <wchar.h>
#include <recon.h>

typedef char wchar16_width[sizeof(WCHAR)==2?1:-1];

int main(int argc,char **argv)
{
    WCHAR text[3]={'a',0,0};
    WCHAR *(__cdecl *convert)(WCHAR *)=_wcsupr;
    (void)argv;
    if(argc>1) {
        if(argc==2)return convert(NULL)!=NULL;
        if(argc==3)text[0]=0;
        if(argc==5)text[0]=0x100;
        return convert(text)!=text;
    }
    return 0;
}
