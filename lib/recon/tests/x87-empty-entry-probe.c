#define RECON_SHIM_IMPL
#include <recon.h>
#include <assert.h>
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"good";
    const char *reference="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    unsigned control=0x027f,status=0x20,tag=0xffff;
    unsigned char before[108],after[108],expected[108],environment[28];unsigned short word;
    recon_controlfp(0x10000,0x30000);
    /* Exercise preservation of stale empty-slot payload and native IP/DP. */
    __asm__ __volatile__("fnclex\n\tfld1\n\tfstp %%st(0)\n\tfnsave %0\n\tfrstor %0":"=m"(before));
    if(!strcmp(mode,"control"))control=0x037f;
    if(!strcmp(mode,"status"))status=0;
    if(!strcmp(mode,"top"))status|=0x800;
    if(!strcmp(mode,"tag"))tag=0;
    if(!strcmp(mode,"hash"))reference="bad";
    if(!strcmp(mode,"uppercase"))reference="AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    if(!strcmp(mode,"null"))reference=NULL;
    if(!strcmp(mode,"live"))__asm__ __volatile__("fld1");
    if(!strcmp(mode,"native-control"))recon_controlfp(0x30000,0x30000);
    if(!strcmp(mode,"native-status")){
        __asm__ __volatile__("fnstenv %0":"=m"(environment));
        word=0x20;memcpy(environment+4,&word,2);
        __asm__ __volatile__("fldenv %0"::"m"(environment));
    }
    errno=111;recon_doserrno=77;recon_SetLastError(0x12345678);
    recon_apply_x87_empty_entry_profile(reference,control,status,tag);
    if(!strcmp(mode,"repeat"))recon_apply_x87_empty_entry_profile(reference,control,status,tag);
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(after));
    memcpy(expected,before,108);word=0x20;memcpy(expected+4,&word,2);
    assert(!memcmp(expected,after,108));
    assert(errno==111 && recon_doserrno==77 && recon_GetLastError()==0x12345678);
    return 0;
}
