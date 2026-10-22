/* Native spawn and traced exec integration probe. */
#include <unistd.h>
#include <fcntl.h>
#include <recon.h>
/* Raw POSIX probes deliberately bypass the legacy descriptor aliases. */
#undef read
#undef write
#undef close
#undef unlink

int __attribute__((noinline)) common(int value) { return value+1; }
int __attribute__((noinline)) child_only(void)
{
    int fd; unsigned long pid=(unsigned long)getpid();
    if (!getenv("SHIM_CHILD_MARKER") || strcmp(getenv("SHIM_CHILD_MARKER"),"preserved")) return 30;
    if (common(4)!=5) return 31;
    fd=_open("child.pid",0x8301,0600);
    if (fd<0 || _write(fd,&pid,sizeof(pid))!=sizeof(pid) || _close(fd)) return 32;
    return 7;
}
int main(int argc,char **argv)
{
    const char *args[5]; int result,fd; char bytes[8];
    if (argc==2 && !strcmp(argv[1],"child")) {
        if (strcmp(argv[0],_pgmptr)) return 33;
        return child_only();
    }
    if (argc==2 && !strcmp(argv[1],"mock")) {
        args[0]="cvpack.exe"; args[1]="/nologo"; args[2]="/cvnew"; args[3]="input.bin"; args[4]=NULL;
        result=_spawnve(_P_WAIT,"cvpack.exe",args,NULL);
        fd=_open("input.bin",0x8000);
        if (result!=9 || fd<0 || _read(fd,bytes,6)!=6 || memcmp(bytes,"packed",6) || _close(fd)) return 34;
    } else if (argc==2 && !strcmp(argv[1],"pe")) {
        args[0]="original.exe"; args[1]=NULL;
        if (_spawnve(_P_WAIT,"original.exe",args,NULL)!=-1 || errno!=ENOEXEC) return 35;
    } else {
        args[0]=_pgmptr; args[1]="child"; args[2]=NULL;
        if (common(2)!=3 || _spawnve(_P_WAIT,_pgmptr,args,NULL)!=7 || common(6)!=7) return 36;
    }
    write(1,"process target output\n",22);
    return 0;
}
