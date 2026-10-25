/* Native-only synchronized input glue, outside reconstructed application code.
 * The target registers its real shared SIGINT handler and blocks on this FIFO.
 * No callback invocation, debugger transport or replacement target read occurs. */
#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <time.h>
#include <limits.h>
#include <recon.h>
extern char **environ;
extern int rc_main(int, char **, char **);

static void controller(pid_t target, int server, const char *fifo, const char *directory)
{
    char name[128], text[1024], link[PATH_MAX], wait[64];
    long call;
    unsigned long descriptor;
    int fd, length, n, blocked = 0;
    struct stat output;
    struct timespec pause = { 0, 5000000 };
    for (n = 0; n < 2000; ++n) {
        snprintf(name,sizeof(name),"/proc/%ld/syscall",(long)target);
        fd = open(name,O_RDONLY);
        if (fd >= 0) {
            length = read(fd,text,sizeof(text)-1); close(fd);
            if (length > 0) {
                text[length] = 0;
                if (sscanf(text,"%ld %lx",&call,&descriptor)==2 && call==3) {
                    snprintf(name,sizeof(name),"/proc/%ld/fd/%lu",(long)target,descriptor);
                    length = readlink(name,link,sizeof(link)-1);
                    if (length >= 0) {
                        link[length]=0;
                        snprintf(name,sizeof(name),"/proc/%ld/wchan",(long)target);
                        fd=open(name,O_RDONLY);
                        if(fd>=0) {
                            length=read(fd,wait,sizeof(wait)-1);close(fd);
                            if(length>0) {
                                wait[length]=0;
                                if(!strcmp(link,fifo) && !strcmp(wait,"pipe_read")) {
                                    blocked=stat("result.i",&output)==0 && output.st_size==0;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        if(kill(target,0)) break;
        nanosleep(&pause,NULL);
    }
    if (blocked) {
        if(kill(target,SIGINT)) blocked=0;
    }
    if(!blocked) {
        static const char message[]="RCPP controller: no verified empty blocked FIFO read\n";
        write(2,message,sizeof(message)-1);
        kill(target,SIGKILL);
    }
    close(server);
    unlink(fifo);
    rmdir(directory);
    /* Controller exit is not target/direct completion; do not publish a target status. */
    _exit(blocked ? 0 : 1);
}

int interrupt_invoke(int argc, char **argv)
{
    char cwd[PATH_MAX], directory[PATH_MAX], fifo[PATH_MAX];
    int server;
    pid_t child;
    if(!getcwd(cwd,sizeof(cwd))) return 125;
    if(snprintf(directory,sizeof(directory),"%s/.rcpp-pipes",cwd)>=(int)sizeof(directory)) return 125;
    if(snprintf(fifo,sizeof(fifo),"%s/vc10-rcpp-interrupt",directory)>=(int)sizeof(fifo)) return 125;
    if(mkdir(directory,0700) || mkfifo(fifo,0600)) return 125;
    server=open(fifo,O_RDWR|O_NONBLOCK);
    if(server<0 || setenv("RECON_NAMED_PIPE_ROOT",directory,1)) return 125;
    child=fork();
    if(child<0) return 125;
    if(!child) controller(getppid(),server,fifo,directory);
    /* Permit only this controller to inspect the target syscall/FD boundary. */
    if(prctl(PR_SET_PTRACER,child,0,0,0)) { kill(child,SIGKILL); close(server); return 125; }
    close(server);
    return rc_main(argc,argv,environ);
}
