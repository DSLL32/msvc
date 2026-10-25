#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include "recon-fixture.h"

int fixture_state=7;
extern char **environ;
static int write_bytes(int fd, const void *data, size_t size)
{
    const char *bytes=data;
    while(size) { ssize_t n=write(fd,bytes,size); if(n<=0)return 0;bytes+=n;size-=n; }
    return 1;
}
static int read_bytes(int fd, void *data, size_t size)
{
    char *bytes=data;
    while(size) { ssize_t n=read(fd,bytes,size); if(n<=0)return 0;bytes+=n;size-=n; }
    return 1;
}
static int send_string(int fd,const char *text)
{
    uint32_t n=(uint32_t)strlen(text);
    return write_bytes(fd,&n,4) && write_bytes(fd,text,n);
}
static int process_fixture(void)
{
    char cwd[4096],buffer[128];
    const char *setting=getenv("RECON_SPAWN_FD");
    uint32_t reply[3],lengths[2],fields[8],header[2],failure;
    int fd,i,envc=0,iterations=getenv("RECON_PROCESS_EXTRA")?2:1;
    if(getenv("RECON_PROCESS_MISSING"))return 0;
    if(!setting || !getcwd(cwd,sizeof(cwd)))return 30;
    while(environ[envc])++envc;
    fd=atoi(setting);
    for(i=0;i<iterations;++i)
    {
        unsigned j;
        fields[0]=0;fields[1]=(uint32_t)getpid();fields[2]=2;fields[3]=envc;
        fields[4]=0;fields[5]=2;fields[6]=2;fields[7]=2;
        header[0]=getenv("RECON_PROCESS_RSP2")?0x32505352:0x31505352;
        header[1]=(header[0]==0x32505352?32:16)+4+12+4+strlen(cwd)+4+12+4+1;
        for(j=0;j<(unsigned)envc;++j)header[1]+=4+strlen(environ[j]);
        if(!write_bytes(fd,header,8) || !write_bytes(fd,fields,header[0]==0x32505352?32:16) ||
            !send_string(fd,"captured.exe") || !send_string(fd,cwd) || !send_string(fd,"captured.exe") || !send_string(fd,"x"))return 31;
        for(j=0;j<(unsigned)envc;++j)if(!send_string(fd,environ[j]))return 32;
        if(!read_bytes(fd,reply,12))return 33;
        if(reply[1]!=2 && reply[2]!=(getenv("RECON_PROCESS_FAIL")?0xffffffffu:getenv("RECON_PROCESS_SEVEN")?7u:0u))return 39;
        if(reply[1]==2)return 0;
        if(header[0]==0x32505352) {uint32_t state[6];if(reply[1]!=5 || !read_bytes(fd,state,24) || state[0]!=0 || state[1]!=2 || state[2]!=2 || state[3] || state[4] || state[5]!=2)return 34;}
        else if(reply[1]==4) {if(!read_bytes(fd,&failure,4) || failure!=2)return 35;}
        else if(reply[1]!=3)return 36;
        if(!read_bytes(fd,lengths,8))return 37;
        for(j=0;j<2;++j) {
            if(lengths[j]>sizeof(buffer) || !read_bytes(fd,buffer,lengths[j]) || !write_bytes(j+1,buffer,lengths[j]))return 38;
        }
    }
    return 0;
}

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
    if (getenv("RECON_CHECK_REPO"))
    {
        const char *repository=getenv("RECON_DRIVE_R");
        printf("R=%s\n",repository?repository:"unset");
        return 0;
    }
    if (getenv("RECON_CHECK_OUTPUT"))
    {
        int result=process_fixture();
        if(result)return result;
        fixture_state=99;
        printf("pid:%u\n",(unsigned)getpid());
        if(!fork()) {usleep(20000);f=fopen("descendant.txt","wb");if(f){fputs("done",f);fclose(f);}_exit(0);}
        return 0;
    }
    if (getenv("RECON_CHECK_POLICY"))
        return process_fixture();
    if (getenv("RECON_CHECK_ASSETS"))
    {
        char cwd[4096], path[4096];
        unsigned char bytes[3];
        const char *assets = recon_case_assets();
        if (!assets || !getcwd(cwd,sizeof(cwd)) ||
            strcmp(cwd,recon_case_workspace_cwd()) || getenv("RECON_CASE_ASSETS"))
            return 11;
        snprintf(path,sizeof(path),"%s/captures/response.bin",assets);
        f=fopen(path,"rb");
        if (!f || fread(bytes,1,3,f)!=3 || fgetc(f)!=EOF ||
            bytes[0]!=0 || bytes[1]!=255 || bytes[2]!=42)
            return 12;
        fclose(f);
        if (getenv("RECON_CHANGE_ASSET"))
        {
            chmod(path,0600);
            f=fopen(path,"wb");
            if(!f) return 13;
            fputs("bad",f); fclose(f);
        }
        puts("asset=3");
        return 0;
    }
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
