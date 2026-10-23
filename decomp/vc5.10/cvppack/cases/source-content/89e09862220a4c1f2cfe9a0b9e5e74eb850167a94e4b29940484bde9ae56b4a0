#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <recon.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <elf.h>

/* An explicitly configured fixture broker can observe native children or replay
 * an external tool at its process boundary. No broker means ordinary spawning;
 * neither path has an original-executable or Wine fallback. */
static void process_error(const char *message)
{
    dprintf(2,"recon process: %s\n",message);
    _exit(125);
}
typedef struct SpawnState { int error;unsigned long dos;DWORD last; } SpawnState;
static SpawnState spawn_state(void)
{
    SpawnState state;state.error=errno;state.dos=recon_doserrno;state.last=recon_GetLastError();return state;
}
static void restore_spawn_state(SpawnState state)
{ errno=state.error;recon_doserrno=state.dos;recon_SetLastError(state.last); }
/* NULL envp becomes NULL lpEnvironment in the canonical CreateProcess call.
 * Its inherited namespace is the OS block, independently of CRT _environ. */
static const char *const *spawn_environment(const char *const *explicit_env,char ***owned,unsigned *source)
{
    const WCHAR *wide;char **vector,*bytes;size_t n,count=0,units;
    *owned=NULL;
    if(explicit_env){*source=1;return explicit_env;}
    if(!recon_crt_arguments_bound){*source=0;return (const char *const *)environ;}
    wide=recon_crt_os_environment;units=recon_crt_os_environment_units;
    if(!wide || units<3 || units>65536 || !wide[0] || wide[units-1] || wide[units-2])
        process_error("bound spawn requires target OS environment");
    for(n=0;n<units;++n){
        if(wide[n]>=128)process_error("nonASCII target OS spawn environment");
        if(!wide[n]){
            if(n==units-1)break;
            if(!wide[n+1] && n!=units-2)process_error("early OS environment terminator");
            ++count;
        }
    }
    vector=malloc((count+1)*sizeof(*vector)+units);
    if(!vector)process_error("spawn environment allocation failure");
    bytes=(char*)(vector+count+1);count=0;vector[count++]=bytes;
    for(n=0;n<units;++n){bytes[n]=(char)wide[n];if(!wide[n] && n<units-2)vector[count++]=bytes+n+1;}
    vector[count]=NULL;*owned=vector;*source=2;return (const char *const *)vector;
}
static void release_spawn_environment(char **owned)
{
    SpawnState state=spawn_state();free(owned);restore_spawn_state(state);
}
static void socket_write(int fd,const void *data,size_t size)
{
    size_t offset=0;
    while (offset<size) {
        ssize_t n=send(fd,(const char *)data+offset,size-offset,MSG_NOSIGNAL);
        if (n<0 && errno==EINTR) continue;
        if (n<=0) process_error("cannot send spawn request");
        offset+=(size_t)n;
    }
}
static void socket_read(int fd,void *data,size_t size)
{
    size_t offset=0;
    while (offset<size) {
        ssize_t n=recv(fd,(char *)data+offset,size-offset,0);
        if (n<0 && errno==EINTR) continue;
        if (n<=0) process_error("cannot read spawn response");
        offset+=(size_t)n;
    }
}
static void send_string(int fd,const char *text)
{
    uint32_t length=(uint32_t)strlen(text);
    socket_write(fd,&length,sizeof(length)); socket_write(fd,text,length);
}
/* Captured streams are already the child's wire bytes. Bypass CRT text mode,
 * but use the caller's logical descriptor so dup2/redirection still applies. */
