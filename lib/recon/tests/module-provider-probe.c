#include <assert.h>
#include <recon.h>
typedef struct Provider { HMODULE module; unsigned loaded,freed,invoked; int damage_errors; } Provider;
static Provider owner;
static int WINAPI invoke(int argc,char **argv,BOOL last)
{
    if(recon_module_provider_state(owner.module)!=RECON_MODULE_PROVIDER_LOADED)
        recon_unsupported("cached callback outside provider lease");
    assert(argc==2 && last==1 && !strcmp(argv[0],"compiler.dll") && !strcmp(argv[1],"input.c"));
    ++owner.invoked;return 7;
}
static void WINAPI abort_pass(int status)
{
    (void)status;recon_unsupported("unobserved abort callback");
}
static void __cdecl lifecycle(HMODULE module,unsigned event,void *context)
{
    Provider *p=context;assert(p==&owner && module==p->module);
    if(event==RECON_MODULE_EVENT_LOAD){
        assert(recon_module_provider_state(module)==RECON_MODULE_PROVIDER_LOADING);
        assert(!p->loaded && !p->freed);++p->loaded;
    }else{
        assert(event==RECON_MODULE_EVENT_FREE && p->loaded==1 && !p->freed && p->invoked==1);
        assert(recon_module_provider_state(module)==RECON_MODULE_PROVIDER_UNLOADING);++p->freed;
    }
    if(p->damage_errors)SetLastError(999);
}
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"";
    ReconModuleSpec identity={"compiler.dll","S:\\dependencies\\compiler.dll",
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",RECON_MODULE_NO_TLS};
    ReconModuleExport exports[2]={{"_InvokeCompilerPass@12",2,(FARPROC)invoke},{"_AbortCompilerPass@4",1,(FARPROC)abort_pass}};
    ReconModuleExportsSpec provider={identity.sha256,exports,2,lifecycle,&owner};
    HMODULE loaded;FARPROC address;char *arguments[3]={"compiler.dll","input.c",NULL};
    int (WINAPI *call)(int,char **,BOOL);
    errno=111;_doserrno=77;SetLastError(12345678);owner.module=recon_register_module(&identity);assert(owner.module);
    if(!strcmp(mode,"hash"))provider.sha256="dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd";
    if(!strcmp(mode,"duplicate-name"))exports[1].name=exports[0].name;
    if(!strcmp(mode,"duplicate-ordinal"))exports[1].ordinal=exports[0].ordinal;
    if(!strcmp(mode,"original-address"))exports[0].address=(FARPROC)0x10612229;
    if(!strcmp(mode,"owner"))owner.module=(HMODULE)1;
    recon_register_module_exports(owner.module,&provider);
    assert(errno==111 && _doserrno==77 && GetLastError()==12345678);
    assert(recon_module_provider_state(owner.module)==RECON_MODULE_PROVIDER_REGISTERED);
    if(!strcmp(mode,"registration"))recon_register_module_exports(owner.module,&provider);
    if(!strcmp(mode,"early-lookup"))GetProcAddress(owner.module,exports[0].name);
    if(!strcmp(mode,"early-getmodule"))GetModuleHandleA(identity.path);
    if(!strcmp(mode,"early-free"))FreeLibrary(owner.module);
    owner.damage_errors=!strcmp(mode,"lifecycle-errors");
    loaded=LoadLibraryA(!strcmp(mode,"path")?"S:\\other\\compiler.dll":identity.path);assert(loaded==owner.module);
    assert(owner.loaded==1 && recon_module_provider_state(loaded)==RECON_MODULE_PROVIDER_LOADED);
    assert(GetModuleHandleA(identity.path)==loaded);
    if(!strcmp(mode,"repeat-load"))LoadLibraryA(identity.path);
    if(!strcmp(mode,"missing-lookup"))FreeLibrary(loaded);
    if(!strcmp(mode,"unknown-name"))GetProcAddress(loaded,"unknown");
    if(!strcmp(mode,"unknown-ordinal"))GetProcAddress(loaded,(LPCSTR)99);
    address=GetProcAddress(loaded,!strcmp(mode,"ordinal")?(LPCSTR)2:exports[0].name);assert(address==(FARPROC)invoke);
    if(!strcmp(mode,"repeat-lookup"))GetProcAddress(loaded,exports[0].name);
    assert(GetProcAddress(loaded,exports[1].name)==(FARPROC)abort_pass);
    call=(int(WINAPI *)(int,char **,BOOL))address;
    assert(call(2,arguments,TRUE)==7 && owner.invoked==1);
    if(!strcmp(mode,"foreign-free"))FreeLibrary((HMODULE)1);
    assert(FreeLibrary(loaded) && owner.freed==1);
    assert(recon_module_provider_state(loaded)==RECON_MODULE_PROVIDER_RELEASED);
    if(!strcmp(mode,"repeat-free"))FreeLibrary(loaded);
    if(!strcmp(mode,"reload"))LoadLibraryA(identity.path);
    if(!strcmp(mode,"late-lookup"))GetProcAddress(loaded,exports[0].name);
    if(!strcmp(mode,"late-getmodule"))GetModuleHandleA(identity.path);
    if(!strcmp(mode,"cached"))call(2,arguments,TRUE);
    assert(errno==111 && _doserrno==77 && GetLastError()==12345678);
    puts("module identity/typed callbacks/exports/single lease/errors exact");return 0;
}
