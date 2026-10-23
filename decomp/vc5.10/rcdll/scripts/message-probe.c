/* Explicit original API observation for the two help resources. Not a fixture. */
#include <windows.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    HMODULE module;char path[MAX_PATH],name[80],buffer[4096];
    DWORD ids[2]={10001,20001},languages[2]={0,0x409},sentinels[2]={0,0x12345678};
    unsigned i,j,k;DWORD length,error;FILE *file;
    module=LoadLibraryA("RCDLL.DLL");if(!module)return 125;
    GetModuleFileNameA(module,path,sizeof(path));printf("target %s\n",path);
    GetModuleFileNameA(GetModuleHandleA("MSVCRT.dll"),path,sizeof(path));printf("crt %s\n",path);
    for(i=0;i<2;++i)for(j=0;j<2;++j)for(k=0;k<2;++k){
        memset(buffer,0xa5,sizeof(buffer));
        SetLastError(sentinels[k]);
        length=FormatMessageA(0xa00,module,ids[i],languages[j],buffer,sizeof(buffer),NULL);
        error=GetLastError();
        printf("row %lu %lu %lu %lu %lu\n",ids[i],languages[j],sentinels[k],length,error);
        sprintf(name,"message-%lu-%lu-%lu.bin",ids[i],languages[j],sentinels[k]);
        file=fopen(name,"wb");if(!file)return 126;
        if(fwrite(buffer,1,sizeof(buffer),file)!=sizeof(buffer))return 127;
        fclose(file);
    }
    if(!FreeLibrary(module))return 128;
    return 0;
}
