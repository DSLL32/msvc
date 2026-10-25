#include <windows.h>
#include <recon.h>
typedef LPVOID (WINAPI *Reallocate)(HANDLE,DWORD,LPVOID,DWORD);
typedef char Signature[__builtin_types_compatible_p(__typeof__(&HeapReAlloc),Reallocate)?1:-1];
int main(int argc,char **argv)
{
    unsigned char storage[32];Reallocate reallocate=HeapReAlloc;
    (void)argv;
    if(argc==1)return 0;
    if(argc==2)return reallocate(NULL,0,NULL,0)!=NULL;
    if(argc==3)return reallocate((HANDLE)1,9,storage,65544)!=NULL;
    return reallocate((HANDLE)1,0xffffffff,(void *)1,0xffffffff)!=NULL;
}
