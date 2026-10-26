#include <assert.h>
#include <recon.h>
static char output[2048];
static DWORD output_capacity=sizeof(output);
static DWORD format(const char *text,...)
{
    va_list ap; DWORD count;
    va_start(ap,text); count=FormatMessageA(0x400,text,0,0,output,output_capacity,&ap); va_end(ap);
    return count;
}
int main(int argc,char **argv)
{
    ReconModuleSpec spec={"target.exe","S:\\target.exe","62f51b0d705d10426d5b38fc6340859d0bfc829d9d93f27d12a6caa8a2ebe6c6",RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS};
    HMODULE module=recon_register_module(&spec); char text[4096]; DWORD count;
    int mac=argc==4 && !strncmp(argv[2],"mac",3);unsigned char before[108],after[108];
    assert(argc==4 && module); recon_load_module_resources(module,argv[1]);
    FormatMessageA(0xa00,module,mac?4509:10001,0,text,sizeof(text),NULL);
    if (!strcmp(argv[2],"heap")) HeapDestroy((HANDLE)1);
    if (!strcmp(argv[2],"printf")) wsprintfA(output,"x");
    if (!strcmp(argv[2],"unknown")) strcpy(text,"unknown %1 %2!d!");
    if(mac && !strcmp(argv[2],"macshort"))output_capacity=5;
    errno=123;recon_doserrno=234;SetLastError(456);
    __asm__ __volatile__("fld1\n\tfldpi":::"st");
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])before));
    if(mac)count=format(text,!strcmp(argv[2],"macnull")?NULL:!strcmp(argv[2],"macoverlap")?output:"mac-resources.rc",
        atoi(argv[3]),"10","240",!strcmp(argv[2],"machigh")?"\200":"HEXA");
    else count=format(text,argv[2],atoi(argv[3]));
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])after));
    assert(errno==123 && recon_doserrno==234 && GetLastError()==456 && !memcmp(before,after,108));
    __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    _setmode(1,0x8000); fwrite(&count,4,1,stdout); fwrite(output,1,sizeof(output),stdout);
    return 0;
}
