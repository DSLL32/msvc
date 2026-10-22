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
static int spawn_request(int mode,const char *file,const char *const *argv,
                         const char *const *environment,int *result)
{
    const char *value=getenv("RECON_SPAWN_FD");
    const char *const *env=environment?environment:(const char *const *)environ;
    char *end,*cwd; long descriptor; size_t size; unsigned n;
    uint32_t header[2],fields[4],response[3];
    if (!value) return 0;
    errno=0; descriptor=strtol(value,&end,10);
    if (!*value || *end || errno || descriptor<0 || descriptor>INT_MAX)
        process_error("invalid RECON_SPAWN_FD");
    cwd=getcwd(NULL,0); if (!cwd) process_error("cannot get spawn working directory");
    fields[0]=(uint32_t)mode; fields[1]=(uint32_t)getpid(); fields[2]=fields[3]=0;
    while (argv && argv[fields[2]]) if (++fields[2]>4096) process_error("too many spawn arguments");
    while (env && env[fields[3]]) if (++fields[3]>16384) process_error("too many spawn environment entries");
    size=sizeof(fields)+8+strlen(file)+strlen(cwd);
    for (n=0;n<fields[2];++n) size+=4+strlen(argv[n]);
    for (n=0;n<fields[3];++n) size+=4+strlen(env[n]);
    if (size>4*1024*1024) process_error("spawn request is too large");
    header[0]=0x31505352u; header[1]=(uint32_t)size; /* RSP1, little-endian i386. */
    socket_write((int)descriptor,header,sizeof(header));
    socket_write((int)descriptor,fields,sizeof(fields));
    send_string((int)descriptor,file); send_string((int)descriptor,cwd);
    for (n=0;n<fields[2];++n) send_string((int)descriptor,argv[n]);
    for (n=0;n<fields[3];++n) send_string((int)descriptor,env[n]);
    free(cwd);
    socket_read((int)descriptor,response,sizeof(response));
    if (response[0]!=0x31505352u || response[1]>1)
        process_error("spawn broker rejected request");
    if (response[1]) { *result=(int32_t)response[2]; return 1; }
    return 0;
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
int recon_spawnve(int mode,const char *file,const char *const *argv,const char *const *env)
{
    char path[PATH_MAX],logical[PATH_MAX]; pid_t pid; int status,result;
    const char *self=getenv("RECON_SELF_SPAWN");
    if (mode!=_P_WAIT) { errno=ENOTSUP; return -1; }
    if (spawn_request(mode,file,argv,env,&result)) return result;
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
