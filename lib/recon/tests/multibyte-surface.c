#include <mbstring.h>
#include <mbctype.h>
#include <recon.h>
int main(int argc,char **argv)
{
    unsigned char target[4];
    if(argc<2) return 0;
    if(!strcmp(argv[1],"compare")) return _mbsncmp((const unsigned char *)"a",(const unsigned char *)"b",1);
    if(!strcmp(argv[1],"find")) return _mbschr((const unsigned char *)"a",'a')!=NULL;
    if(!strcmp(argv[1],"advance")) return _mbsinc((const unsigned char *)"a")!=NULL;
    if(!strcmp(argv[1],"copy")) _mbccpy(target,(const unsigned char *)"a");
    if(!strcmp(argv[1],"space")) return _ismbcspace(' ');
    return 1;
}
