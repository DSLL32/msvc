#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <windows.h>
#include <recon.h>
typedef char FindSize[sizeof(WIN32_FIND_DATAA)==320?1:-1];
typedef char FindName[offsetof(WIN32_FIND_DATAA,cFileName)==44?1:-1];
typedef char FindAlt[offsetof(WIN32_FIND_DATAA,cAlternateFileName)==304?1:-1];
typedef HANDLE (WINAPI *FirstType)(LPCSTR,LPWIN32_FIND_DATAA);
typedef BOOL (WINAPI *NextType)(HANDLE,LPWIN32_FIND_DATAA);
typedef BOOL (WINAPI *CloseType)(HANDLE);
typedef char FirstSignature[__builtin_types_compatible_p(__typeof__(&FindFirstFileA),FirstType)?1:-1];
typedef char NextSignature[__builtin_types_compatible_p(__typeof__(&FindNextFileA),NextType)?1:-1];
typedef char CloseSignature[__builtin_types_compatible_p(__typeof__(&FindClose),CloseType)?1:-1];
static int descriptor_count(void)
{
    DIR *d=opendir("/proc/self/fd");struct dirent *e;int n=0;assert(d);
    while((e=readdir(d))!=NULL)++n;assert(!closedir(d));return n;
}
int main(int argc,char **argv)
{
    struct {WIN32_FIND_DATAA data;unsigned char tail[16];} first,second;
    FirstType begin=FindFirstFileA;NextType advance=FindNextFileA;CloseType finish=FindClose;
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    (void)argc;(void)argv;(void)first;(void)second;(void)advance;(void)finish;
    if(argc>1 && !strcmp(argv[1],"next"))advance(NULL,&first.data);
    else if(argc>1 && !strcmp(argv[1],"close"))finish(NULL);
    else begin("upda*.lib",&first.data);
    return 42;
#else
    HANDLE a,b;int n,before=descriptor_count(),dirs=0,files=0;unsigned char saved[320];
    char name[MAX_PATH];uint64_t ticks;unsigned short cw,sw,new_cw,new_sw;
    if(argc>1) {
        if(!strcmp(argv[1],"overlap")) {strcpy((char *)&first.data,"upda*.lib");begin((char *)&first.data,&first.data);}
        else if(!strcmp(argv[1],"output"))begin("upda*.lib",(LPWIN32_FIND_DATAA)1);
        else if(!strcmp(argv[1],"parent"))begin("mat*\\*.lib",&first.data);
        else if(!strcmp(argv[1],"drive"))begin("Z:*.lib",&first.data);
        else if(!strcmp(argv[1],"ads"))begin("file:stream",&first.data);
        else if(!strcmp(argv[1],"link"))begin("alias*.lib",&first.data);
        else if(!strcmp(argv[1],"dirlink"))begin("aliasdir\\*.lib",&first.data);
        else begin("pipe*.lib",&first.data);
        return 42;
    }
    memset(&first,0xa5,sizeof(first));memset(&second,0xa5,sizeof(second));
    errno=123;recon_doserrno=456;SetLastError(789);
    __asm__ __volatile__("fld1; fnstcw %0; fnstsw %1":"=m"(cw),"=m"(sw));
    a=begin("UPDA*.LIB",&first.data);
    __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(new_cw),"=m"(new_sw));
    assert(cw==new_cw && sw==new_sw);__asm__ __volatile__("fstp %%st(0)":::"st");
    assert(a!=INVALID_HANDLE_VALUE && !strcmp(first.data.cFileName,"updated.lib"));
    assert(first.data.dwFileAttributes==FILE_ATTRIBUTE_NORMAL && first.data.nFileSizeLow==7 && !first.data.nFileSizeHigh);
    ticks=((uint64_t)first.data.ftLastWriteTime.dwHighDateTime<<32)|first.data.ftLastWriteTime.dwLowDateTime;
    assert(ticks==((uint64_t)1700000120+11644473600ULL)*10000000);
    assert(!first.data.dwReserved0 && !first.data.dwReserved1 && !first.data.cAlternateFileName[0]);
    for(n=0;n<16;++n)assert(first.tail[n]==0xa5);
    assert(errno==123 && recon_doserrno==456 && GetLastError()==789);
    memcpy(saved,&first.data,320);
    assert(!advance(a,&first.data) && GetLastError()==ERROR_NO_MORE_FILES);
    assert(!memcmp(saved,&first.data,320) && errno==123 && recon_doserrno==456);
    assert(!advance(a,&first.data) && GetLastError()==ERROR_NO_MORE_FILES);
    assert(finish(a) && GetLastError()==ERROR_NO_MORE_FILES);
    assert(!finish(a) && GetLastError()==ERROR_INVALID_HANDLE);
    assert(!advance(a,&first.data) && GetLastError()==ERROR_INVALID_HANDLE);
    a=begin("match*.lib",&first.data);b=begin("match*.lib",&second.data);
    assert(a!=INVALID_HANDLE_VALUE && b!=INVALID_HANDLE_VALUE && a!=b);
    assert(!strcmp(first.data.cFileName,second.data.cFileName));strcpy(name,first.data.cFileName);
    do {
        if(first.data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)++dirs;
        else {++files;if(!strcmp(first.data.cFileName,"match2.lib"))assert(first.data.dwFileAttributes&FILE_ATTRIBUTE_READONLY);}
    } while(advance(a,&first.data));
    assert(dirs==1 && files==2 && !strcmp(second.data.cFileName,name));
    assert(finish(a) && finish(b));
    a=begin("sub\\upda*.lib",&first.data);assert(a!=INVALID_HANDLE_VALUE);assert(finish(a));
    memset(&first.data,0xa5,320);memcpy(saved,&first.data,320);
    assert(begin("none*.lib",&first.data)==INVALID_HANDLE_VALUE && GetLastError()==ERROR_FILE_NOT_FOUND);
    assert(!memcmp(saved,&first.data,320));
    assert(begin("missing\\*.lib",&first.data)==INVALID_HANDLE_VALUE && GetLastError()==ERROR_PATH_NOT_FOUND);
    assert(begin("updated.lib\\*.lib",&first.data)==INVALID_HANDLE_VALUE && GetLastError()==ERROR_PATH_NOT_FOUND);
    assert(!finish((HANDLE)1) && GetLastError()==ERROR_INVALID_HANDLE);
    for(n=0;n<32;++n) {a=begin("upda*.lib",&first.data);assert(a!=INVALID_HANDLE_VALUE);assert(finish(a));}
    assert(descriptor_count()==before && errno==123 && recon_doserrno==456);
    return 0;
#endif
}
