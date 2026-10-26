#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <recon.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

static BOOL WINAPI module_entry(HINSTANCE module,DWORD reason,LPVOID reserved)
{
    (void)module;(void)reserved;
    if(reason==DLL_PROCESS_DETACH) {
        assert(recon_crt_argc>=1 && recon_crt_argv && recon_crt_environment);
        assert(recon_pgmptr && recon_GetCommandLineA());puts("final detach inputs live PASS");
    }
    return TRUE;
}
static char *windows(const char *text)
{
    char *out=malloc(strlen(text)+3),*p;assert(out);strcpy(out,"Z:");strcat(out,text);
    for(p=out;*p;++p)if(*p=='/')*p='\\';return out;
}
int main(int argc,char **argv,char **envp)
{
    ReconNativeEntry *entry;char actual[4096],cwd[4096],*expected;
    ssize_t length;unsigned short cw,after;int fmode=recon_fmode;const char *line;
    const char *call[4];char *child_env[]={"MSC_CMD_FLAGS=only",NULL};int result;
    __asm__ volatile("fnstcw %0":"=m"(cw));
    errno=EIO;recon_doserrno=999;recon_SetLastError(888);
    entry=recon_native_entry_initialize(argc,argv,envp);
    __asm__ volatile("fnstcw %0":"=m"(after));
    assert(cw==after && fmode==recon_fmode && !errno && !recon_doserrno && !recon_GetLastError());
    assert(entry->argc==argc && entry->argv!=argv && entry->environment!=envp);
    assert(entry->argv[argc]==NULL && recon_crt_argv==entry->argv && recon_crt_environment==entry->environment);
    assert(recon_crt_initenv==entry->environment && recon_GetACP()==1252);
    length=readlink("/proc/self/exe",actual,sizeof(actual)-1);assert(length>0);actual[length]=0;
    expected=windows(actual);assert(!strcmp(expected,entry->program_path) && !strcmp(recon_pgmptr,expected));free(expected);
    assert(getcwd(cwd,sizeof(cwd)));expected=windows(cwd);assert(!strcmp(expected,entry->current_directory));free(expected);
    assert(!strcmp(entry->command_line,recon_GetCommandLineA()));
    if(argc>1 && !strcmp(argv[1],"child")) {
        assert(argc==3 && !strcmp(entry->argv[2],"payload"));
        assert(entry->environment[0] && !strcmp(entry->environment[0],"MSC_CMD_FLAGS=only") && !entry->environment[1]);
        assert(!strcmp(recon_aenvptr,"MSC_CMD_FLAGS=only"));
        puts("minimal child PASS");return 0;
    }
    if(argc>1 && !strcmp(argv[1],"empty-child")) {
        assert(!entry->environment[0] && !recon_aenvptr[0] && !recon_aenvptr[1]);
        puts("empty child PASS");return 0;
    }
    if(argc>1 && !strcmp(argv[1],"quote")) {
        /* Direct shell argv is never blindly retokenized. */
        assert(argc==8 && !strcmp(entry->argv[2],"") && !strcmp(entry->argv[3],"two words"));
        assert(!strcmp(entry->argv[4],"tail\\") && !strcmp(entry->argv[5],"a\"b"));
        assert(!strcmp(entry->argv[6],"a\\\"b") && !strcmp(entry->argv[7],"a b\\"));
        line=strchr(entry->command_line,' ');assert(line);
        assert(!strcmp(line," quote \"\" \"two words\" tail\\ \"a\\\"b\" \"a\\\\\\\"b\" \"a b\\\\\""));
        puts("direct argv/Windows quoting PASS");return 0;
    }
    assert(recon_crt_os_environment && recon_crt_os_environment!=(void *)entry->environment);
    /* Mutating the copied CRT value must not rewrite host or OS snapshots. */
    if(entry->environment[0]) {
        char initial=entry->environment[0][0];entry->environment[0][0]='!';
        assert(envp[0][0]==initial && recon_crt_os_environment[0]!=(WCHAR)'!');
        entry->environment[0][0]=initial;
    }
    call[0]=entry->program_path;call[1]="child";call[2]="payload";call[3]=NULL;
    result=recon_spawnve(_P_WAIT,entry->program_path,call,(const char *const *)child_env);assert(result==0);
    call[1]="empty-child";call[2]=NULL;
    {const char *empty[]={NULL};assert(!recon_spawnve(_P_WAIT,entry->program_path,call,empty));}
    /* Inherited OS environment also works for LIBC100, independently of CRT. */
    call[1]="child";call[2]="payload";
    if(envp[0] && !strcmp(envp[0],"MSC_CMD_FLAGS=only") && !envp[1])
        assert(!recon_spawnv(_P_WAIT,entry->program_path,call));
    {
        static const char sha[]="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
        ReconModuleSpec identity={"cleanup.dll","Z:\\cleanup.dll",sha,RECON_MODULE_NO_TLS};
        ReconStaticModuleSpec spec={NULL,0,module_entry,NULL,0,NULL};HMODULE module;
        module=recon_register_module(&identity);assert(module && recon_configure_static_module(module,&spec));
        assert(recon_attach_static_module(module));
        /* Actual process finalizer must still see every owned startup input. */
    }
    puts("ordinary owned startup/OS inheritance/identity/cleanup PASS");return 0;
}
