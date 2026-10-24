#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
int fixture_add(int a, int b) { return a+b; }
int fixture_trap(void) { __asm__ volatile("int3"); return 0; }
void fixture_on_exit(void)
{
    FILE *f=fopen("exit.txt","wb");fputs("closed",f);fclose(f);
    puts("bye");setenv("RECON_EXIT_VALUE","after",1);
}
int fixture_main(int argc,char **argv)
{
    char line[128]; FILE *f;
    if(argc>1 && !strcmp(argv[1],"atexit")){atexit(fixture_on_exit);return 0;}
    if(argc>1 && !strcmp(argv[1],"trap")) return fixture_trap();
    if(argc>1 && !strcmp(argv[1],"orphan")) {
        if(!fork()) { if(!fork()) {usleep(30000);f=fopen("grandchild.txt","wb");fputs("done",f);fclose(f);_exit(0);} _exit(0); }
        return 0;
    }
    if(argc>1 && !strcmp(argv[1],"timeout")) { if(!fork()){setsid();for(;;)pause();}return 0; }
    if(argc>1 && !strcmp(argv[1],"event")){puts("ready");fflush(stdout);if(!fgets(line,sizeof(line),stdin))return 3;printf("got:%s",line);return 0;}
    if(!fgets(line,sizeof(line),stdin))return 3;
    printf("result=%d %s",fixture_add(2,3),line);
    f=fopen("out.bin","wb");if(!f)return 4;fwrite("\0\xff",1,2,f);fclose(f);return 0;
}
