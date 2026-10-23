#include <assert.h>
#include <recon.h>
struct _startupinfo { int newmode; };
static int math_callback(struct _exception *e) { (void)e; return 7; }
int main(int count,char **arguments)
{
    char name[]="tool.exe",flag[]="/help",variable[]="ONLY_TARGET=value";
    char *args[]={name,flag,NULL},*env[]={variable,NULL},*other[]={NULL};
    char **av,**ev; int ac; struct _startupinfo info;
    info.newmode=0;
    if (count>1 && !strcmp(arguments[1],"unbound")) __getmainargs(&ac,&av,&ev,0,&info);
    recon_crt_bind_arguments(2,args,env);
    if (count>1 && !strcmp(arguments[1],"wildcard")) __getmainargs(&ac,&av,&ev,1,&info);
    if (count>1 && !strcmp(arguments[1],"rebind")) recon_crt_bind_arguments(0,other,env);
    if (count>1 && !strcmp(arguments[1],"fsopen")) _fsopen("unused","r",64);
    errno=EDOM; SetLastError(1234);
    __getmainargs(&ac,&av,&ev,0,&info);
    assert(ac==2 && av==args && ev==env && info.newmode==0 && recon_crt_newmode==0);
    assert(errno==EDOM && GetLastError()==1234);
    flag[0]='!'; variable[0]='X';
    assert(av[1][0]=='!' && ev[0][0]=='X' && ev[1]==NULL);
    assert(__p__fmode()==&recon_fmode && __p__commode()==&recon_commode);
    *__p__fmode()=0x8000; *__p__commode()=7;
    assert(recon_fmode==0x8000 && recon_commode==7);
    assert(*__p___initenv()==NULL); *__p___initenv()=ev;
    assert(recon_crt_initenv==env);
    __set_app_type(1); assert(recon_crt_app_type==1);
    __setusermatherr(math_callback); assert(recon_crt_matherr==math_callback);
    recon_crt_bind_arguments(2,args,env);
    return 0;
}
