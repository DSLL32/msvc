#include <assert.h>
#include <recon.h>
static int live_values;
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static char *arguments[]={"LINK.EXE",NULL};
static char *environment[]={"TMP=.","TEMP=ignored",NULL};
static char *other_environment[]={"TMP=elsewhere",NULL};
static void check_errors(unsigned char before[108])
{
    unsigned char after[108];fp(after);
    assert(errno==111 && recon_doserrno==222 && GetLastError()==333);
    assert(!memcmp(before,after,2) && !memcmp(before+4,after+4,2) && !memcmp(before+8,after+8,2));
    if(live_values)assert(!memcmp(before+28,after+28,20));
}
static void errors(unsigned char before[108])
{ errno=111;recon_doserrno=222;SetLastError(333);fp(before); }
int main(int argc,char **argv)
{
    struct {unsigned char left[4];char bytes[260];unsigned char right[4];} path,first,second;
    char collision[260],bytes[8];unsigned char before[108];DWORD count,length;UINT one,two,next;
    int custom=argc>2 && !strcmp(argv[1],"ascii-prefix");
    const char *prefix=custom?argv[2]:argc>1 && !strcmp(argv[1],"short-prefix")?"hel":
        argc>1 && !strcmp(argv[1],"long-prefix")?"hellox":
        argc>1 && !strncmp(argv[1],"hello",5)?"hello":"lah";
    size_t prefix_bytes=strlen(prefix)<3?strlen(prefix):3;
    HANDLE file;int live=argc>1 && (!strcmp(argv[1],"live") || !strcmp(argv[1],"hello-live"));
    int selected=custom || (argc>1 && (!strcmp(argv[1],"hello") || !strcmp(argv[1],"hello-live") || !strcmp(argv[1],"short-prefix") || !strcmp(argv[1],"long-prefix")));
    live_values=live;
    if(argc<2 || strcmp(argv[1],"unbound"))
        recon_crt_bind_arguments(1,arguments,argc>1 && !strcmp(argv[1],"environment")?other_environment:environment);
    memset(&path,0xa5,sizeof(path));memset(&first,0xa5,sizeof(first));memset(&second,0xa5,sizeof(second));
    if(argc>1 && !live && !selected) {
        if(!strcmp(argv[1],"capacity")){GetTempPathA(259,path.bytes);return 42;}
        if(!strcmp(argv[1],"null")){GetTempPathA(260,NULL);return 42;}
        if(!strcmp(argv[1],"wrap")){GetTempPathA(260,(char *)0xfffffff0U);return 42;}
        if(!strcmp(argv[1],"env-overlap")){GetTempPathA(260,environment[0]+4);return 42;}
        GetTempPathA(260,path.bytes);
        if(!strcmp(argv[1],"unique"))GetTempFileNameA(path.bytes,"lah",1,first.bytes);
        else if(!strcmp(argv[1],"prefix"))GetTempFileNameA(path.bytes,"a/b",0,first.bytes);
        else if(!strcmp(argv[1],"short-prefix"))GetTempFileNameA(path.bytes,"hel",0,first.bytes);
        else if(!strcmp(argv[1],"long-prefix"))GetTempFileNameA(path.bytes,"hellox",0,first.bytes);
        else if(!strcmp(argv[1],"prefix-wrap"))GetTempFileNameA(path.bytes,(const char *)0xfffffffeU,0,first.bytes);
        else if(!strcmp(argv[1],"hello-overlap")){strcpy(first.bytes,"hello");GetTempFileNameA(path.bytes,first.bytes,0,first.bytes+4);}
        else if(!strcmp(argv[1],"directory"))GetTempFileNameA(".","lah",0,first.bytes);
        else if(!strcmp(argv[1],"overlap"))GetTempFileNameA(path.bytes,"lah",0,path.bytes);
        else if(!strcmp(argv[1],"file-null"))GetTempFileNameA(path.bytes,"lah",0,NULL);
        else if(!strcmp(argv[1],"file-wrap"))GetTempFileNameA(path.bytes,"lah",0,(char *)0xfffffff0U);
        else assert(0);
        return 42;
    }
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    errors(before);length=GetTempPathA(260,path.bytes);check_errors(before);
    assert(length==8 && !strcmp(path.bytes,"S:\\case\\"));
    assert(!memcmp(path.left,"\245\245\245\245",4) && !memcmp(path.right,"\245\245\245\245",4));
    assert((unsigned char)path.bytes[length+1]==0xa5);
    errors(before);one=GetTempFileNameA(path.bytes,prefix,0,first.bytes);check_errors(before);
    assert(one && one<=65535 && strlen(first.bytes)==length+prefix_bytes+8);
    snprintf(collision,sizeof(collision),"%s%.3s%04x.tmp",path.bytes,prefix,one);assert(!strcmp(first.bytes,collision));
    assert((unsigned char)first.bytes[length+prefix_bytes+9]==0xa5);
    file=CreateFileA(first.bytes,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);assert(file!=INVALID_HANDLE_VALUE);
    assert(ReadFile(file,bytes,sizeof(bytes),&count,NULL) && !count && CloseHandle(file));
    next=one==65535?1:one+1;
    snprintf(collision,sizeof(collision),"%s%.3s%04x.tmp",path.bytes,prefix,next);
    file=CreateFileA(collision,GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);assert(file!=INVALID_HANDLE_VALUE);
    assert(WriteFile(file,"keep",4,&count,NULL) && count==4 && CloseHandle(file));
    errors(before);two=GetTempFileNameA(path.bytes,prefix,0,second.bytes);check_errors(before);
    assert(two==(next==65535?1:next+1) && strcmp(first.bytes,second.bytes));
    file=CreateFileA(collision,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);assert(file!=INVALID_HANDLE_VALUE);
    assert(ReadFile(file,bytes,sizeof(bytes),&count,NULL) && count==4 && !memcmp(bytes,"keep",4) && CloseHandle(file));
    file=CreateFileA(second.bytes,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);assert(file!=INVALID_HANDLE_VALUE);
    assert(WriteFile(file,"response",8,&count,NULL) && count==8 && CloseHandle(file));
    assert(!memcmp(first.left,"\245\245\245\245",4) && !memcmp(first.right,"\245\245\245\245",4));
    assert(!memcmp(second.left,"\245\245\245\245",4) && !memcmp(second.right,"\245\245\245\245",4));
    assert(DeleteFileA(first.bytes) && DeleteFileA(second.bytes) && DeleteFileA(collision));
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}
