#include <mbstring.h>
#include <recon.h>
int main(int argc,char **argv)
{
    unsigned char text[4]={'A',128,'Z',0};
    unsigned char * (__cdecl *convert)(unsigned char *)=_mbslwr;
    if(argc>1) {
        if(!strcmp(argv[1],"null")) return convert(NULL)!=NULL;
        if(!strcmp(argv[1],"empty")) text[0]=0;
        if(!strcmp(argv[1],"ascii")) text[1]='B';
        return convert(text)!=text;
    }
    return 0;
}
