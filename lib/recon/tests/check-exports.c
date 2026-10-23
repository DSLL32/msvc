#include <assert.h>
#include <recon.h>
int main(int argc,char **argv)
{
    HMODULE kernel=GetModuleHandleA("kernel32.dll"),user=GetModuleHandleA("user32.dll"),ntdll=GetModuleHandleA("ntdll");
    if(argc>1) {
        if(!strcmp(argv[1],"kernel")) GetProcAddress(kernel,"UnobservedExport");
        if(!strcmp(argv[1],"user")) GetProcAddress(user,"UnobservedExport");
        if(!strcmp(argv[1],"ntdll")) GetProcAddress(ntdll,"UnobservedExport");
        if(!strcmp(argv[1],"ordinal")) GetProcAddress(kernel,(LPCSTR)7);
        if(!strcmp(argv[1],"null")) GetProcAddress(kernel,NULL);
        return 3;
    }
    errno=111;SetLastError(123);
    assert(!GetProcAddress(kernel,"IsTNT") && GetLastError()==127 && errno==111);
    SetLastError(123);
    assert(GetProcAddress(kernel,"CreateFileMappingA")== (FARPROC)recon_CreateFileMappingA);
    assert(GetProcAddress(kernel,"MapViewOfFile")== (FARPROC)recon_MapViewOfFile);
    assert(GetProcAddress(kernel,"UnmapViewOfFile")== (FARPROC)recon_UnmapViewOfFile);
    assert(GetProcAddress(user,"LoadStringA")== (FARPROC)recon_LoadStringA);
    assert(GetProcAddress(ntdll,"LdrProcessRelocationBlock")== (FARPROC)recon_LdrProcessRelocationBlock);
    assert(GetLastError()==123 && errno==111);
    return 0;
}
