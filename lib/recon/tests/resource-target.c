#include <assert.h>
#include <recon.h>
int main(int argc,char **argv)
{
    ReconModuleSpec spec={"target.exe","S:\\target.exe","62f51b0d705d10426d5b38fc6340859d0bfc829d9d93f27d12a6caa8a2ebe6c6",RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS};
    HMODULE module=recon_register_module(&spec); char buffer[4096]; int result; DWORD id;
    assert(argc==5 && module); recon_load_module_resources(module,argv[1]);
    id=(DWORD)strtoul(argv[3],NULL,0); memset(buffer,0xa5,sizeof(buffer));
    errno=123; SetLastError(0x12345678);
    if (!strcmp(argv[2],"badflags")) return FormatMessageA(0,module,id,0,buffer,4096,NULL);
    if (!strcmp(argv[2],"badlang")) return FormatMessageA(0xa00,module,id,1,buffer,4096,NULL);
    if (!strcmp(argv[2],"badmodule")) return LoadStringA((HINSTANCE)99,id,buffer,4096);
    if (!strcmp(argv[2],"string")) {
        assert(LoadLibraryA("USER32.DLL")==GetModuleHandleA("user32.dll"));
        assert(GetProcAddress(LoadLibraryA("USER32.DLL"),"LoadStringA")== (FARPROC)recon_LoadStringA);
        result=LoadStringA(NULL,id,buffer,atoi(argv[4]));
    } else result=(int)FormatMessageA(0xa00,module,id,0,buffer,atoi(argv[4]),NULL);
    assert(errno==123 && GetLastError()==0x12345678);
    _setmode(1,0x8000);
    fwrite(&result,4,1,stdout); fwrite(buffer,1,sizeof(buffer),stdout);
    return 0;
}
