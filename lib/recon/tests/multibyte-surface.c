#include <mbstring.h>
#include <mbctype.h>
#include <recon.h>
int main(int argc,char **argv)
{
    unsigned char target[4]; const unsigned char *text=(const unsigned char *)"a";
    memset(target,0xa5,sizeof(target));
    if(argc<2) return 0;
    if(!strcmp(argv[1],"compare")) return _mbsncmp((const unsigned char *)"a",(const unsigned char *)"b",1)!=-1;
    if(!strcmp(argv[1],"find")) return _mbschr(text,'a')!=text;
    if(!strcmp(argv[1],"advance")) return _mbsinc(text)!=text+1;
    if(!strcmp(argv[1],"copy")) { _mbccpy(target,text); return target[0]!='a' || target[1]!=0xa5; }
    if(!strcmp(argv[1],"space")) return _ismbcspace(' ')!=8;
    if(!strcmp(argv[1],"upper")) return _mbsupr(target)==NULL;
    if(!strcmp(argv[1],"pbrk")) return _mbspbrk(text,text)==NULL;
    if(!strcmp(argv[1],"substring")) return _mbsstr(text,text)==NULL;
    if(!strcmp(argv[1],"fullcompare")) return _mbsicmp(text,text);
    if(!strcmp(argv[1],"bytecopy")) return _mbsnbcpy(target,text,1)==NULL;
    if(!strcmp(argv[1],"widecopy")) {WCHAR out[4]={0},word[2]={'a',0};return wcscat(out,word)==NULL;}
    if(!strcmp(argv[1],"wideformat")) {WCHAR out[4],format[2]={'a',0};return swprintf(out,format);}
    if(!strcmp(argv[1],"wideclass")) return iswctype('a',4);
    if(!strcmp(argv[1],"leadbyte")) return IsDBCSLeadByteEx(1252,'a');
    return 1;
}
