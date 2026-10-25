#include <assert.h>
#include "../wine/loader-break.h"
int main(void)
{
    unsigned char loader[]={0x64,0xa3,0,0,0,0,0xcc,0x8b,0x45,0xa8,0x64,0xa3,0,0,0,0,0xc9,0xc3};
    unsigned char debugbreak[]={0xcc,0xc3};
    assert(original_loader_break("ntdll.dll",loader,sizeof(loader)));
    assert(original_loader_break("NTDLL.DLL",loader,sizeof(loader)));
    assert(!original_loader_break("other.dll",loader,sizeof(loader)));
    assert(!original_loader_break("ntdll.dll.other",loader,sizeof(loader)));
    assert(!original_loader_break("ntdll.dll",debugbreak,sizeof(debugbreak)));
    assert(!original_loader_break("ntdll.dll",loader,sizeof(loader)-1));
    loader[6]=0x90;
    assert(!original_loader_break("ntdll.dll",loader,sizeof(loader)));
    return 0;
}
