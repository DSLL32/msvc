#include <assert.h>
#include <wctype.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    static const char members[]="0123456789ABCDEFabcdef";
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,0x12345678}};
    unsigned char before[108],after[108],mb[257];unsigned short table[257];
    unsigned long handles[6];unsigned value,row,n,expected;int result;
    if(argc>1) {
        if(!strcmp(argv[1],"nonascii")){iswctype(128,128);return 42;}
        if(!strcmp(argv[1],"eof")){iswctype(0xffff,128);return 42;}
        if(!strcmp(argv[1],"mask")){iswctype('0',132);return 42;}
        if(!strcmp(argv[1],"locale"))recon_lc_handle[2]=1033;
        if(!strcmp(argv[1],"codepage"))recon_lc_codepage=1252;
        if(!strcmp(argv[1],"mb-codepage"))recon_mb_codepage=1252;
        if(!strcmp(argv[1],"mb-width"))recon_mb_cur_max=2;
        if(strcmp(argv[1],"live")){iswctype('0',128);return 42;}
        __asm__ __volatile__("fld1\n\tfldpi":::"st");
    }
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    iswctype('0',128);return 42;
#else
    memcpy(mb,recon_mbctype,sizeof(mb));memcpy(table,recon_pctype-1,sizeof(table));
    memcpy(handles,recon_lc_handle,sizeof(handles));
    for(row=0;row<sizeof(errors)/sizeof(errors[0]);++row)for(value=0;value<128;++value) {
        expected=0;
        for(n=0;n<sizeof(members)-1;++n)if(value==(unsigned char)members[n])expected=128;
        errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
        fp(before);result=iswctype((WCHAR)value,128);fp(after);
        assert(result==(int)expected && !memcmp(before,after,108));
        assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] &&
               recon_GetLastError()==errors[row][2]);
    }
    assert(!memcmp(mb,recon_mbctype,sizeof(mb)) && !memcmp(table,recon_pctype-1,sizeof(table)));
    assert(!memcmp(handles,recon_lc_handle,sizeof(handles)));
    assert(!recon_lc_codepage && !recon_mb_codepage && recon_mb_cur_max==1);
    if(argc>1)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
#endif
}
