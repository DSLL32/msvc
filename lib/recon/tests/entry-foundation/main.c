#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <recon.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

extern void entry_reset_checks(void);
extern HMODULE entry_user_a(const char *),entry_user_b(const char *);
static const char identity[]="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
static char events[256];static unsigned event_count;
static int exit_events;
static pthread_barrier_t barrier;
static void event(char value)
{assert(event_count<sizeof(events));events[event_count++]=value;if(exit_events)assert(write(1,&value,1)==1);}
static int __cdecl plus_one(int n) {return n+1;}
static int WINAPI twice(int n) {return n*2;}
static BOOL WINAPI entry_b(HINSTANCE module,DWORD reason,LPVOID reserved)
{(void)module;(void)reserved;event(reason==DLL_PROCESS_ATTACH?'B':'b');return TRUE;}
static BOOL WINAPI entry_a(HINSTANCE module,DWORD reason,LPVOID reserved)
{
    assert(recon_GetModuleHandleA("alpha.dll")==module);
    if(reason==DLL_PROCESS_ATTACH)event(reserved?'S':'A');
    else {event(reserved?'T':'a');if(exit_events && reserved)recon_fputs("D",&recon_iob[1]);}
    return TRUE;
}
static BOOL WINAPI entry_failure(HINSTANCE module,DWORD reason,LPVOID reserved)
{(void)module;(void)reserved;event(reason==DLL_PROCESS_ATTACH?'F':'f');return reason!=DLL_PROCESS_ATTACH;}
static HMODULE register_module(const char *name,const char *path)
{
    ReconModuleSpec spec={name,path,identity,RECON_MODULE_NO_TLS};
    HMODULE result=recon_register_module(&spec);assert(result);return result;
}
static void configure(HMODULE module,ReconDllEntry entry,const HMODULE *dependencies,unsigned n)
{
    ReconModuleExport exports[]={{"AddOne",1,(FARPROC)(uintptr_t)plus_one},{"Twice",2,(FARPROC)(uintptr_t)twice},{NULL,3,(FARPROC)(uintptr_t)plus_one}};
    ReconStaticModuleSpec spec={exports,3,entry,dependencies,n,NULL};
    assert(recon_configure_static_module(module,&spec));
}
static void *thread_error(void *argument)
{
    DWORD value=(DWORD)(uintptr_t)argument;recon_SetLastError(value);
    pthread_barrier_wait(&barrier);assert(recon_GetLastError()==value);return NULL;
}
static void *thread_module(void *argument)
{
    HMODULE handle=argument?entry_user_a("alpha.dll"):entry_user_b("alpha.dll");
    pthread_barrier_wait(&barrier);assert(recon_FreeLibrary(handle));return NULL;
}
static void __cdecl observer_lifecycle(HMODULE module,unsigned event,void *context)
{(void)module;(void)event;(void)context;}
static void observer_policy(void)
{
    HMODULE lease=register_module("lease.dll","C:\\apps\\lease.dll");
    HMODULE ordinary=register_module("ordinary.dll","C:\\apps\\ordinary.dll");
    ReconModuleExport exports={"One",1,(FARPROC)(uintptr_t)plus_one};
    ReconModuleExportsSpec observer={identity,&exports,1,observer_lifecycle,NULL};
    ReconStaticModuleSpec configured={&exports,1,NULL,NULL,0,NULL};
    recon_register_module_exports(lease,&observer);
    assert(!recon_configure_static_module(ordinary,&configured));
    assert(recon_GetLastError()==ERROR_INVALID_PARAMETER);
    puts("policy separation PASS");
}
static void modules(void)
{
    HMODULE a=register_module("alpha.dll","C:\\apps\\alpha.dll");
    HMODULE b=register_module("beta.dll","C:\\apps\\beta.dll");
    HMODULE f=register_module("failure.dll","C:\\apps\\failure.dll"),x,y;
    pthread_t first,second;struct recon_stat info;
    configure(b,entry_b,NULL,0);configure(a,entry_a,&b,1);configure(f,entry_failure,&b,1);
    {
        pid_t child=fork();int status;assert(child>=0);
        if(!child) {
            ReconModuleExport exports={"One",1,(FARPROC)(uintptr_t)plus_one};
            ReconModuleExportsSpec observer={identity,&exports,1,observer_lifecycle,NULL};
            HMODULE lease=register_module("lease.dll","C:\\apps\\lease.dll");
            int quiet=open("/dev/null",O_WRONLY);assert(quiet>=0 && dup2(quiet,2)>=0);close(quiet);
            recon_register_module_exports(lease,&observer);_exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
    }

    assert(!recon_GetModuleHandleA("alpha.dll") && recon_GetLastError()==ERROR_MOD_NOT_FOUND);
    assert(!recon_access("C:\\apps\\ALPHA.DLL",0));
    assert(!recon_stat("C:\\apps\\alpha.dll",&info));
    assert(recon_GetFileAttributesA("C:/apps/alpha.dll")&FILE_ATTRIBUTE_READONLY);
    assert(!recon_LoadLibraryA("C:/elsewhere/alpha.dll") && recon_GetLastError()==ERROR_MOD_NOT_FOUND);
    x=entry_user_a("alpha");y=entry_user_b("C:/apps/ALPHA.dll");
    assert(x==a && x==y && event_count==2 && !memcmp(events,"BA",2));
    assert(recon_GetProcAddress(a,"AddOne")==recon_GetProcAddress(a,(char *)1));
    assert(recon_GetProcAddress(a,(char *)3)==(FARPROC)(uintptr_t)plus_one);
    assert(!recon_GetProcAddress(a,(char *)99) && recon_GetLastError()==ERROR_PROC_NOT_FOUND);
    assert(!recon_GetProcAddress((HMODULE)99,"AddOne") && recon_GetLastError()==ERROR_INVALID_HANDLE);
    assert(!recon_FreeLibrary((HMODULE)99) && recon_GetLastError()==ERROR_INVALID_HANDLE);
    assert(!recon_GetProcAddress(a,"addone") && recon_GetLastError()==ERROR_PROC_NOT_FOUND);
    assert(!recon_GetProcAddress(a,NULL) && recon_GetLastError()==ERROR_PROC_NOT_FOUND);
    assert(!recon_LoadLibraryA("missing.dll") && recon_GetLastError()==ERROR_MOD_NOT_FOUND);
    assert(recon_FreeLibrary(x) && event_count==2);
    assert(recon_FreeLibrary(y) && event_count==4 && !memcmp(events,"BAab",4));
    assert(!recon_GetProcAddress(a,"AddOne") && recon_GetLastError()==ERROR_INVALID_HANDLE);
    assert(!recon_FreeLibrary(a) && recon_GetLastError()==ERROR_INVALID_HANDLE);
    event_count=0;assert(!recon_LoadLibraryA("failure.dll"));
    assert(recon_GetLastError()==ERROR_DLL_INIT_FAILED && event_count==4 && !memcmp(events,"BFfb",4));
    assert(!recon_GetModuleHandleA("beta.dll"));
    event_count=0;assert(!pthread_barrier_init(&barrier,NULL,2));
    assert(!pthread_create(&first,NULL,thread_module,(void *)1));
    assert(!pthread_create(&second,NULL,thread_module,NULL));
    assert(!pthread_join(first,NULL) && !pthread_join(second,NULL));pthread_barrier_destroy(&barrier);
    assert(event_count==4 && !memcmp(events,"BAab",4));
    event_count=0;assert(recon_attach_static_module(a));
    assert(event_count==2 && !memcmp(events,"BS",2));
    assert(recon_attach_static_module(a) && event_count==2);
    x=recon_LoadLibraryA("alpha.dll");assert(x==a && recon_FreeLibrary(x) && event_count==2);
    recon_detach_process_modules();assert(event_count==4 && !memcmp(events,"BSTb",4));
    recon_detach_process_modules();assert(event_count==4);
    recon_SetLastError(123);assert(!pthread_barrier_init(&barrier,NULL,3));
    assert(!pthread_create(&first,NULL,thread_error,(void *)456));
    assert(!pthread_create(&second,NULL,thread_error,(void *)789));
    pthread_barrier_wait(&barrier);assert(recon_GetLastError()==123);
    assert(!pthread_join(first,NULL) && !pthread_join(second,NULL));pthread_barrier_destroy(&barrier);
    assert(recon_GetLastError()==123);
}
static void app_exit_callback(void) {event('X');}
static int termination(const char *mode)
{
    HMODULE a=register_module("alpha.dll","Z:\\apps\\alpha.dll");
    HMODULE b=register_module("beta.dll","Z:\\apps\\beta.dll");
    configure(b,entry_b,NULL,0);configure(a,entry_a,&b,1);
    exit_events=1;assert(!atexit(app_exit_callback));
    if(!strcmp(mode,"dynamic-return"))assert(recon_LoadLibraryA("alpha.dll")==a);
    else assert(recon_attach_static_module(a));
    if(!strcmp(mode,"app-exit"))recon_exit(7);
    if(!strcmp(mode,"exit-process"))recon_ExitProcess(9);
    return 0;
}
static void files(void)
{
    static const char bytes[]="A\r\nB\032TAIL\n";
    const char *name="C:\\apps\\CL.ERR";char buffer[32];DWORD got;HANDLE one,two;
    struct recon_stat info;int fd,other,raw;ShimFILE *stream;
    assert(!setenv("RECON_CANONICAL_CWD","C:\\apps",1));
    assert(recon_register_embedded_file(name,bytes,sizeof(bytes)-1));
    assert(!recon_register_embedded_file("c:/APPS/./cl.err",bytes,sizeof(bytes)-1));
    assert(recon_GetLastError()==ERROR_ALREADY_EXISTS);
    assert(!recon_access("sub/../cl.err",0) && !recon_access(name,4));
    assert(recon_access(name,2)==-1 && errno==EACCES);
    assert(!recon_stat(name,&info) && info.st_size==sizeof(bytes)-1 && (info.st_mode&0222)==0);
    assert(recon_GetFileAttributesA(name)==FILE_ATTRIBUTE_READONLY);
    one=recon_CreateFileA(name,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    two=recon_CreateFileA("c:/apps/cl.err",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
    assert(one!=INVALID_HANDLE_VALUE && two!=INVALID_HANDLE_VALUE && one!=two);
    assert(recon_GetFileSize(one,NULL)==sizeof(bytes)-1);
    assert(recon_ReadFile(one,buffer,3,&got,NULL) && got==3 && !memcmp(buffer,"A\r\n",3));
    assert(recon_ReadFile(two,buffer,1,&got,NULL) && got==1 && buffer[0]=='A');
    assert(recon_SetFilePointer(one,0,NULL,FILE_BEGIN)==0);
    assert(recon_ReadFile(one,buffer,sizeof(buffer),&got,NULL) && got==sizeof(bytes)-1);
    assert(recon_ReadFile(one,buffer,1,&got,NULL) && got==0);
    raw=(int)(uintptr_t)one-1;
    assert((fcntl(raw,F_GET_SEALS)&(F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK))==(F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK));
    assert(write(raw,"x",1)==-1 && errno==EBADF);
    assert(!recon_WriteFile(one,"x",1,&got,NULL));
    {
        HANDLE mapping=recon_CreateFileMappingA(one,NULL,PAGE_READONLY,0,0,NULL);
        const void *view;assert(mapping);
        view=recon_MapViewOfFile(mapping,FILE_MAP_READ,0,0,sizeof(bytes)-1);
        assert(view && !memcmp(view,bytes,sizeof(bytes)-1));
        assert(recon_UnmapViewOfFile(view) && recon_CloseHandle(mapping));
    }
    assert(recon_CloseHandle(one) && recon_CloseHandle(two));
    assert(recon_CreateFileA(name,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL)==INVALID_HANDLE_VALUE);
    assert(recon_GetLastError()==ERROR_ACCESS_DENIED);
    assert(recon_CreateFileA(name,GENERIC_READ,0,NULL,CREATE_NEW,0,NULL)==INVALID_HANDLE_VALUE);
    assert(recon_GetLastError()==ERROR_FILE_EXISTS);
    assert(!recon_DeleteFileA(name) && recon_GetLastError()==ERROR_ACCESS_DENIED);
    assert(recon_remove(name)==-1 && recon_rename(name,"other.err")==-1);
    assert(recon_rename("other.err",name)==-1);
    fd=recon_open(name,0x8000);other=recon_open(name,0x8000);assert(fd>=0 && other>=0);
    assert(recon_read(fd,buffer,3)==3 && recon_read(other,buffer,1)==1 && buffer[0]=='A');
    assert(!recon_fstat(fd,&info) && info.st_size==sizeof(bytes)-1);
    assert(recon_close(fd)==0 && recon_close(other)==0);
    stream=recon_fopen(name,"rt");assert(stream);
    assert(recon_fgetc(stream)=='A' && recon_fgetc(stream)=='\n' && recon_fgetc(stream)=='B');
    assert(recon_fgetc(stream)==EOF);assert(recon_fseek(stream,0,SEEK_SET)==0);
    assert(recon_fgetc(stream)=='A' && recon_fclose(stream)==0);
    stream=recon_fopen(name,"rb");assert(stream);
    assert(recon_fread(buffer,1,sizeof(buffer),stream)==sizeof(bytes)-1 && !memcmp(buffer,bytes,sizeof(bytes)-1));
    assert(recon_fclose(stream)==0);
    assert(!recon_fopen(name,"wb"));
    fd=open("ordinary-input.txt",O_RDWR|O_CREAT|O_TRUNC,0600);assert(fd>=0);
    assert(write(fd,"disk",4)==4);close(fd);
    fd=recon_open("ordinary-input.txt",0x8000);assert(fd>=0 && recon_read(fd,buffer,4)==4 && !memcmp(buffer,"disk",4));
    assert(!recon_close(fd) && !recon_remove("ordinary-input.txt"));
    assert(recon_register_embedded_file("C:/apps/empty",NULL,0));
    one=recon_CreateFileA("C:/apps/empty",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
    assert(one!=INVALID_HANDLE_VALUE && !recon_GetFileSize(one,NULL) && recon_CloseHandle(one));
    puts("files/text/binary/positions/seals/probes/fallback PASS");
}
static void resources(void)
{
    static unsigned char bytes[128],other[128];DWORD count=1,kind=1,id=42,language=0x409,length=5;
    ReconModuleSpec spec={"app.exe","C:\\apps\\app.exe",identity,RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS};
    HMODULE module=recon_register_module(&spec);char output[16];assert(module);
    memcpy(bytes,"RMR1",4);memcpy(bytes+4,&count,4);memcpy(bytes+8,identity,64);
    memcpy(bytes+72,&kind,4);memcpy(bytes+76,&id,4);memcpy(bytes+80,&language,4);memcpy(bytes+84,&length,4);memcpy(bytes+88,"Hello",5);
    recon_load_module_resources_span(module,bytes,93);
    assert(recon_LoadStringA(module,42,output,sizeof(output))==5 && !strcmp(output,"Hello"));
    /* The path loader also recognizes an embedded container despite POSIX IO. */
    memcpy(other,bytes,93);id=43;memcpy(other+76,&id,4);memcpy(other+88,"Other",5);
    assert(recon_register_embedded_file("C:/apps/messages.rmr",other,93));
    recon_load_module_resources(module,"C:/apps/messages.rmr");
    assert(recon_LoadStringA(module,43,output,sizeof(output))==5 && !strcmp(output,"Other"));
    assert(recon_LoadStringA(module,42,output,sizeof(output))==5 && !strcmp(output,"Hello"));
    {
        unsigned i;for(i=0;i<4;++i) {
            int status;pid_t child=fork();assert(child>=0);
            if(!child) {
                unsigned char bad[128];int quiet=open("/dev/null",O_WRONLY);
                assert(quiet>=0 && dup2(quiet,2)>=0);close(quiet);
                memcpy(bad,bytes,93);id=99;memcpy(bad+76,&id,4);
                if(i==0)bad[8]='X';
                if(i==1)bad[88]=0x80;
                if(i==2)bad[80]=0;
                recon_load_module_resources_span(module,bad,i==3?92:93);_exit(42);
            }
            assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        }
    }
    puts("resource span/embedded-container identity+existing parser PASS");
}
int main(int argc,char **argv)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    assert(recon_fmode==0); /* Chosen canonical VC5 shared-runtime initialization. */
#endif
    if(argc>1 && !strcmp(argv[1],"reset")) {entry_reset_checks();return 0;}
    if(argc>1 && !strcmp(argv[1],"legacy-exit-process")) {
        recon_fputs("buffered",&recon_iob[1]);recon_ExitProcess(3);
    }
    if(argc>1 && !strcmp(argv[1],"policy-isolation")) {observer_policy();return 0;}
    if(argc>1)return termination(argv[1]);
    files();resources();modules();puts("module users/refs/dependencies/failure/static/TLS PASS");return 0;
}
