#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static const char provider[]="e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210";
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
    unsigned short old_table[257],new_table[257],*old=recon_pctype,*selected;
    unsigned n;DWORD errors[3];int result,codepage=-4;char *text;
    const char *mode=argc>3?argv[3]:"";
    const ReconCrtCtypeSelection *state=recon_crt_ctype_selection_state();
    ReconCrtLocaleProfile combined={provider,RECON_CRT_LOCALE_1252_EN_US,mb,257,new_table,257};
    assert(argc>=3);
    read_table(argv[1],"locale-before-mbctype.bin",mb,sizeof(mb));
    read_table(argv[1],"locale-before-ctype.bin",old_table,sizeof(old_table));
    read_table(argv[1],"locale-after-ctype.bin",new_table,sizeof(new_table));
    read_table(argv[1],"locale-before-errors.bin",errors,sizeof(errors));
    assert(errors[0]==0 && errors[1]==0 && (errors[2]==0 || errors[2]==203));
    assert(!state->bound && !state->calls && !state->mb_calls);
    if(!strcmp(mode,"unbound")) {recon_setmbcp(-4);return 42;}
    if(!strcmp(mode,"combined")) {recon_crt_apply_locale_profile(&combined);recon_setmbcp(-4);return 42;}
    recon_crt_bind_empty_ctype_1252(provider,argv[2]);
    if(!strcmp(mode,"before-locale")) {recon_setmbcp(-4);return 42;}
    errno=0;recon_doserrno=0;recon_SetLastError(errors[2]);
    if(!strcmp(mode,"live")) __asm__ __volatile__("fld1":::"st");
    save_fp(fp_before);text=recon_setlocale(2,"");save_fp(fp_after);
    assert(!memcmp(fp_before,fp_after,108));
    selected=recon_pctype;
    assert(state->bound==1 && state->calls==1 && state->mb_calls==0 && state->result==text);
    assert(!strcmp(state->reference_sha256,argv[2]) && !strcmp(text,"English_United States.1252"));
    assert(errno==0 && recon_doserrno==0 && recon_GetLastError()==errors[2]);
    assert(selected!=old && !memcmp(old-1,old_table,sizeof(old_table)) && !memcmp(selected-1,new_table,sizeof(new_table)));
    assert(recon_mb_codepage==0 && recon_mb_codepage_gate==0 && recon_mb_cur_max==1 && recon_lc_codepage==1252);
    if(!strcmp(mode,"argument")) codepage=1252;
    if(!strcmp(mode,"zero")) codepage=0;
    if(!strcmp(mode,"other-selector")) codepage=-3;
    if(!strcmp(mode,"errno")) errno=2;
    if(!strcmp(mode,"doserrno")) recon_doserrno=2;
    if(!strcmp(mode,"last")) recon_SetLastError(2);
    if(!strcmp(mode,"mbcp")) recon_mb_codepage=1252;
    if(!strcmp(mode,"mbgate")) recon_mb_codepage_gate=1;
    if(!strcmp(mode,"mbmax")) recon_mb_cur_max=2;
    if(!strcmp(mode,"lccp")) recon_lc_codepage=0;
    if(!strcmp(mode,"handle")) recon_lc_handle[2]=0;
    if(!strcmp(mode,"mbtable")) recon_mbctype[200]^=1;
    if(!strcmp(mode,"ctype")) recon_pctype['A']^=1;
    if(!strcmp(mode,"oldtable")) old['A']^=1;
    if(!strcmp(mode,"pointer")) recon_pctype=old;
    if(!strcmp(mode,"result")) text[0]='e';
    save_fp(fp_before);result=recon_setmbcp(codepage);save_fp(fp_after);
    assert(!memcmp(fp_before,fp_after,108));
    assert(result==0 && state->mb_calls==1 && state->calls==1 && state->bound==1);
    assert(recon_mb_codepage==1252 && recon_mb_codepage_gate==0 && recon_mb_cur_max==1 && recon_lc_codepage==1252);
    assert(errno==0 && recon_doserrno==0 && recon_GetLastError()==errors[2]);
    assert(recon_pctype==selected && !memcmp(old-1,old_table,sizeof(old_table)) && !memcmp(selected-1,new_table,sizeof(new_table)));
    assert(!memcmp(recon_mbctype,mb,sizeof(mb)) && state->result==text && !strcmp(text,"English_United States.1252"));
    for(n=0;n<6;++n) assert(recon_lc_handle[n]==(n==2?1033:0));
    if(!strcmp(mode,"repeat")) recon_setmbcp(-4);
    if(!strcmp(mode,"locale-repeat")) recon_setlocale(2,"");
    if(!strcmp(mode,"live")) __asm__ __volatile__("fstp %%st(0)":::"st");
    puts("LC_CTYPE then SBCS locale-codepage selection, fullstate/errors/FP/inventory pass");
    return 0;
}
