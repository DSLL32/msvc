#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>

static const char provider[]="e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210";
static const char reference[]="ebdbb204d8af623e8e00defcad03832006960fa7f4c88ec22317b76c0c638568";
static void read_table(const char *directory,const char *name,void *data,size_t count)
{
    char path[1024];FILE *file;
    assert(snprintf(path,sizeof(path),"%s/%s",directory,name)<(int)sizeof(path));
    file=fopen(path,"rb");assert(file);
    assert(fread(data,1,count,file)==count && fgetc(file)==EOF);fclose(file);
}
static void save_fp(unsigned char bytes[108])
{
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes));
}
int main(int argc,char **argv)
{
    unsigned char mb[257],fp_before[108],fp_after[108];
    unsigned short before[257],after[257],*old=recon_pctype;
    unsigned n;const char *mode=argc>2?argv[2]:"";
    const ReconCrtCtypeSelection *state=recon_crt_ctype_selection_state();
    char *result;int category=2;const char *name="";
    ReconCrtLocaleProfile combined={provider,RECON_CRT_LOCALE_1252_EN_US,mb,257,after,257};
    assert(argc>=2);
    read_table(argv[1],"locale-before-mbctype.bin",mb,sizeof(mb));
    read_table(argv[1],"locale-before-ctype.bin",before,sizeof(before));
    read_table(argv[1],"locale-after-ctype.bin",after,sizeof(after));
    assert(!state->bound && !state->calls && !state->result);
    if(!strcmp(mode,"unbound")) {recon_setlocale(2,"");return 42;}
    if(!strcmp(mode,"provider")) {recon_crt_bind_empty_ctype_1252("bad",reference);return 42;}
    if(!strcmp(mode,"reference")) {recon_crt_bind_empty_ctype_1252(provider,"bad");return 42;}
    if(!strcmp(mode,"reference-hex")) {recon_crt_bind_empty_ctype_1252(provider,"gggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg");return 42;}
    if(!strcmp(mode,"null-provider")) {recon_crt_bind_empty_ctype_1252(NULL,reference);return 42;}
    if(!strcmp(mode,"null-reference")) {recon_crt_bind_empty_ctype_1252(provider,NULL);return 42;}
    if(!strcmp(mode,"combined-first")) recon_crt_apply_locale_profile(&combined);
    errno=111;recon_doserrno=222;recon_SetLastError(333);
    if(!strcmp(mode,"live")) __asm__ __volatile__("fld1":::"st");
    save_fp(fp_before);
    recon_crt_bind_empty_ctype_1252(provider,reference);
    save_fp(fp_after);
    assert(!memcmp(fp_before,fp_after,108));
    assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==333);
    assert(state==recon_crt_ctype_selection_state() && state->bound==1 && !state->calls && !state->result);
    assert(!strcmp(state->reference_sha256,reference));
    assert(recon_pctype==old && !memcmp(old-1,before,sizeof(before)) && !memcmp(recon_mbctype,mb,sizeof(mb)));
    if(!strcmp(mode,"rebind")) recon_crt_bind_empty_ctype_1252(provider,reference);
    if(!strcmp(mode,"combined-bound")) recon_crt_apply_locale_profile(&combined);
    if(!strcmp(mode,"ctype")) recon_pctype['A']^=1;
    if(!strcmp(mode,"mbtable")) recon_mbctype[200]^=1;
    if(!strcmp(mode,"pointer")) recon_pctype=old+1;
    if(!strcmp(mode,"mbcp")) recon_mb_codepage=1252;
    if(!strcmp(mode,"mbgate")) recon_mb_codepage_gate=1;
    if(!strcmp(mode,"mbmax")) recon_mb_cur_max=2;
    if(!strcmp(mode,"lccp")) recon_lc_codepage=1252;
    if(!strcmp(mode,"handle")) recon_lc_handle[0]=1;
    if(!strcmp(mode,"category")) category=0;
    if(!strcmp(mode,"query")) name=NULL;
    if(!strcmp(mode,"name")) name="English_United States.1252";
    errno=!strcmp(mode,"errno")?1:0;recon_doserrno=!strcmp(mode,"doserrno")?1:0;
    recon_SetLastError(!strcmp(mode,"last")?2:203);
    save_fp(fp_before);result=recon_setlocale(category,name);save_fp(fp_after);
    assert(!memcmp(fp_before,fp_after,108));
    assert(errno==0 && recon_doserrno==0 && recon_GetLastError()==203);
    assert(state->calls==1 && state->result==result && !strcmp(result,"English_United States.1252"));
    assert(recon_lc_codepage==1252 && recon_mb_codepage==0 && recon_mb_codepage_gate==0 && recon_mb_cur_max==1);
    for(n=0;n<6;++n) assert(recon_lc_handle[n]==(n==2?1033:0));
    assert(recon_pctype!=old && !memcmp(recon_pctype-1,after,sizeof(after)));
    assert(!memcmp(old-1,before,sizeof(before)) && !memcmp(recon_mbctype,mb,sizeof(mb)));
    assert((uintptr_t)result+strlen(result)+1<=(uintptr_t)(recon_pctype-1) ||
           (uintptr_t)(recon_pctype-1)+sizeof(after)<=(uintptr_t)result);
    result[0]='e';assert(result[0]=='e');result[0]='E';
    recon_pctype['A']^=1;recon_pctype['A']^=1;
    assert(!memcmp(recon_pctype-1,after,sizeof(after)) && !strcmp(result,"English_United States.1252"));
    if(!strcmp(mode,"repeat")) recon_setlocale(2,"");
    if(!strcmp(mode,"mb-api")) recon_mbsstr((const unsigned char *)"a",(const unsigned char *)"a");
    if(!strcmp(mode,"live")) __asm__ __volatile__("fstp %%st(0)":::"st");
    puts("isolated CTYPE fulltables/old-new-result ownership/MB/errors/FP/inventory pass");
    return 0;
}
