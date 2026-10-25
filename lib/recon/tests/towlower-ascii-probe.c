#include <assert.h>
#include <wctype.h>
#include <recon.h>

typedef char wchar16_width[sizeof(WCHAR)==2?1:-1];
static WCHAR (__cdecl *convert)(WCHAR)=towlower;
static void save_fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }

int main(int argc,char **argv)
{
    static const unsigned uppercase[]={65,66,67,68,69,70,71,72,73,74,75,76,77,
                                       78,79,80,81,82,83,84,85,86,87,88,89,90};
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,0x12345678}};
    unsigned char before[108],after[108],mb[257];
    unsigned short ctype[257];unsigned long handles[6];
    unsigned value,row,n,expected;WCHAR result;
    if(argc>1) {
        if(!strcmp(argv[1],"nonascii")) { convert(128);return 42; }
        if(!strcmp(argv[1],"high")) { convert(0xffff);return 42; }
        if(!strcmp(argv[1],"locale")) recon_lc_handle[2]=1033;
        if(!strcmp(argv[1],"codepage")) recon_lc_codepage=1252;
        if(!strcmp(argv[1],"mb-codepage")) recon_mb_codepage=1252;
        if(!strcmp(argv[1],"mb-width")) recon_mb_cur_max=2;
        if(strcmp(argv[1],"live")) { convert('S');return 42; }
        __asm__ __volatile__("fld1\n\tfldpi":::"st");
    }
    memcpy(mb,recon_mbctype,sizeof(mb));
    memcpy(ctype,recon_pctype-1,sizeof(ctype));
    memcpy(handles,recon_lc_handle,sizeof(handles));
    for(row=0;row<sizeof(errors)/sizeof(errors[0]);++row)for(value=0;value<128;++value) {
        expected=value;
        for(n=0;n<sizeof(uppercase)/sizeof(uppercase[0]);++n)
            if(value==uppercase[n]) expected="abcdefghijklmnopqrstuvwxyz"[n];
        errno=errors[row][0];recon_doserrno=errors[row][1];
        recon_SetLastError(errors[row][2]);
        save_fp(before);result=convert((WCHAR)value);save_fp(after);
        assert(result==expected && !memcmp(before,after,sizeof(before)));
        assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] &&
               recon_GetLastError()==errors[row][2]);
    }
    assert(!memcmp(mb,recon_mbctype,sizeof(mb)));
    assert(!memcmp(ctype,recon_pctype-1,sizeof(ctype)));
    assert(!memcmp(handles,recon_lc_handle,sizeof(handles)));
    assert(!recon_lc_codepage && !recon_mb_codepage && recon_mb_cur_max==1);
    if(argc>1) __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}
