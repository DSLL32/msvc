#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
#include <recon.h>
static char user[258];
static void observe(FILE *s,const char *name,char *buffer,int mode,size_t size)
{
    int result,error; DWORD last; int role;
    errno=111; SetLastError(0x12345678); result=setvbuf(s,buffer,mode,size);error=errno;last=GetLastError();
    role=!s->_base?0:s->_base==user?1:s->_base==(char *)&s->_charbuf?2:3;
    printf("%s %d %x %d %d %d %d %d %d %d %lu\n",name,result,s->_flag,s->_cnt,s->_bufsiz,role,s->_ptr==s->_base,s->_charbuf,s->_tmpfname==NULL,error,last);
}
int main(int argc,char **argv)
{
    FILE *s; s=fopen("stdio.dat","wb"); if(!s) return 3;
    observe(s,"owned",NULL,_IOFBF,2048);
    observe(s,"user",user,_IOLBF,257);
    observe(s,"none",NULL,_IONBF,0);
    observe(s,"owned-odd",NULL,_IOFBF,513);
    observe(s,"bad-size",NULL,_IOFBF,1);
    observe(s,"bad-mode",NULL,3,2048);
    if(argc>1 && !strcmp(argv[1],"data")) { fputs("pending",s); observe(s,"pending-user",user,_IOFBF,258); fputs("after",s); }
    if(fclose(s)) return 4; return 0;
}
