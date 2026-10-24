#define RECON_SHIM_IMPL
#include <assert.h>
#include <limits.h>
#include <recon.h>
static void save_fp(unsigned char bytes[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes)); }
int main(int argc,char **argv)
{
    unsigned char input[4112],saved[4112],before[108],after[108];
    unsigned long fields[3],rows,n,result;const char *mode;FILE *file;const void *argument;
    assert(sizeof(long)==4 && argc==3);mode=argv[2];memset(input,0xa5,sizeof(input));
    if(!strcmp(mode,"negative")) {recon_mspdb_SigForPbCb(input,-1,0xffffffffUL);return 42;}
    if(!strcmp(mode,"min")) {recon_mspdb_SigForPbCb(input,LONG_MIN,0xffffffffUL);return 42;}
    if(!strcmp(mode,"null")) {recon_mspdb_SigForPbCb(NULL,1,0xffffffffUL);return 42;}
    if(!strcmp(mode,"wrap")) {recon_mspdb_SigForPbCb((void *)(uintptr_t)0xfffffff0UL,32,0xffffffffUL);return 42;}
    if(!strcmp(mode,"zero-poison")) {
        errno=111;recon_doserrno=222;recon_SetLastError(0x12345678);save_fp(before);
        result=recon_mspdb_SigForPbCb((void *)(uintptr_t)1,0,0x12345678);save_fp(after);
        assert(result==0x12345678 && !memcmp(before,after,108));
        assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==0x12345678);
        puts("zero count no read, seed/errors/fullFP preserved");return 0;
    }
    file=fopen(argv[1],"rb");assert(file && fread(&rows,4,1,file)==1 && rows<=1024);
    if(!strcmp(mode,"live"))__asm__ __volatile__("fld1":::"st");
    for(n=0;n<rows;++n) {
        assert(fread(fields,4,3,file)==3 && fields[1]<=4096);
        memset(input,0xa5,sizeof(input));assert(fread(input+8,1,fields[1],file)==fields[1]);memcpy(saved,input,sizeof(input));
        argument=fields[1]?input+8:NULL;
        errno=!strcmp(mode,"live")?111:0;recon_doserrno=!strcmp(mode,"live")?222:0;
        recon_SetLastError(!strcmp(mode,"live")?0x12345678:0);save_fp(before);
        result=recon_mspdb_SigForPbCb(argument,(long)fields[1],fields[0]);save_fp(after);
        assert(result==fields[2] && !memcmp(input,saved,sizeof(input)) && !memcmp(before,after,108));
        assert(errno==(!strcmp(mode,"live")?111:0) && recon_doserrno==(!strcmp(mode,"live")?222:0));
        assert(recon_GetLastError()==(!strcmp(mode,"live")?0x12345678:0));
    }
    assert(fgetc(file)==EOF);fclose(file);
    if(!strcmp(mode,"live"))__asm__ __volatile__("fstp %%st(0)":::"st");
    puts("canonical CRC vectors/input tails/errors/fullFP preserved");return 0;
}
