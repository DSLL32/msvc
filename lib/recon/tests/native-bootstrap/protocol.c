#define _GNU_SOURCE
#define RECON_NATIVE_ENTRY_PROTOCOL_V1
#define RECON_SHIM_IMPL
#include <recon.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

static unsigned descriptors(void)
{
    unsigned count=0;struct dirent *entry;DIR *directory=opendir("/proc/self/fd");assert(directory);
    while((entry=readdir(directory)))if(entry->d_name[0]!='.')++count;
    assert(!closedir(directory));return count;
}
int main(int argc,char **argv,char **envp)
{
    ReconNativeEntry *entry=recon_native_entry_initialize(argc,argv,envp);
    const char *call[4];char *minimal[]={"MSC_CMD_FLAGS=only",NULL};unsigned n,before;
    char expected[4096],bytes[5]={0};struct stat first,second;int fd,result;
    if(entry->argc>1 && !strcmp(entry->argv[1],"raw-child")) {
        assert(entry->argc==11 && !strcmp(entry->argv[2],"-f") && !strcmp(entry->argv[3],"in.txt"));
        assert(!strcmp(entry->argv[4],"-g") && !strcmp(entry->argv[5],"out.txt"));
        assert(!strcmp(entry->argv[6],"-D") && !strcmp(entry->argv[7],"NAME=a b"));
        assert(!strcmp(entry->argv[8],"-pc") && !strcmp(entry->argv[9],"\\:/") && !strcmp(entry->argv[10],"-E"));
        snprintf(expected,sizeof(expected),"%s raw-child   -f in.txt -g out.txt -D \"NAME=a b\" -pc \\:/ -E ",entry->program_path);
        assert(!strcmp(entry->command_line,expected));
        assert(fcntl(198,F_GETFD)==-1 && errno==EBADF);
        assert(entry->environment[0] && !strcmp(entry->environment[0],"MSC_CMD_FLAGS=only") && !entry->environment[1]);
        puts("raw child parsed/transport consumed/minimal environment PASS");return 0;
    }
    if(entry->argc>1 && !strcmp(entry->argv[1],"escaped-child")) {
        assert(entry->argc==7 && !*entry->argv[2] && !strcmp(entry->argv[3],"a b"));
        assert(!strcmp(entry->argv[4],"a\"b") && !strcmp(entry->argv[5],"tail\\") && !strcmp(entry->argv[6],"a\\b"));
        assert(fcntl(198,F_GETFD)==-1 && errno==EBADF);puts("raw quotes/backslashes/empty PASS");return 0;
    }
    if(entry->argc>1 && !strcmp(entry->argv[1],"double-child")) {
        assert(entry->argc==4 && !strcmp(entry->argv[2],"a\"b") && !strcmp(entry->argv[3],"c"));
        puts("old doubled-quote toggle PASS");return 0;
    }
    if(entry->argc>1 && !strcmp(entry->argv[1],"program-child")) {
        assert(entry->argc==2 && !strcmp(entry->argv[0],"label with spaces"));
        assert(!strcmp(entry->command_line,"\"label with spaces\" program-child"));
        puts("old quoted argv0 PASS");return 0;
    }
    if(entry->argc>1 && !strcmp(entry->argv[1],"unrelated")) {
        assert(fcntl(198,F_GETFD)>=0 && pread(198,bytes,4,0)==4 && !strcmp(bytes,"keep"));
        assert(!close(198));puts("unrelated direct descriptor retained PASS");return 0;
    }
    assert(entry->argc==2);before=descriptors();
    call[0]=entry->program_path;call[1]="raw-child";
    call[2]="  -f in.txt -g out.txt -D \"NAME=a b\" -pc \\:/ -E ";call[3]=NULL;
    for(n=0;n<12;++n)assert(!recon_spawnve(_P_WAIT,entry->program_path,call,(const char *const *)minimal));
    assert(descriptors()==before && fcntl(198,F_GETFD)==-1 && errno==EBADF);
    call[1]="escaped-child";call[2]="\"\" \"a b\" a\\\"b tail\\ a\\b";
    assert(!recon_spawnve(_P_WAIT,entry->program_path,call,(const char *const *)minimal));
    call[1]="double-child";call[2]="\"a\"\"b c\"";
    assert(!recon_spawnve(_P_WAIT,entry->program_path,call,(const char *const *)minimal));
    call[0]="\"label with spaces\"";call[1]="program-child";call[2]=NULL;
    assert(!recon_spawnve(_P_WAIT,entry->program_path,call,(const char *const *)minimal));
    call[0]=entry->program_path;
    fd=open("slot-owner",O_RDWR|O_CREAT|O_TRUNC,0600);assert(fd>=0 && write(fd,"keep",4)==4);
    assert(!fstat(fd,&first) && dup2(fd,198)==198);assert(!close(fd));
    result=recon_spawnve(_P_WAIT,entry->program_path,call,(const char *const *)minimal);
    assert(result==-1 && errno==EBUSY && !fstat(198,&second) && first.st_dev==second.st_dev && first.st_ino==second.st_ino);
    assert(pread(198,bytes,4,0)==4 && !strcmp(bytes,"keep"));assert(!close(198));
    call[0]=entry->argv[1];call[1]="packed first second";call[2]=NULL;
    assert(!recon_spawnve(_P_WAIT,entry->argv[1],call,(const char *const *)minimal));
    assert(descriptors()==before);puts("occupied slot/no parent leaks/unmarked native target PASS");return 0;
}
