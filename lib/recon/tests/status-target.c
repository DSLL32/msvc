#include <sys/wait.h>
#include <recon.h>
int recon_status_entry(int argc,char **argv,char **env)
{
    uint32_t code; int status; pid_t child;
    (void)env;
    if (argc<3) return 0;
    code=(uint32_t)strtoul(argv[2],NULL,0);
    if (!strcmp(argv[1],"duplicate")) { recon_record_terminal_status(code); recon_record_terminal_status(code); }
    if (!strcmp(argv[1],"exit")) exit((int)code);
    if (!strcmp(argv[1],"win32")) ExitProcess(code);
    if (!strcmp(argv[1],"fork")) {
        child=fork(); if (!child) exit(7); waitpid(child,&status,0);
    }
    if (!strcmp(argv[1],"exec")) {
        child=fork(); if (!child) { execl(argv[0],argv[0],"exit","7",NULL); _exit(127); }
        waitpid(child,&status,0);
    }
    if (!strcmp(argv[1],"missing")) _exit(code);
    return (int)code;
}
