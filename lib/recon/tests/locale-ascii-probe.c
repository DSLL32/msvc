/* Explicit canonical original-only probe; setup is outside measured calls. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <mbstring.h>
#include <mbctype.h>
#include <ctype.h>
#include <recon.h>
#ifndef RECON_NATIVE_TEST
__declspec(dllimport) unsigned long __lc_handle[6];
__declspec(dllimport) unsigned __lc_codepage;

#endif
static unsigned long errors[2]={0,0x12345678};
static int values[2]={0,111};
static void before(unsigned i,unsigned j){errno=values[j];_doserrno=77;SetLastError(errors[i]);}
static void report(const char *name,unsigned a,unsigned b,long result,unsigned i,unsigned j)
{
    int saved=errno;unsigned long dos=_doserrno,error=GetLastError();
    printf("%s %u %u %ld %d %lu %lu\n",name,a,b,result,saved,dos,error);
    if(saved!=values[j] || dos!=77 || error!=errors[i])exit(91);
}
#ifndef RECON_NATIVE_TEST
static void save(const char *name,const void *data,unsigned bytes)
{
    FILE *file=fopen(name,"wb");if(!file||fwrite(data,1,bytes,file)!=bytes)exit(92);fclose(file);
}
#endif
int main(int argc,char **argv)
{
    unsigned c,i,j,n;unsigned char text[3],out[3],left[2],right[2];
    const unsigned char *p;char *entry,*value;unsigned long cp;char **e;
    const unsigned char source[]=" S:\\case\\c1.dll;quoted \"name\"\t";
    const unsigned char accept[]=" \t";
#ifdef RECON_NATIVE_TEST
    {
        static char assignment[]="RECON_PROBE_VALUE=Borrowed Value ";
        static char *environment[]={assignment,NULL};
        static char *arguments[]={"probe.exe",NULL};
        unsigned char mb[261];unsigned short ctype[257];char path[1024];FILE *file;
        ReconCrtLocaleProfile profile={"e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210",RECON_CRT_LOCALE_1252_EN_US,mb+4,257,ctype,257};
        if(argc<2)return 98;
        recon_crt_bind_arguments(1,arguments,environment);
        snprintf(path,sizeof(path),"%s/invoke-after-mb-state.bin",argv[1]);file=fopen(path,"rb");
        if(!file || fread(mb,1,sizeof(mb),file)!=sizeof(mb))return 98;
        fclose(file);
        snprintf(path,sizeof(path),"%s/invoke-after-ctype.bin",argv[1]);file=fopen(path,"rb");
        if(!file || fread(ctype,1,sizeof(ctype),file)!=sizeof(ctype))return 98;
        fclose(file);
        if(argc<3 || strcmp(argv[2],"no-profile"))recon_crt_apply_locale_profile(&profile);
        if(argc>2){
            const char *mode=argv[2];unsigned char high[]={128,0};
            if(!strcmp(mode,"nonascii-value"))environment[0][18]=(char)128;
            if(!strcmp(mode,"codepage"))recon_mb_codepage=932;
            if(!strcmp(mode,"lc-codepage"))recon_lc_codepage=932;
            if(!strcmp(mode,"handle"))recon_lc_handle[0]=1033;
            if(!strcmp(mode,"ctype"))recon_pctype['A']^=1;
            if(!strcmp(mode,"mb-table"))recon_mbctype['A'+1]^=1;
            if(!strcmp(mode,"max"))recon_mb_cur_max=2;
            if(!strcmp(mode,"pointer"))recon_pctype++;
            if(!strcmp(mode,"mb-dec"))_mbsdec(source,source+1);
            if(!strcmp(mode,"mb-icmp"))_mbsicmp(source,source);
            if(!strcmp(mode,"mb-nbicmp"))_mbsnbicmp(source,source,1);
            if(!strcmp(mode,"mb-pbrk"))_mbspbrk(source,accept);
            if(!strcmp(mode,"wcstol")){WCHAR w[]={'1',0};wcstol(w,NULL,10);}
            if(!strcmp(mode,"searchenv")){char output[260];_searchenv("absent","PATH",output);}
            if(!strcmp(mode,"nonascii-inc"))_mbsinc(high);
            if(!strcmp(mode,"nonascii-clen"))_mbclen(high);
            if(!strcmp(mode,"nonascii-space"))_ismbcspace(128);
            if(!strcmp(mode,"nonascii-copy"))_mbccpy(out,high);
            if(!strcmp(mode,"nonascii-compare"))_mbsncmp(high,source,1);
            if(!strcmp(mode,"nonascii-chr"))_mbschr(high,'x');
            if(!strcmp(mode,"nonascii-span"))_mbsspn(high,accept);
            if(!strcmp(mode,"nonascii-getenv"))getenv("\200");
            if(!strcmp(mode,"no-profile")){recon_mb_codepage=1252;recon_lc_codepage=1252;recon_lc_handle[2]=1033;}
        }
        cp=recon_mb_codepage;
    }
    printf("state %lu %d %u %lu %lu %lu %lu %lu %lu\n",cp,MB_CUR_MAX,recon_lc_codepage,recon_lc_handle[0],recon_lc_handle[1],recon_lc_handle[2],recon_lc_handle[3],recon_lc_handle[4],recon_lc_handle[5]);
#else
    (void)argc;(void)argv;
    if(!setlocale(LC_CTYPE,"English_United States.1252") || _setmbcp(1252))return 93;
    cp=*(unsigned long*)((unsigned char*)GetModuleHandleA("MSVCRT.dll")+0x3f1c0);
    printf("state %lu %d %u %lu %lu %lu %lu %lu %lu\n",cp,MB_CUR_MAX,__lc_codepage,__lc_handle[0],__lc_handle[1],__lc_handle[2],__lc_handle[3],__lc_handle[4],__lc_handle[5]);
    save("codepage.bin",&cp,4);save("mbctype.bin",_mbctype,257);save("ctype.bin",_pctype-1,514);
    if(_putenv("RECON_PROBE_VALUE=Borrowed Value "))return 94;
#endif
    entry=NULL;
    for(e=
#ifdef RECON_NATIVE_TEST
        recon_crt_environment
#else
        _environ
#endif
        ;*e;++e)if(!strncmp(*e,"RECON_PROBE_VALUE=",18)){entry=*e;break;}
    if(!entry)return 95;
    for(i=0;i<2;++i)for(j=0;j<2;++j){
        before(i,j);value=getenv("recon_probe_value");
        if(value!=entry+18)return 96;
        report("getenv",0,0,1,i,j);
        before(i,j);value=getenv("RECON_PROBE_MISSING");report("getenv",1,0,value==NULL,i,j);
        for(c=0;c<128;++c){
            text[0]=(unsigned char)c;text[1]=0;text[2]=0xa5;
            before(i,j);p=_mbsinc(text);report("inc",c,0,(long)(p-text),i,j);
            before(i,j);n=_mbclen(text);report("clen",c,0,(long)n,i,j);
            before(i,j);n=_ismbcspace(c);report("space",c,0,(long)n,i,j);
            memset(out,0xa5,3);before(i,j);_mbccpy(out,text);report("copy",c,0,(long)(out[0]|out[1]<<8|out[2]<<16),i,j);
            before(i,j);p=_mbschr(source,c);report("chr",c,0,p?(long)(p-source):-1,i,j);
            before(i,j);n=_mbsspn(text,accept);report("span",c,0,(long)n,i,j);
        }
        for(c=0;c<128;++c)for(n=0;n<128;++n){
            left[0]=(unsigned char)c;left[1]=0;right[0]=(unsigned char)n;right[1]=0;
            before(i,j);report("cmp",c,n,_mbsncmp(left,right,1),i,j);
        }
        before(i,j);report("cmp",128,128,_mbsncmp(NULL,NULL,0),i,j);
        before(i,j);report("span",128,0,_mbsspn((unsigned char*)" \t rest",accept),i,j);
    }
    value=getenv("RECON_PROBE_VALUE");value[0]='Q';
    if(entry[18]!='Q'||getenv("RECON_PROBE_VALUE")!=value)return 97;
    return 0;
}
