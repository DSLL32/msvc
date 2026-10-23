#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <recon.h>
static unsigned char image_data[36];
int main(int argc,char **argv)
{
    unsigned n; BOOL result;
#ifdef RECON_NATIVE_TEST
    const char *mode=argc>1?argv[1]:"good";
    ReconModuleSpec spec={"LINK.EXE","R:\\toolchains\\1.0-nt\\bin\\LINK.EXE","3a53166175a2e98179d4da41bb0f3bfafec0ecf78884441736f138c8ab4d3718",RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS};
    ReconModuleMemorySpec proof={spec.sha256,0x5d470,36,0x10000,0x6d470};
    HMODULE module=recon_register_module(&spec); void *base=image_data;
    assert(sizeof(IMAGE_EXPORT_DIRECTORY)==40 && offsetof(IMAGE_EXPORT_DIRECTORY,AddressOfNameOrdinals)==36);
    if(!strcmp(mode,"hash")) proof.sha256="0000000000000000000000000000000000000000000000000000000000000000";
    if(!strcmp(mode,"owner")) module=(HMODULE)2;
    if(!strcmp(mode,"range")) proof.original_rva=0xfffffff0;
    if(!strcmp(mode,"rva-as-va")) proof.original_rva=0x6d470;
    if(!strcmp(mode,"heap")) base=malloc(36);
    if(strcmp(mode,"unregistered")) recon_register_module_memory(module,&proof,base,!strcmp(mode,"size")?35:36);
    if(!strcmp(mode,"overlap")) { proof.original_rva++;proof.original_va++;proof.size--; recon_register_module_memory(module,&proof,image_data+1,35); }
#else
    (void)argc; (void)argv;
#endif
    for(n=0;n<4;++n) {
        image_data[n]=(unsigned char)(n+1);
        errno=(n&2)?111:0; SetLastError((n&1)?0x12345678:0);
#ifdef RECON_NATIVE_TEST
        result=FlushViewOfFile(!strcmp(mode,"base")?image_data+1:image_data,!strcmp(mode,"length")?1:0);
#else
        result=FlushViewOfFile(image_data,0);
#endif
        printf("image %u %d %d %lu\n",n,result,errno,GetLastError());
    }
    return 0;
}
