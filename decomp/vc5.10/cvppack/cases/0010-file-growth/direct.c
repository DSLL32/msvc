/* Internal API DIRECT: live mapped and stream owners, no parser continuation. */
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
#define prepare_cached_file (*(CachedFile * (__cdecl *)(const char *,int,int,int *))AT(0x1380))
#define open_stream_file (*(int (__cdecl *)(CachedFile *,int,int,int))AT(0x1510))
#define cached_files (*(CachedFile ***)AT(0x1565c))
#define closed_file_count (*(int *)AT(0x12548))
#define mapped_files (*(int *)AT(0x12564))
#endif

#define SOURCE_BYTES (4U*1024U*1024U+8193U)
#define TRUNCATED_BYTES (2U*1024U*1024U+37U)
static unsigned char buffer[997];
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
    fprintf(stderr,"file-growth probe line%u\n",line);
#endif
    finish(0);
}
#define CHECK(x) do{if(!(x)){failed(__LINE__);return;}}while(0)
static unsigned char recipe(unsigned position)
{ return (unsigned char)(29*position+7*(position>>8)+(position>>12)+71); }
static void __cdecl at_main(int argc,char **argv,char **environment)
{
    int mapped,stream,created=0;unsigned at,count,index;
    CachedFile *owner;
    (void)environment;
    CHECK(!invoked++ && argc==3 && argv && argv[0] && argv[0][0] &&
          argv[1] && !strcmp(argv[1],"/nologo") &&
          argv[2] && !strcmp(argv[2],"unused.bin") && !argv[3]);
    files_initialize(2,2,2,2,2,1);
    mapped=open_cached_file("mapped.bin",0x8302,0600);
    CHECK(mapped>=0 && (cached_files[mapped]->flags&8));
    owner=prepare_cached_file("stream.bin",0x8302,0600,&created);
    CHECK(owner && created==1 && owner->index!=mapped);
    stream=owner->index;
    CHECK(open_stream_file(owner,0x8302,0600,created)==0 && !(owner->flags&8));
    for(at=0;at<SOURCE_BYTES;at+=count) {
        count=SOURCE_BYTES-at;if(count>sizeof(buffer))count=sizeof(buffer);
        for(index=0;index<count;++index)buffer[index]=recipe(at+index);
        CHECK(write_cached_file(mapped,buffer,count)==count);
        CHECK(write_cached_file(stream,buffer,count)==count);
    }
    CHECK(tell_cached_file(mapped)==SOURCE_BYTES && tell_cached_file(stream)==SOURCE_BYTES);
    CHECK(seek_cached_file(mapped,0,0)==0 && seek_cached_file(stream,0,0)==0);
    for(at=0;at<SOURCE_BYTES;at+=count) {
        count=SOURCE_BYTES-at;if(count>sizeof(buffer))count=sizeof(buffer);
        CHECK(read_cached_file(mapped,buffer,count)==count);
        for(index=0;index<count;++index)CHECK(buffer[index]==recipe(at+index));
        CHECK(read_cached_file(stream,buffer,count)==count);
        for(index=0;index<count;++index)CHECK(buffer[index]==recipe(at+index));
    }
    CHECK(cached_files[mapped]->capacity>=SOURCE_BYTES && owner->page_capacity>SOURCE_BYTES);
    CHECK(resize_cached_file(mapped,TRUNCATED_BYTES)==0 && resize_cached_file(stream,TRUNCATED_BYTES)==0);
    CHECK(seek_cached_file(mapped,4093,0)==4093 && seek_cached_file(stream,4093,0)==4093);
    CHECK(read_cached_file(mapped,buffer,64)==64);
    for(index=0;index<64;++index)CHECK(buffer[index]==recipe(4093+index));
    CHECK(read_cached_file(stream,buffer,64)==64);
    for(index=0;index<64;++index)CHECK(buffer[index]==recipe(4093+index));
    CHECK(close_file(mapped,1)!=0 && close_file(stream,1)==0);
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
int cvpack_growth_direct(void)
{
    return recon_native_at_entry((const void *)main,(const void *)at_main,invoke,NULL)==1;
}
#endif
