#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <recon.h>
static unsigned descriptors(void)
{
    DIR *directory=opendir("/proc/self/fd");struct dirent *entry;unsigned count=0;
    assert(directory);while((entry=readdir(directory))!=NULL)if(entry->d_name[0]!='.')++count;
    assert(!closedir(directory));return count;
}
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"";HANDLE file,mapping=NULL;unsigned char *view=NULL,bytes[4096];
    ShimFILE *stream;DWORD access=GENERIC_READ,share=FILE_SHARE_READ;int raw,closed=0;
    unsigned n,pass,before_fds=descriptors();unsigned short cw,sw,after_cw,after_sw;
    if(!strcmp(mode,"share-none") || !strcmp(mode,"replace"))share=0;
    if(!strcmp(mode,"share-rw"))share=FILE_SHARE_READ|FILE_SHARE_WRITE;
    if(!strcmp(mode,"writer"))access|=GENERIC_WRITE;
    for(pass=0;pass<(*mode?1:12);++pass) {
        file=recon_CreateFileA("input.bin",access,share,NULL,OPEN_EXISTING,0,NULL);assert(file!=INVALID_HANDLE_VALUE);
        raw=(int)(uintptr_t)file-1;
        if(!strcmp(mode,"unknown") || !strcmp(mode,"reuse") || !strcmp(mode,"identity")) {
            if(!strcmp(mode,"identity"))assert(!close(raw));
            else assert(recon_CloseHandle(file));
            closed=1;
            if(!strcmp(mode,"identity")) {
                assert(open("other.bin",O_RDONLY)==raw);
                assert(!recon_fsopen_read_alias(raw));assert(!close(raw));
            }
            assert(open("input.bin",O_RDONLY)==raw);
        } else if(!strcmp(mode,"replace")) {
            /* Verify a fresh known open replaces facts for this descriptor. */
            assert(recon_CloseHandle(file));
            file=recon_CreateFileA("input.bin",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
            assert((int)(uintptr_t)file-1==raw);assert(!close(raw));
            file=recon_CreateFileA("input.bin",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
            assert((int)(uintptr_t)file-1==raw);
        }
        if(!*mode || !strcmp(mode,"view-only") || !strcmp(mode,"mapping") ||
           !strcmp(mode,"writer") || !strcmp(mode,"unknown-view") || !strcmp(mode,"write-mode")) {
            if(!strcmp(mode,"unknown-view")) {
                assert(recon_CloseHandle(file));assert(open("input.bin",O_RDONLY)==raw);
            }
            mapping=recon_CreateFileMappingA(file,NULL,access&GENERIC_WRITE?PAGE_READWRITE:PAGE_READONLY,0,4096,NULL);assert(mapping);
            view=recon_MapViewOfFile(mapping,access&GENERIC_WRITE?FILE_MAP_WRITE:FILE_MAP_READ,0,0,4096);assert(view);
            if(strcmp(mode,"mapping")) {assert(recon_CloseHandle(mapping));mapping=NULL;}
            if(!strcmp(mode,"view-only") || !strcmp(mode,"unknown-view")) {assert(recon_CloseHandle(file));closed=1;}
        }
        if(!closed)assert(recon_SetFilePointer(file,13,NULL,FILE_BEGIN)==13);
        errno=EDOM;recon_doserrno=222;recon_SetLastError(333);
        __asm__ __volatile__("fld1; fnstcw %0; fnstsw %1":"=m"(cw),"=m"(sw));
        stream=recon_fsopen("input.bin",!strcmp(mode,"write-mode")?"wb":"rb",!strcmp(mode,"write-mode")?0x10:0x20);
        __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(after_cw),"=m"(after_sw));
        assert(stream && cw==after_cw && sw==after_sw && errno==EDOM && recon_doserrno==222 && !recon_GetLastError());
        if(!strcmp(mode,"second-lease"))recon_fsopen("input.bin","rb",0x20);
        if(!strcmp(mode,"late-writer"))recon_CreateFileA("input.bin",GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,0,NULL);
        assert(recon_ftell(stream)==0 && recon_fread(bytes,1,sizeof(bytes),stream)==sizeof(bytes));
        for(n=0;n<sizeof(bytes);++n)assert(bytes[n]==(unsigned char)n && (!view || view[n]==(unsigned char)n));
        if(!closed)assert(recon_SetFilePointer(file,0,NULL,FILE_CURRENT)==13);
        assert(!recon_fclose(stream));
        if(view) {assert(view[257]==1);assert(recon_UnmapViewOfFile(view));view=NULL;}
        if(mapping){assert(recon_CloseHandle(mapping));mapping=NULL;}
        if(!closed)assert(recon_CloseHandle(file));
        assert(!recon_fsopen_read_alias(raw));
        __asm__ __volatile__("fstp %%st(0)":::"st");
    }
    assert(descriptors()==before_fds);return 0;
}
