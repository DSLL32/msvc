#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
static void hold_interrupt(int sig) { (void)sig; write(1,"interrupt\n",10); _exit(0); }
int fixture_add(int a, int b) { return a + b; }
int fixture_trap(void)
{
    __asm__ volatile("int3");
    return 0;
}
int fixture_returning_stub(void) { return 0; }
void fixture_on_exit(void)
{
    FILE *f = fopen("exit.txt", "wb");
    fputs("closed", f);
    fclose(f);
    puts("bye");
}
int fixture_main(int argc, char **argv)
{
    char line[128];
    FILE *f;
    if (getenv("RECON_CHECK_PROCESS"))
    {
        const char *setting=getenv("RECON_SPAWN_FD");
        uint32_t frame[]={0x31505352,16+4+14+4,0,0,0,0,14};
        uint32_t empty=0,reply[3];int fd;
        if(!setting)return 9;
        fd=atoi(setting);frame[3]=(uint32_t)getpid();
        if(write(fd,frame,sizeof(frame))!=sizeof(frame) ||
            write(fd,"undeclared.exe",14)!=14 || write(fd,&empty,4)!=4 ||
            read(fd,reply,sizeof(reply))!=sizeof(reply) || reply[1]!=2)return 10;
        return 0; /* Handling a denial must still fail the whole fixture. */
    }
    if (getenv("RECON_CHECK_HOLD"))
    {
        signal(SIGINT,hold_interrupt);
        puts("ready"); fflush(stdout);
        if(!fgets(line,sizeof(line),stdin)) return 8;
        return 9;
    }
    if (getenv("RECON_CHECK_NULL"))
    {
        struct stat status;
        if (fstat(0, &status) || !S_ISCHR(status.st_mode) || getchar() != EOF)
            return 8;
        puts("null");
        return 0;
    }
    if (getenv("RECON_CHECK_ARGV"))
    {
        printf("argc=%d argv0=%s\n", argc, argc ? argv[0] : "none");
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "atexit"))
    {
        atexit(fixture_on_exit);
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "backtrap"))
    {
        fixture_returning_stub();
        __asm__ volatile("int3");
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "trap"))
        return fixture_trap();
    if (argc > 1 && !strcmp(argv[1], "orphan"))
    {
        if (!fork())
        {
            if (!fork())
            {
                usleep(30000);
                f = fopen("grandchild.txt", "wb");
                fputs("done", f);
                fclose(f);
                _exit(0);
            }
            _exit(0);
        }
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "timeout"))
    {
        if (!fork())
        {
            setsid();
            for (;;)
                pause();
        }
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "event"))
    {
        puts("ready");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin))
            return 3;
        printf("got:%s", line);
        return 0;
    }
    if (!fgets(line, sizeof(line), stdin))
        return 3;
    printf("result=%d %s", fixture_add(2, 3), line);
    f = fopen("out.bin", "wb");
    if (!f)
        return 4;
    fwrite("\0\xff", 1, 2, f);
    fclose(f);
    return 0;
}
