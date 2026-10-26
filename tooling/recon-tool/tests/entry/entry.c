#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <recon.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
extern int original_startup(int,char **,char **);
extern int a_ReadCounter(void),b_ReadCounter(void),a_ReadConstructor(void),b_ReadConstructor(void);
extern int __attribute__((stdcall)) a_Export(int,int (__attribute__((stdcall)) *)(int));
extern int __attribute__((stdcall)) b_Export(int,int (__attribute__((stdcall)) *)(int));
extern BOOL WINAPI a_DllEntry(HINSTANCE,DWORD,LPVOID),b_DllEntry(HINSTANCE,DWORD,LPVOID);
extern unsigned char recon_module_a_state_start[],recon_module_a_state_end[];
extern unsigned char recon_module_b_state_start[],recon_module_b_state_end[];
extern const unsigned char recon_asset_exe_catalog_start[],recon_asset_exe_catalog_end[];
extern const unsigned char recon_asset_a_messages_start[],recon_asset_a_messages_end[];
extern char **environ;
int shared_calls,shared_ctor_calls;
static int exit_fd=-1,exit_probe;
void entry_event(int tag,unsigned reason)
{
    char value=tag==1?'a':'b';
    if(exit_probe && reason==DLL_PROCESS_DETACH)assert(write(exit_fd,&value,1)==1);
}
static const char sha[]="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
static int __attribute__((stdcall)) increment(int n) { return n+1; }
static void close_check(void) { assert(shared_calls>4);assert(write(exit_fd,"X",1)==1); }
static HMODULE module(const char *name)
{
    char path[128];ReconModuleSpec s;HMODULE h;
    snprintf(path,sizeof(path),"R:\\entry\\%s",name);
    s.name=name;s.path=path;s.sha256=sha;s.flags=RECON_MODULE_NO_TLS;
    h=recon_register_module(&s);assert(h);return h;
}
static void files(void)
{
    static const unsigned char expected[]={0,1,2,255,'C','L','\r','\n',0,'X'};
    unsigned char data[32];int first,second;
    assert((size_t)(recon_asset_exe_catalog_end-recon_asset_exe_catalog_start)==sizeof(expected));
    assert(!memcmp(recon_asset_exe_catalog_start,expected,sizeof(expected)));
    assert(recon_register_embedded_file("R:\\entry\\catalog.msg",recon_asset_exe_catalog_start,sizeof(expected)));
    assert(recon_register_embedded_file("R:\\entry\\a.msg",recon_asset_a_messages_start,(size_t)(recon_asset_a_messages_end-recon_asset_a_messages_start)));
    assert((size_t)(recon_asset_a_messages_end-recon_asset_a_messages_start)==12);
    assert(!memcmp(recon_asset_a_messages_start,"DLL\0catalog\xff",12));
    first=recon_open("R:\\entry\\catalog.msg",0x8000);second=recon_open("R:\\entry\\catalog.msg",0x8000);
    assert(first>=0 && second>=0 && first!=second);
    assert(recon_read(first,data,3)==3 && !memcmp(data,expected,3));
    assert(recon_read(second,data,sizeof(data))==(int)sizeof(expected) && !memcmp(data,expected,sizeof(expected)));
    assert(recon_lseek(first,0,0)==0 && recon_filelength(first)==(long)sizeof(expected));
    assert(recon_close(first)==0 && recon_close(second)==0);
}
int main(int argc,char **argv,char **environment)
{
    int special=argc==2 && !strcmp(argv[1],"premain");
    HMODULE a,b;ReconModuleWritableRegion ar,br;ReconStaticModuleSpec as,bs;
    ReconModuleExport ae[]={{"Export",1,(FARPROC)(uintptr_t)a_Export},{"Counter",2,(FARPROC)(uintptr_t)a_ReadCounter}};
    ReconModuleExport be[]={{"Export",1,(FARPROC)(uintptr_t)b_Export},{"Counter",2,(FARPROC)(uintptr_t)b_ReadCounter}};
    HMODULE deps[1];char command[4096];int prior;
    assert(environment==environ && argc>0 && argv[argc]==NULL);
    assert(!strcmp(getenv("ENTRY_TOKEN"),"present"));
    snprintf(command,sizeof(command),"%s%s",argv[0],special?" premain":" one two");
    recon_bind_command_line(command,strlen(command)+1);
    recon_crt_bind_arguments(argc,argv,environment);
    assert(recon_crt_arguments_bound && recon_crt_argv==argv && recon_crt_environment==environment);
    assert(recon_GetCommandLineA()==command);
    assert(shared_ctor_calls==(special?1:0));
    exit_fd=open("exit.log",O_CREAT|O_TRUNC|O_WRONLY,0600);assert(exit_fd>=0);
    files();a=module("a.dll");b=module("b.dll");deps[0]=b;
    ar.address=recon_module_a_state_start;ar.bytes=(size_t)(recon_module_a_state_end-recon_module_a_state_start);
    br.address=recon_module_b_state_start;br.bytes=(size_t)(recon_module_b_state_end-recon_module_b_state_start);
    assert(ar.bytes>64 && br.bytes>64 && (uintptr_t)ar.address%64==0 && (uintptr_t)br.address%64==0);
    assert(!((uintptr_t)&shared_calls>=(uintptr_t)ar.address && (uintptr_t)&shared_calls<(uintptr_t)ar.address+ar.bytes));
    assert(!((uintptr_t)&shared_calls>=(uintptr_t)br.address && (uintptr_t)&shared_calls<(uintptr_t)br.address+br.bytes));
    assert((uintptr_t)ar.address+ar.bytes<=(uintptr_t)br.address || (uintptr_t)br.address+br.bytes<=(uintptr_t)ar.address);
    bs.exports=be;bs.count=2;bs.entry=b_DllEntry;bs.dependencies=NULL;bs.dependency_count=0;bs.writable_region=special?NULL:&br;
    as.exports=ae;as.count=2;as.entry=a_DllEntry;as.dependencies=deps;as.dependency_count=1;as.writable_region=&ar;
    assert(recon_configure_static_module(b,&bs));assert(recon_configure_static_module(a,&as));
    assert(recon_LoadLibraryA("a.dll")==a);
    assert(a_ReadCounter()==18 && b_ReadCounter()==(special?24:18));
    assert((uintptr_t)recon_GetProcAddress(a,"Export")== (uintptr_t)a_Export);
    assert((uintptr_t)recon_GetProcAddress(b,(const char *)(uintptr_t)1)==(uintptr_t)b_Export);
    assert(b_Export(1,increment)==(special?26:20));
    assert(recon_GetLastError()==(DWORD)(special?26:20));
    if(!special)assert(original_startup(argc,argv,environment)==0);
    else assert(a_Export(0,increment)==19);
    assert(recon_GetLastError()==(DWORD)(special?19:27));
    assert(recon_LoadLibraryA("a.dll")==a && a_ReadCounter()==(special?19:27));
    assert(recon_FreeLibrary(a));assert(a_ReadCounter()==(special?19:27));
    assert(recon_FreeLibrary(a));assert(a_ReadCounter()==-99 && b_ReadCounter()==-99);
    prior=shared_calls;assert(recon_LoadLibraryA("a.dll")==a);
    assert(a_ReadCounter()==18 && b_ReadCounter()==(special?-98:18));
    assert(shared_calls>prior && a_ReadConstructor()==0 && b_ReadConstructor()==(special?1:0));
    assert(shared_ctor_calls==(special?1:0));
    assert(atexit(close_check)==0);
    exit_probe=1;
    puts(special?"premain without reset PASS":"entry modules assets reset PASS");
    assert(fflush(stdout)==0);
    return 0;
}
