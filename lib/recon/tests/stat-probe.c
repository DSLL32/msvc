#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <sys/stat.h>
#include <recon.h>
int main(int argc,char **argv)
{
    struct _stat status;
    int i,j,result;
    printf("layout %u %u %u %u %u\n",sizeof(status),offsetof(struct _stat,st_size),offsetof(struct _stat,st_atime),offsetof(struct _stat,st_mtime),offsetof(struct _stat,st_ctime));
    for (j=1;j<argc;j++) {
        memset(&status,0x55,sizeof(status)); errno=0;
        result=_stat(argv[j],&status);
        printf("stat %s %d %d %u %u %o %d %d %d %u %ld %ld %ld %ld ",argv[j],result,errno,status.st_dev,status.st_ino,status.st_mode,status.st_nlink,status.st_uid,status.st_gid,status.st_rdev,status.st_size,status.st_atime,status.st_mtime,status.st_ctime);
        for(i=0;i<sizeof(status);i++) printf("%02x",((unsigned char *)&status)[i]);
        printf("\n");
    }
    return 0;
}
