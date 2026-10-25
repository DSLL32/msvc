#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
unsigned char image_anchor,image_data[36];
static const unsigned char readonly_data[36]={1};
static void __cdecl lifecycle(HMODULE module,unsigned event,void *context){(void)module;(void)event;(void)context;}
static int WINAPI callback(void){return 7;}
void image_test(const char *mode,void *foreign_data,FARPROC foreign_callback)
{
    ReconModuleSpec spec={"test.exe","R:\\test.exe","aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS};
    ReconModuleMemorySpec proof={spec.sha256,0x1000,36,0x10000,0x11000};
    ReconModuleSpec dep={"test.dll","R:\\test.dll","bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",RECON_MODULE_NO_TLS};
    ReconModuleExport entry={"Test",1,(FARPROC)callback};
    ReconModuleExportsSpec exports={dep.sha256,&entry,1,lifecycle,NULL};
    HMODULE module=recon_register_module(&spec),provider;void *address=image_data;
    assert(module);if(!strcmp(mode,"foreign-data"))address=foreign_data;
    if(!strcmp(mode,"heap"))address=malloc(36);
    if(!strcmp(mode,"readonly-data"))address=(void *)readonly_data;
    errno=111;recon_doserrno=222;recon_SetLastError(333);
    recon_register_module_memory(module,&proof,address,36);
    if(!strcmp(mode,"overlap"))recon_register_module_memory(module,&proof,address,36);
    assert(recon_FlushViewOfFile(address,0));
    assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==333);
    provider=recon_register_module(&dep);assert(provider);
    if(!strcmp(mode,"case") || !strcmp(mode,"unbound-case"))entry.address=foreign_callback;
    if(!strcmp(mode,"data-callback"))entry.address=(FARPROC)image_data;
    recon_register_module_exports(provider,&exports);
    assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==333);
}
