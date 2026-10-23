#include <assert.h>
#include <recon.h>

int main(void)
{
    ReconModuleSpec main_spec={"program.exe","R:\\program.exe",
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS};
    ReconModuleSpec dll_spec={"compiler.dll","R:\\tools\\compiler.dll",
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
        RECON_MODULE_NO_TLS};
    ReconModuleSpec bad;
    const ReconModuleSpec *stored;
    HMODULE main_module,dll_module;
    char path[64];
    assert(GetModuleHandleA(NULL)==(HMODULE)1); /* Existing unregistered VC1 host. */
    errno=EDOM; SetLastError(77);
    main_module=recon_register_module(&main_spec);
    assert(main_module && GetLastError()==77 && errno==EDOM);
    dll_module=recon_register_module(&dll_spec);
    assert(dll_module && dll_module!=main_module && GetLastError()==77);
    assert(GetModuleHandleA(NULL)==main_module);
    assert(GetModuleHandleA("PROGRAM.EXE")==main_module);
    assert(GetModuleHandleA("r:\\TOOLS\\COMPILER.DLL")==dll_module);
    stored=recon_module_identity(dll_module);
    assert(stored && !strcmp(stored->name,dll_spec.name) && !strcmp(stored->path,dll_spec.path));
    assert(!strcmp(stored->sha256,dll_spec.sha256) && stored->flags==RECON_MODULE_NO_TLS);
    assert(stored!=&dll_spec && stored->name!=dll_spec.name && stored->path!=dll_spec.path);
    memset(path,0xa5,sizeof(path));
    assert(GetModuleFileNameA(dll_module,path,sizeof(path))==strlen(dll_spec.path));
    assert(!strcmp(path,dll_spec.path) && (unsigned char)path[strlen(path)+1]==0xa5);
    assert(!recon_module_thread_calls_disabled(dll_module));
    assert(DisableThreadLibraryCalls(dll_module) && GetLastError()==77 && errno==EDOM);
    assert(recon_module_thread_calls_disabled(dll_module));
    assert(!recon_module_thread_calls_disabled(main_module));
    assert(recon_register_module(&dll_spec)==dll_module);
    assert(recon_module_thread_calls_disabled(dll_module));
    assert(!LoadLibraryA("compiler.dll") && GetLastError()==ERROR_MOD_NOT_FOUND);
    assert(!DisableThreadLibraryCalls((HMODULE)0x12345000) && GetLastError()==ERROR_MOD_NOT_FOUND);
    assert(!recon_module_identity(NULL) && GetLastError()==ERROR_MOD_NOT_FOUND);
    bad=dll_spec; bad.sha256=main_spec.sha256;
    assert(!recon_register_module(&bad) && GetLastError()==183);
    bad=dll_spec; bad.sha256="not-a-hash";
    assert(!recon_register_module(&bad) && GetLastError()==ERROR_INVALID_PARAMETER);
    bad=dll_spec; bad.flags=0;
    assert(!recon_register_module(&bad) && GetLastError()==ERROR_INVALID_PARAMETER);
    bad=dll_spec; bad.flags|=4;
    assert(!recon_register_module(&bad) && GetLastError()==ERROR_INVALID_PARAMETER);
    bad=dll_spec; bad.path="R:\\wrong.dll";
    assert(!recon_register_module(&bad) && GetLastError()==ERROR_INVALID_PARAMETER);
    bad=dll_spec; bad.flags|=RECON_MODULE_PRIMARY;
    assert(!recon_register_module(&bad) && GetLastError()==183);
    assert(GetModuleHandleA(NULL)==main_module);
    assert(GetModuleHandleA("kernel32.dll")== (HMODULE)1);
    return 0;
}