static void replay_stream(int socket,int descriptor,uint32_t size)
{
    char buffer[16384];
    while (size) {
        DWORD count=size>sizeof(buffer)?sizeof(buffer):size,offset=0,written;
        HANDLE handle=(HANDLE)recon_get_osfhandle(descriptor);
        socket_read(socket,buffer,count);
        while (offset<count) {
            if (!recon_WriteFile(handle,buffer+offset,count-offset,&written,NULL) ||
                !written || written>count-offset)
                process_error("cannot replay child stream");
            offset+=written;
        }
        size-=count;
    }
}
static int spawn_request(int mode,const char *file,const char *const *argv,
                         const char *const *environment,unsigned environment_source,
                         SpawnState entry,int allow_state_profile,int *result)
{
    const char *value=getenv("RECON_SPAWN_FD");
    const char *const *env=environment;
    const char *profile=getenv("RECON_SPAWN_STATE_PROFILE");
    int state_profile=profile!=NULL;uint32_t state_fields[4],effect[6];
    char *end,*cwd; long descriptor; size_t size; unsigned n;
    uint32_t header[2],fields[4],response[3],lengths[2],failure_errno=0;
    if(state_profile){
        if(!allow_state_profile || strcmp(profile,"msvcrt510-wait-success-v1") ||
            mode!=_P_WAIT || environment_source!=2 || entry.error || entry.dos!=2 || entry.last!=2)
            process_error("unreviewed spawn state profile/precondition");
        recon_crt_validate_spawn_profile();
    }
    if (!value) {if(state_profile)process_error("spawn state profile requires broker");return 0;}
    errno=0; descriptor=strtol(value,&end,10);
    if (!*value || *end || errno || descriptor<0 || descriptor>INT_MAX)
        process_error("invalid RECON_SPAWN_FD");
    cwd=getcwd(NULL,0); if (!cwd) process_error("cannot get spawn working directory");
    fields[0]=(uint32_t)mode; fields[1]=(uint32_t)getpid(); fields[2]=fields[3]=0;
    while (argv && argv[fields[2]]) if (++fields[2]>4096) process_error("too many spawn arguments");
    while (env && env[fields[3]]) if (++fields[3]>16384) process_error("too many spawn environment entries");
    size=sizeof(fields)+(state_profile?sizeof(state_fields):0)+8+strlen(file)+strlen(cwd);
    for (n=0;n<fields[2];++n) size+=4+strlen(argv[n]);
    for (n=0;n<fields[3];++n) size+=4+strlen(env[n]);
    if (size>4*1024*1024) process_error("spawn request is too large");
    header[0]=state_profile?0x32505352u:0x31505352u; header[1]=(uint32_t)size; /* RSP1/RSP2. */
    socket_write((int)descriptor,header,sizeof(header));
    socket_write((int)descriptor,fields,sizeof(fields));
    if(state_profile){
        state_fields[0]=(uint32_t)entry.error;state_fields[1]=entry.dos;
        state_fields[2]=entry.last;state_fields[3]=environment_source;
        socket_write((int)descriptor,state_fields,sizeof(state_fields));
    }
    send_string((int)descriptor,file); send_string((int)descriptor,cwd);
    for (n=0;n<fields[2];++n) send_string((int)descriptor,argv[n]);
    for (n=0;n<fields[3];++n) send_string((int)descriptor,env[n]);
    free(cwd);
    socket_read((int)descriptor,response,sizeof(response));
    if (response[0]!=header[0] ||
        (state_profile?response[1]!=5:(response[1]!=0 && response[1]!=1 && response[1]!=3 && response[1]!=4)))
        process_error("spawn broker rejected request");
    if ((response[1]==4 && response[2]!=0xffffffffu) ||
        (response[1]!=4 && response[2]>255) || (!response[1] && response[2]))
        process_error("invalid spawn response status");
    if(state_profile){
        socket_read((int)descriptor,effect,sizeof(effect));
        if(response[2] || effect[0]!=(uint32_t)entry.error || effect[1]!=entry.dos || effect[2]!=entry.last ||
            effect[3]!=0 || effect[4]!=0 || effect[5]!=2)
            process_error("unreviewed spawn response parent state/result");
    }
    if (response[1]==4) {
        socket_read((int)descriptor,&failure_errno,sizeof(failure_errno));
        if (!failure_errno || failure_errno>4095)
            process_error("invalid spawn response errno");
    }
    if (response[1]==3 || response[1]==4 || state_profile) {
        socket_read((int)descriptor,lengths,sizeof(lengths));
        if (lengths[0]>64*1024*1024 || lengths[1]>64*1024*1024-lengths[0])
            process_error("spawn response streams are too large");
        replay_stream((int)descriptor,1,lengths[0]);
        replay_stream((int)descriptor,2,lengths[1]);
    }
    if(state_profile){SpawnState after;after.error=(int)effect[3];after.dos=effect[4];after.last=effect[5];restore_spawn_state(after);}
    if (failure_errno) errno=(int)failure_errno;
    if (response[1]) { *result=(int32_t)response[2]; return 1; }
    return 0;
}
/* Exact external-command replay only. This is not a POSIX/Windows shell. */
int recon_system(const char *command)
{
    const char *processor=getenv("COMSPEC"); const char *arguments[4];
    int result,saved_errno=errno;char **owned;unsigned source;const char *const *env;SpawnState entry=spawn_state();
    if (!command || !processor || !*processor)
        recon_unsupported("unreviewed system command processor");
    arguments[0]=processor; arguments[1]="/c"; arguments[2]=command; arguments[3]=NULL;
    env=spawn_environment(NULL,&owned,&source);
    if (!spawn_request(_P_WAIT,processor,arguments,env,source,entry,0,&result))
        recon_unsupported("system requires captured broker response");
    release_spawn_environment(owned);
    if (result!=0) recon_unsupported("unreviewed system command result/fallback");
    errno=saved_errno;
    return result;
}
/* Reject PE files and scripts in the actual execution path. */
static int native_program(const char *file,char *path)
{
    unsigned char header[20]; int fd,n;
    if (recon_host_path(file,path,PATH_MAX)<0) return -1;
    fd=open(path,O_RDONLY); if (fd<0) return -1;
    n=read(fd,header,sizeof(header)); close(fd);
    if (n!=sizeof(header) || memcmp(header,ELFMAG,SELFMAG) ||
        header[EI_CLASS]!=ELFCLASS32 || header[18]!=EM_386 || header[19]) {
        errno=ENOEXEC; return -1;
    }
    return 0;
}
static int spawnve_body(int mode,const char *file,const char *const *argv,const char *const *env,
                        unsigned source,SpawnState entry)
{
    char path[PATH_MAX],logical[PATH_MAX]; pid_t pid; int status,result;
    const char *self=getenv("RECON_SELF_SPAWN");
    const char *require_exists=getenv("RECON_SPAWN_REQUIRE_EXISTS");
    if (mode!=_P_WAIT) { if(getenv("RECON_SPAWN_STATE_PROFILE"))process_error("spawn state profile requires wait mode");errno=ENOTSUP; return -1; }
    if (require_exists) {
        struct stat st;
        if (strcmp(require_exists,"1"))
            process_error("RECON_SPAWN_REQUIRE_EXISTS must be 1 when set");
        /* Original CRT PATH search probes candidates before CreateProcess.
         * Fixtures may opt into that boundary without making virtual mocks
         * or canonical same-program routing depend on host files. */
        if (!(self && !strcmp(self,"1") && recon_pgmptr &&
              recon_host_path(file,path,sizeof(path))==0 &&
              recon_host_path(recon_pgmptr,logical,sizeof(logical))==0 && !strcasecmp(path,logical))) {
            if (recon_host_path(file,path,sizeof(path))<0 || stat(path,&st)<0) return -1;
            if (!S_ISREG(st.st_mode)) { errno=EACCES; return -1; }
            if (access(path,R_OK)<0) return -1;
        }
    }
    if (spawn_request(mode,file,argv,env,source,entry,1,&result)) return result;
    if (self && strcmp(self,"1")) process_error("RECON_SELF_SPAWN must be 1 when set");
    if (self && recon_pgmptr && recon_host_path(file,path,sizeof(path))==0 &&
        recon_host_path(recon_pgmptr,logical,sizeof(logical))==0 && !strcasecmp(path,logical)) {
        if (native_program("/proc/self/exe",path)<0) return -1;
    } else if (native_program(file,path)<0) return -1;
    pid=fork(); if (pid<0) return -1;
    if (!pid) { execve(path,(char *const *)argv,(char *const *)(env?env:(const char *const *)environ)); _exit(127); }
    while (waitpid(pid,&status,0)<0) if (errno!=EINTR) return -1;
    if (!WIFEXITED(status)) { errno=EINTR; return -1; }
    return WEXITSTATUS(status);
}

int recon_spawnve(int mode,const char *file,const char *const *argv,const char *const *explicit_env)
{
    SpawnState entry=spawn_state();char **owned;unsigned source;int result;
    const char *const *env=spawn_environment(explicit_env,&owned,&source);
    result=spawnve_body(mode,file,argv,env,source,entry);release_spawn_environment(owned);return result;
}
__attribute__((weak)) int recon_spawnvpe(int mode,const char *file,const char *const *argv,const char *const *explicit_env)
{
    SpawnState entry=spawn_state();char **owned;unsigned source;int result;
    const char *const *env=spawn_environment(explicit_env,&owned,&source);
    const char *path=recon_getenv("PATH");char buffer[PATH_MAX],program[PATH_MAX];
    result=spawnve_body(mode,file,argv,env,source,entry);
    if(result==-1 && errno==ENOENT && path)while((path=recon_getpath(path,buffer,sizeof(buffer)))!=NULL){
        if(snprintf(program,sizeof(program),"%s\\%s",buffer,file)>=(int)sizeof(program))continue;
        result=spawnve_body(mode,program,argv,env,source,entry);if(result!=-1 || errno!=ENOENT)break;
    }
    release_spawn_environment(owned);return result;
}
