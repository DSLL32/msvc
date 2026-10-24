#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static const char provider[]="e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210";
static void read_bytes(const char *directory,const char *name,void *data,size_t count)
{
    char path[1024];FILE *file;
    assert(snprintf(path,sizeof(path),"%s/%s",directory,name)<(int)sizeof(path));
    file=fopen(path,"rb");assert(file);
    assert(fread(data,1,count,file)==count && fgetc(file)==EOF);fclose(file);
}
static void save_fp(unsigned char bytes[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes)); }
int main(int argc,char **argv)
{
    unsigned char mb[257],record[2637],fp_before[108],fp_after[108];
    unsigned short old_table[257],new_table[257],*old=recon_pctype,*selected;
    unsigned inputs[256],outputs[256],meta[20],n,count,value=65,result;
    DWORD errors[3];char name[80],*text;const char *mode;
    const ReconCrtCtypeSelection *state=recon_crt_ctype_selection_state();
    ReconCrtLocaleProfile combined={provider,RECON_CRT_LOCALE_1252_EN_US,mb,257,new_table,257};
    assert(argc==5);mode=argv[3];count=(unsigned)atoi(argv[4]);assert(count==255 || count==256);
    read_bytes(argv[1],"locale-before-mbctype.bin",mb,sizeof(mb));
    read_bytes(argv[1],"locale-before-ctype.bin",old_table,sizeof(old_table));
    read_bytes(argv[1],"locale-after-ctype.bin",new_table,sizeof(new_table));
    read_bytes(argv[1],"locale-before-errors.bin",errors,sizeof(errors));
    for(n=0;n<count;++n) {
        sprintf(name,"path-fold-%03u-return.bin",n);read_bytes(argv[1],name,record,sizeof(record));
        memcpy(meta,record,sizeof(meta));inputs[n]=meta[11];outputs[n]=meta[5];
        assert(inputs[n]<=255 && outputs[n]<=255);
    }
    if(!strcmp(mode,"unbound")) {recon_mbctolower(value);return 42;}
    if(!strcmp(mode,"combined")) {recon_crt_apply_locale_profile(&combined);recon_mbctolower(value);return 42;}
    recon_crt_bind_empty_ctype_1252(provider,argv[2]);
    if(!strcmp(mode,"before-locale")) {recon_mbctolower(value);return 42;}
    errno=0;recon_doserrno=0;recon_SetLastError(errors[2]);text=recon_setlocale(2,"");selected=recon_pctype;
    if(!strcmp(mode,"before-mb")) {recon_mbctolower(value);return 42;}
    assert(recon_setmbcp(-4)==0);
    if(!strcmp(mode,"wide")) value=256;
    if(!strcmp(mode,"uintmax")) value=~0U;
    if(!strcmp(mode,"bound")) ((ReconCrtCtypeSelection *)state)->bound=0;
    if(!strcmp(mode,"calls")) ((ReconCrtCtypeSelection *)state)->calls=0;
    if(!strcmp(mode,"mbcalls")) ((ReconCrtCtypeSelection *)state)->mb_calls=0;
    if(!strcmp(mode,"mbcp")) recon_mb_codepage=0;
    if(!strcmp(mode,"mbgate")) recon_mb_codepage_gate=1;
    if(!strcmp(mode,"mbmax")) recon_mb_cur_max=2;
    if(!strcmp(mode,"lccp")) recon_lc_codepage=0;
    if(!strcmp(mode,"handle")) recon_lc_handle[2]=0;
    if(!strcmp(mode,"other-handle")) recon_lc_handle[0]=1033;
    if(!strcmp(mode,"mbtable")) recon_mbctype[200]^=1;
    if(!strcmp(mode,"ctype")) recon_pctype['A']^=1;
    if(!strcmp(mode,"oldtable")) old['A']^=1;
    if(!strcmp(mode,"pointer")) recon_pctype=old;
    if(!strcmp(mode,"result")) text[0]='e';
    if(*mode && strcmp(mode,"live") && strcmp(mode,"sentinel")) {recon_mbctolower(value);return 42;}
    if(!strcmp(mode,"live")) __asm__ __volatile__("fld1":::"st");
    errno=!strcmp(mode,"sentinel")?111:0;recon_doserrno=!strcmp(mode,"sentinel")?222:0;
    recon_SetLastError(!strcmp(mode,"sentinel")?0x12345678:errors[2]);
    for(n=0;n<count;++n) {
        save_fp(fp_before);result=recon_mbctolower(inputs[n]);save_fp(fp_after);
        assert(result==outputs[n] && !memcmp(fp_before,fp_after,108));
        assert(errno==(!strcmp(mode,"sentinel")?111:0));
        assert(recon_doserrno==(!strcmp(mode,"sentinel")?222:0));
        assert(recon_GetLastError()==(!strcmp(mode,"sentinel")?0x12345678:errors[2]));
    }
    assert(state->bound==1 && state->calls==1 && state->mb_calls==1 && state->result==text);
    assert(!strcmp(state->reference_sha256,argv[2]) && !strcmp(text,"English_United States.1252"));
    assert(recon_pctype==selected && !memcmp(old-1,old_table,sizeof(old_table)) && !memcmp(selected-1,new_table,sizeof(new_table)));
    assert(!memcmp(recon_mbctype,mb,sizeof(mb)));
    assert(recon_mb_codepage==1252 && !recon_mb_codepage_gate && recon_mb_cur_max==1 && recon_lc_codepage==1252);
    for(n=0;n<6;++n) assert(recon_lc_handle[n]==(n==2?1033:0));
    if(!strcmp(mode,"live")) __asm__ __volatile__("fstp %%st(0)":::"st");
    puts("actual byte-domain lowercase vector, tables/owners/errors/fullFP preserved");return 0;
}
