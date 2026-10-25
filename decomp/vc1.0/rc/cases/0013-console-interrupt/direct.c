/* Exact pre-lexer checkpoint and genuine native SIGINT. No GDB or parser
 * mutation after the original boundary; Win32 console/thread semantics remain
 * historical original evidence rather than a Linux delivery claim. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <recon-native.h>
#include <recon.h>
extern int resource_main(int,char **,char **);
extern BOOL __stdcall initialize_lexer(FILE *);
extern BOOL WINAPI interrupt_resource(DWORD);
extern char **environ;
struct Invocation { int argc;char **argv; };
static void invoke(void *value)
{
    struct Invocation *args=value;
    resource_main(args->argc,args->argv,environ);
}
static void require(int condition) { if(!condition)exit(126); }
static int file_equal(const char *path,const void *bytes,unsigned count)
{
    unsigned char buffer[512];FILE *file=fopen(path,"rb");int passed;
    if(!file)return 0;
    passed=count<=sizeof(buffer) && fread(buffer,1,sizeof(buffer),file)==count && !memcmp(buffer,bytes,count);
    fclose(file);return passed;
}
static BOOL __stdcall before_lexer(FILE *stream)
{
    char input[16],output[16],response[256];struct stat info;
    DIR *dir;struct dirent *entry;unsigned files=0;
    static const char source[]="1 RCDATA BEGIN 7 END\n";
    static const char prepared[]="#line 1\"INTERRUPT.RC\"\r\n#line 1\r\n1 RCDATA BEGIN 7 END\r\n";
    require(stream!=0);
    sprintf(input,"RC%06u",(unsigned)getpid()%1000000);
    sprintf(output,"RD%06u",(unsigned)getpid()%1000000);
    sprintf(response,"#line 1 \"%s\"\r\n#line 1 \"INTERRUPT.RC\"\r\n#line 1 \"INTERRUPT.RC\"\r\n1 RCDATA BEGIN 7 END\r\n",input);
    require(file_equal("interrupt.rc",source,sizeof(source)-1));
    require(file_equal(input,prepared,sizeof(prepared)-1));
    require(file_equal(output,response,strlen(response)));
    require(!stat("interrupt.res",&info) && info.st_size==0);
    dir=opendir(".");require(dir!=0);
    while((entry=readdir(dir))!=0)if(strcmp(entry->d_name,".") && strcmp(entry->d_name,".."))++files;
    closedir(dir);require(files==4);
    recon_native_entry_finish(1);
}
static void signal_now(void *unused) { (void)unused;raise(SIGINT); }
static BOOL WINAPI checked_interrupt(DWORD control)
{
    require(control==CTRL_C_EVENT);
    interrupt_resource(control); /* Restored entry; real cleanup never returns. */
    recon_native_entry_finish(0);
}
int interrupted_case(int argc,char **argv)
{
    struct Invocation args;args.argc=argc;args.argv=argv;
    require(recon_native_at_entry(initialize_lexer,before_lexer,invoke,&args)==1);
    require(recon_native_at_entry(interrupt_resource,checked_interrupt,signal_now,0)==1);
    return 126;
}
