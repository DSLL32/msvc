/* Explicit canonical CRT substring probe; assembly observes x87 only. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mbstring.h>
#include <mbctype.h>
#ifdef RECON_NATIVE_TEST
#include <recon.h>
#endif
#ifndef RECON_NATIVE_TEST
__declspec(dllimport) unsigned long __lc_handle[6];
__declspec(dllimport) unsigned __lc_codepage;
#endif
static void snapshot(unsigned char *data)
{
#ifdef RECON_NATIVE_TEST
    __asm__ __volatile__("fnsave (%0); frstor (%0)"::"r"(data):"memory");
#else
    __asm mov eax,data
    __asm fnsave [eax]
    __asm frstor [eax]
#endif
}
static void initialize(void)
{
#ifdef RECON_NATIVE_TEST
    __asm__ __volatile__("fninit; fld1");
#else
    __asm fninit
    __asm fld1
#endif
}
int main(int argc,char **argv)
{
    const char *texts[]={"%b.obj","prefix%b.obj","%b.obj","ababa","aaaaa","","abc","abc","abc","\t\n x","abcabc"};
    const char *needles[]={"%b","%b","%B","aba","aaa","","","abcd","d"," x","bc"};
    unsigned char text[256],needle[256],saved_text[256],saved_needle[256],before[108],after[108],*result;
    unsigned state,n,i;long offset;int saved;unsigned long dos,error;FILE *file;
#ifdef RECON_NATIVE_TEST
    if(argc>1){
        const unsigned char *left=(const unsigned char*)"abc",*right=(const unsigned char*)"b";
        if(!strcmp(argv[1],"text"))left=(const unsigned char*)"\200";
        if(!strcmp(argv[1],"needle"))right=(const unsigned char*)"\200";
        if(!strcmp(argv[1],"null-text"))left=NULL;
        if(!strcmp(argv[1],"null-needle"))right=NULL;
        if(!strcmp(argv[1],"codepage"))recon_mb_codepage=932;
        if(!strcmp(argv[1],"1252"))recon_mb_codepage=1252;
        if(!strcmp(argv[1],"locale"))recon_lc_handle[2]=1033;
        if(!strcmp(argv[1],"lc-codepage"))recon_lc_codepage=1252;
        if(!strcmp(argv[1],"max"))recon_mb_cur_max=2;
        _mbsstr(left,right);return 42;
    }
#else
    {
        HMODULE module=GetModuleHandleA("MSVCRT.DLL");FILE *profile;
        if(!module)return 80;
        /* Ordinary /MD startup may select the ACP internally. Establish and
         * record the explicitly measured CP0 subset before any API rows. */
        profile=fopen("startup-mb-state.bin","wb");if(!profile)return 81;
        fwrite((char*)module+0x3f1c0,1,4,profile);fwrite((char*)module+0x3f1c8,1,257,profile);fclose(profile);
        if(_setmbcp(0)!=0)return 85;
        if(_getmbcp()!=0 || MB_CUR_MAX!=1 || *(DWORD*)((char*)module+0x3f1c0)!=0 || __lc_codepage || __lc_handle[2])return 80;
        profile=fopen("mb-state.bin","wb");if(!profile)return 81;
        fwrite((char*)module+0x3f1c0,1,4,profile);fwrite((char*)module+0x3f1c8,1,257,profile);fclose(profile);
        profile=fopen("locale-state.bin","wb");if(!profile)return 81;
        {unsigned maximum=MB_CUR_MAX;fwrite(&maximum,4,1,profile);}
        fwrite(&__lc_codepage,4,1,profile);fwrite(__lc_handle,4,6,profile);fclose(profile);
    }
#endif
    file=fopen("states.bin","wb");if(!file)return 82;
    for(state=0;state<4;++state)for(n=0;n<139;++n){
        memset(text,0x5a,256);memset(needle,0x5a,256);
        if(n<11){strcpy((char*)text,texts[n]);strcpy((char*)needle,needles[n]);}
        else{for(i=1;i<128;++i)text[i-1]=(unsigned char)i;text[127]=0;needle[0]=(unsigned char)(n-11);needle[1]=0;}
        memcpy(saved_text,text,256);memcpy(saved_needle,needle,256);initialize();snapshot(before);
        errno=(state&1)?111:0;_doserrno=77;SetLastError((state&2)?0x12345678:0);
        result=_mbsstr(text,needle);saved=errno;dos=_doserrno;error=GetLastError();snapshot(after);
        if(result && ((unsigned long)result<(unsigned long)text || (unsigned long)result>(unsigned long)text+strlen((char*)text)))return 83;
        offset=result?(long)((unsigned long)result-(unsigned long)text):-1;
        if(memcmp(text,saved_text,256)||memcmp(needle,saved_needle,256)||memcmp(before,after,108))return 84;
        printf("%u %u %ld %d %lu %lu\n",state,n,offset,saved,dos,error);
        fwrite(saved_text,1,256,file);fwrite(saved_needle,1,256,file);fwrite(text,1,256,file);fwrite(needle,1,256,file);
        fwrite(before,1,108,file);fwrite(after,1,108,file);fwrite(&offset,4,1,file);
    }
    fclose(file);return 0;
}
