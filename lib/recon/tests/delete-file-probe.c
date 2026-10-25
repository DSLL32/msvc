#define RECON_SHIM_IMPL
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <recon.h>

static BOOL (WINAPI *delete_file)(LPCSTR)=recon_DeleteFileA;
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static HANDLE create_file(const char *name)
{
    HANDLE file=recon_CreateFileA(name,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    DWORD wrote;
    assert(file!=INVALID_HANDLE_VALUE);
    assert(recon_WriteFile(file,"owned payload",13,&wrote,NULL) && wrote==13);
    assert(recon_SetFilePointer(file,2097152,NULL,FILE_BEGIN)==2097152 && recon_SetEndOfFile(file));
    return file;
}
static void check(const char *name,BOOL success,DWORD error,unsigned row)
{
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,0x12345678}};
    unsigned char before[108],after[108],flags[256],files[sizeof(recon_iob)];
    memcpy(flags,recon_osfile,sizeof(flags));memcpy(files,recon_iob,sizeof(files));
    errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
    fp(before);assert(delete_file(name)==success);fp(after);
    assert(!memcmp(before,after,108));
    assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1]);
    assert(recon_GetLastError()==(success?errors[row][2]:error));
    assert(!memcmp(flags,recon_osfile,sizeof(flags)) && !memcmp(files,recon_iob,sizeof(files)));
}
int main(int argc,char **argv)
{
    HANDLE file,mapping;unsigned char *view;unsigned row;int fd;struct stat information;
    int live=argc>1 && !strcmp(argv[1],"live");
    if(argc>1 && !live) {
        file=create_file("victim.bin");assert(recon_CloseHandle(file));
        if(!strcmp(argv[1],"null")){delete_file(NULL);return 42;}
        if(!strcmp(argv[1],"empty")){delete_file("");return 42;}
        if(!strcmp(argv[1],"wildcard")){delete_file("*.bin");return 42;}
        if(!strcmp(argv[1],"directory")){assert(!mkdir("directory",0700));delete_file("directory");return 42;}
        if(!strcmp(argv[1],"symlink")){assert(!symlink("victim.bin","alias.bin"));delete_file("alias.bin");return 42;}
        if(!strcmp(argv[1],"open")){fd=open("victim.bin",O_RDONLY);assert(fd>=0);delete_file("victim.bin");return 42;}
        if(!strcmp(argv[1],"hardlink")) {
            assert(!link("victim.bin","alias.bin"));fd=open("alias.bin",O_RDONLY);assert(fd>=0);
            delete_file("victim.bin");return 42;
        }
        file=recon_CreateFileA("victim.bin",GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        assert(file!=INVALID_HANDLE_VALUE);
        mapping=recon_CreateFileMappingA(file,NULL,PAGE_READWRITE,0,0,NULL);assert(mapping);
        if(!strcmp(argv[1],"view")) {
            view=recon_MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,0);assert(view);
            assert(recon_CloseHandle(mapping) && recon_CloseHandle(file));
            delete_file("victim.bin");return 42;
        }
        assert(!strcmp(argv[1],"mapping"));assert(recon_CloseHandle(file));
        delete_file("victim.bin");return 42;
    }
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(row=0;row<3;++row) {
        file=create_file("deleted.bin");assert(recon_CloseHandle(file));
        check("deleted.bin",TRUE,0,row);assert(lstat("deleted.bin",&information)<0 && errno==ENOENT);
        check("deleted.bin",FALSE,ERROR_FILE_NOT_FOUND,row);
        check("missing-parent/deleted.bin",FALSE,ERROR_PATH_NOT_FOUND,row);
        file=create_file("readonly.bin");assert(recon_CloseHandle(file) && !chmod("readonly.bin",0400));
        check("readonly.bin",FALSE,ERROR_ACCESS_DENIED,row);
        assert(!stat("readonly.bin",&information) && information.st_size==2097152);
        assert(!chmod("readonly.bin",0600));check("readonly.bin",TRUE,0,row);
    }
    file=create_file("retained.bin");assert(recon_CloseHandle(file));
    check("retained.bin/child.bin",FALSE,ERROR_PATH_NOT_FOUND,2);
    /* Another inode's live view must not block the actual cleanup loop. */
    file=create_file("other.bin");mapping=recon_CreateFileMappingA(file,NULL,PAGE_READWRITE,0,0,NULL);assert(mapping);
    view=recon_MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,0);assert(view && !memcmp(view,"owned payload",13));
    check("retained.bin",TRUE,0,2);assert(!memcmp(view,"owned payload",13));
    view[2097151]=0x5a;
    assert(recon_UnmapViewOfFile(view) && recon_CloseHandle(mapping) && recon_CloseHandle(file));
    fd=open("other.bin",O_RDONLY);assert(fd>=0 && lseek(fd,2097151,SEEK_SET)==2097151);
    {unsigned char byte;assert(read(fd,&byte,1)==1 && byte==0x5a);}assert(!close(fd));
    check("other.bin",TRUE,0,2);
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}
