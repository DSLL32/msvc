/* Host-only runner reentry probe; no reconstructed application functions. */
#define RECON_SHIM_IMPL
#include <recon.h>
#include <unistd.h>
#include <limits.h>
int main(int argc,char **argv)
{
    char executable[PATH_MAX],child_mode[]="native-child",case_path[]="/owned/case.so";
    const char *prefix[3]={child_mode,case_path,"--"};
    const char *args[3];const char *file;int n,result;
    if(argc==6 && !strcmp(argv[1],"native-child")){
        if(strcmp(argv[2],"/owned/case.so") || strcmp(argv[3],"--") ||
            strcmp(argv[4],"Z:\\virtual\\LINK.EXE") || strcmp(argv[5],"payload") ||
            !getenv("CHILD_MARKER") || strcmp(getenv("CHILD_MARKER"),"inherited"))return 90;
        write(1,"routed\n",7);return 42;
    }
    if(argc==2 && !strcmp(argv[1],"payload")){write(1,"legacy\n",7);return 43;}
    if(argc!=2)return 91;
    n=readlink("/proc/self/exe",executable,sizeof(executable)-1);if(n<0)return 92;executable[n]=0;
    if(strcmp(argv[1],"legacy")){
        const char *owner=!strcmp(argv[1],"wrongexe")?"/bin/true":executable;
        if(!strcmp(argv[1],"copyexe"))owner=getenv("OTHER_ELF");
        errno=EDOM;recon_doserrno=7;recon_SetLastError(11);
        recon_set_native_self_command(owner,prefix,!strcmp(argv[1],"zero")?0:3);
        if(errno!=EDOM || recon_doserrno!=7 || recon_GetLastError()!=11)return 93;
        if(!strcmp(argv[1],"twice"))recon_set_native_self_command(executable,prefix,3);
        /* Host caller storage may disappear/change after registration. */
        child_mode[0]='X';case_path[1]='X';prefix[2]="bad";
    }
    file=!strcmp(argv[1],"wrongself")?"Z:\\virtual\\OTHER.EXE":recon_pgmptr;
    args[0]=recon_pgmptr;args[1]="payload";args[2]=NULL;
    result=recon_spawnve(_P_WAIT,file,args,NULL);
    if(!strcmp(argv[1],"wrongself") || !strcmp(argv[1],"disabled"))return result==-1?0:94;
    if(!strcmp(argv[1],"replay"))return result==9?0:95;
    return result==(!strcmp(argv[1],"legacy")?43:42)?0:96;
}
