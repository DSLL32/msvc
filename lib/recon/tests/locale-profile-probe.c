#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static void read_table(const char *directory,const char *name,void *data,size_t count)
{
    char path[1024];FILE *file;
    snprintf(path,sizeof(path),"%s/%s",directory,name);file=fopen(path,"rb");assert(file);
    assert(fread(data,1,count,file)==count && fgetc(file)==EOF);fclose(file);
}
int main(int argc,char **argv)
{
    unsigned char before_mb[261],after_mb[261],*mb_address=recon_mbctype;
    unsigned short before_ctype[257],after_ctype[257],*old=recon_pctype;
    const char *mode=argc>2?argv[2]:"";unsigned n;
    ReconCrtLocaleProfile profile={"e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210",RECON_CRT_LOCALE_1252_EN_US,after_mb+4,257,after_ctype,257};
    assert(argc>=2);
    read_table(argv[1],"invoke-before-mb-state.bin",before_mb,sizeof(before_mb));
    read_table(argv[1],"invoke-after-mb-state.bin",after_mb,sizeof(after_mb));
    read_table(argv[1],"invoke-before-ctype.bin",before_ctype,sizeof(before_ctype));
    read_table(argv[1],"invoke-after-ctype.bin",after_ctype,sizeof(after_ctype));
    if(!strcmp(mode,"default")){recon_crt_apply_locale_profile(&profile);return 42;}
    assert(recon_mb_codepage==0 && recon_lc_codepage==0 && recon_mb_cur_max==1);
    assert(!memcmp(recon_mbctype,before_mb+4,257) && !memcmp(old-1,before_ctype,sizeof(before_ctype)));
    assert(old[-1]==0 && old['\t']==0x28);
    if(!strcmp(mode,"provider"))profile.provider_sha256="ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
    if(!strcmp(mode,"kind"))profile.kind=99;
    if(!strcmp(mode,"mb-count"))profile.mbctype_count=256;
    if(!strcmp(mode,"ctype-count"))profile.ctype_count=256;
    if(!strcmp(mode,"mb"))after_mb[200]^=1;
    if(!strcmp(mode,"ctype-ascii"))after_ctype['A'+1]^=1;
    if(!strcmp(mode,"ctype-high"))after_ctype[230]^=1;
    if(!strcmp(mode,"ctype-eof"))after_ctype[0]=1;
    if(!strcmp(mode,"initial-ctype"))recon_pctype['A']^=1;
    if(!strcmp(mode,"initial-mb"))recon_mbctype['A'+1]^=1;
    if(!strcmp(mode,"initial-codepage"))recon_mb_codepage=1252;
    if(!strcmp(mode,"initial-handle"))recon_lc_handle[0]=1;
    errno=111;recon_doserrno=77;recon_SetLastError(12345678);
    recon_crt_apply_locale_profile(&profile);
    assert(errno==111 && recon_doserrno==77 && recon_GetLastError()==12345678);
    assert(recon_mb_codepage==1252 && recon_lc_codepage==1252 && recon_mb_cur_max==1);
    for(n=0;n<6;++n)assert(recon_lc_handle[n]==(n==2?1033:0));
    assert(recon_mbctype==mb_address && !memcmp(recon_mbctype,after_mb+4,257));
    assert(recon_pctype!=old && !memcmp(recon_pctype-1,after_ctype,sizeof(after_ctype)));
    assert(!memcmp(old-1,before_ctype,sizeof(before_ctype)) && recon_pctype[-1]==0 && recon_pctype['\t']==0x268);
    if(!strcmp(mode,"repeat"))recon_crt_apply_locale_profile(&profile);
    if(!strcmp(mode,"unproved-api"))recon_mbspbrk((const unsigned char *)"A",(const unsigned char *)"B");
    puts("all257 mb bytes/257 ctype words/scalars/pointer ownership/errors exact");return 0;
}
