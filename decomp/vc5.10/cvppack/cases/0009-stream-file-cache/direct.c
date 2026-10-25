/* API-level DIRECT workload. Stop at genuine main before initialization, then call the
 * ordinary unmapped variant. Never resume the parser or claim public CLI reach. */
#include <stdio.h>
#include <string.h>
#include "files.h"
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <recon-native.h>
#endif
#include <recon.h>

#ifdef RECON_ORIGINAL_DIRECT
static ReconOriginalProbe *probe;
#define AT(rva) RECON_ORIGINAL_ADDRESS(probe,rva)
#define files_initialize (*(void (__cdecl *)(int,int,int,int,int,int))AT(0x10a0))
#define open_cached_file (*(int (__cdecl *)(const char *,int,int))AT(0x1310))
#define read_cached_file (*(unsigned (__cdecl *)(int,void *,unsigned))AT(0x1980))
#define write_cached_file (*(unsigned (__cdecl *)(int,const void *,unsigned))AT(0x1cc0))
#define seek_cached_file (*(int (__cdecl *)(int,int,int))AT(0x1910))
#define resize_cached_file (*(int (__cdecl *)(int,unsigned))AT(0x11b0))
#define close_file (*(int (__cdecl *)(int,int))AT(0x1680))
#define tell_cached_file (*(unsigned (__cdecl *)(int))AT(0x1a80))
#define cached_files (*(CachedFile ***)AT(0x1565c))
#define closed_file_count (*(int *)AT(0x12548))
#define mapped_files (*(int *)AT(0x12564))
#endif
#define SOURCE_BYTES 65573U
#define TRUNCATED_BYTES 12033U
static unsigned char buffer[997],verify[997];
static unsigned invoked;
static void finish(int passed)
{
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_finish(probe,passed);
#else
    recon_native_entry_finish(passed);
#endif
}
static void failed(unsigned line)
{
#ifdef RECON_ORIGINAL_DIRECT
    recon_original_emit(probe,"probe-failure.bin",&line,sizeof(line));
#else
    fprintf(stderr,"file-cache probe line%u\n",line);
#endif
    finish(0);
}
#define CHECK(x) do{if(!(x)){failed(__LINE__);return;}}while(0)
static unsigned char recipe(unsigned position,unsigned seed)
{ return (unsigned char)(29*position+7*(position>>8)+(position>>12)+seed); }
static int seed_file(const char *name,unsigned seed)
{
    HANDLE file=CreateFileA(name,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    unsigned at,count,i;DWORD written;
    if(file==INVALID_HANDLE_VALUE)return 0;
    for(at=0;at<SOURCE_BYTES;at+=count) {
        count=SOURCE_BYTES-at;if(count>sizeof(buffer))count=sizeof(buffer);
        for(i=0;i<count;++i)buffer[i]=recipe(at+i,seed);
        if(!WriteFile(file,buffer,count,&written,NULL) || written!=count){CloseHandle(file);return 0;}
    }
    return CloseHandle(file)!=0;
}
static void __cdecl at_main(int argc,char **argv,char **environment)
{
    int source,copy,reused,other;unsigned at,count,i;CachedFile *owner;
    (void)environment;
    CHECK(!invoked++ && argc==3 && argv && argv[0] && argv[0][0] &&
          argv[1] && !strcmp(argv[1],"/nologo") &&
          argv[2] && !strcmp(argv[2],"unused.bin") && !argv[3]);
    /* Original main404691..40469d normally pushes16,16,2,16,2,1.
     * This DIRECT invocation chooses its unmapped API variant before that call. */
    CHECK(seed_file("source-a.bin",0) && seed_file("source-b.bin",113));
    files_initialize(2,2,2,2,2,0);
    CHECK(!mapped_files && !closed_file_count);
    source=open_cached_file("source-a.bin",0x8000,0);
    copy=open_cached_file("copied.bin",0x8302,0600);
    CHECK(source>=0 && copy>=0 && source!=copy && !(cached_files[source]->flags&8) &&
          !(cached_files[copy]->flags&8));
    for(at=0;at<SOURCE_BYTES;at+=count) {
        count=SOURCE_BYTES-at;if(count>sizeof(buffer))count=sizeof(buffer);
        CHECK(read_cached_file(source,buffer,count)==count);
        for(i=0;i<count;++i)CHECK(buffer[i]==recipe(at+i,0));
        CHECK(write_cached_file(copy,buffer,count)==count);
    }
    CHECK(tell_cached_file(source)==SOURCE_BYTES && tell_cached_file(copy)==SOURCE_BYTES);
    CHECK(close_file(source,1)==0 && close_file(copy,1)==0);
    source=open_cached_file("source-a.bin",0x8000,0);
    CHECK(source>=0);owner=cached_files[source];
    CHECK(seek_cached_file(source,4093,0)==4093 && read_cached_file(source,verify,64)==64);
    for(i=0;i<64;++i)CHECK(verify[i]==recipe(4093+i,0));
    CHECK(close_file(source,0)==0 && closed_file_count==1);
    reused=open_cached_file("source-a.bin",0x8000,0);
    CHECK(reused==source && cached_files[reused]==owner && !closed_file_count && close_file(reused,0)==0);
    other=open_cached_file("source-b.bin",0x8000,0);
    CHECK(other>=0 && read_cached_file(other,verify,64)==64);
    for(i=0;i<64;++i)CHECK(verify[i]==recipe(i,113));
    CHECK(close_file(other,0)==0 && closed_file_count==1);
    copy=open_cached_file("copied.bin",0x8002,0);
    CHECK(copy>=0 && closed_file_count==1);
    for(at=0;at<SOURCE_BYTES;at+=count) {
        count=SOURCE_BYTES-at;if(count>sizeof(verify))count=sizeof(verify);
        CHECK(read_cached_file(copy,verify,count)==count);
        for(i=0;i<count;++i)CHECK(verify[i]==recipe(at+i,0));
    }
    CHECK(resize_cached_file(copy,TRUNCATED_BYTES)==0 && seek_cached_file(copy,4093,0)==4093 &&
          read_cached_file(copy,verify,64)==64);
    for(i=0;i<64;++i)CHECK(verify[i]==recipe(4093+i,0));
    CHECK(close_file(copy,1)==0);
    finish(1);
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context)
{
    const int *args;
    probe=context;
    if(!probe || probe->abi_version!=RECON_ORIGINAL_PROBE_VERSION ||
       probe->struct_size!=sizeof(*probe) || probe->marker_rva!=0x4620){finish(0);return;}
    args=(const int *)probe->original_esp;
    at_main(args[1],(char **)args[2],(char **)args[3]);
    finish(0);
}
#else
static void invoke(void *unused)
{
    char *args[]={"CVPACK.EXE","/nologo","unused.bin",NULL};
    (void)unused;main(3,args,(char **)0);
}
int cvpack_cache_direct(void)
{
    return recon_native_at_entry((const void *)main,(const void *)at_main,invoke,NULL)==1;
}
#endif
