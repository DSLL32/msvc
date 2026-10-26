#include <assert.h>
#include <limits.h>
#include <sys/mman.h>
#include <recon.h>
static int (__cdecl *format_number)(LPSTR,LPCSTR,...)=wsprintfA;
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
struct Vector {int value;const char *text;};
int main(int argc,char **argv)
{
    static const struct Vector vectors[]={{0,"0"},{10,"10"},{240,"240"},{-1,"-1"},{INT_MIN,"-2147483648"},{INT_MAX,"2147483647"}};
    char bytes[128];unsigned char before[108],after[108];unsigned row,n;int live=argc>1&&!strcmp(argv[1],"live");
    if(argc>1&&!live){
        if(!strcmp(argv[1],"null-output"))format_number(NULL,"%d");
        else if(!strcmp(argv[1],"null-format"))format_number(bytes,NULL);
        else if(!strcmp(argv[1],"output-wrap"))format_number((char *)0xfffffff8U,"%d");
        else if(!strcmp(argv[1],"format-wrap"))format_number(bytes,(char *)0xfffffffeU);
        else if(!strcmp(argv[1],"wide"))format_number(bytes,"%ws");
        else if(!strcmp(argv[1],"suffix"))format_number(bytes,"%d ");
        else if(!strcmp(argv[1],"other"))format_number(bytes,"%i");
        else if(!strcmp(argv[1],"overlap")){strcpy(bytes,"%d");format_number(bytes,bytes);}
        else assert(0);
        return 42;
    }
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(row=0;row<3;++row)for(n=0;n<sizeof(vectors)/sizeof(vectors[0]);++n){
        memset(bytes,0xa5,sizeof(bytes));errno=111*row;recon_doserrno=222*row;SetLastError(333*row);fp(before);
        assert(format_number(bytes+1,"%d",vectors[n].value)==(int)strlen(vectors[n].text));fp(after);
        assert(!strcmp(bytes+1,vectors[n].text) && (unsigned char)bytes[0]==0xa5 && (unsigned char)bytes[strlen(vectors[n].text)+2]==0xa5);
        assert(errno==111*row && recon_doserrno==222U*row && GetLastError()==333U*row && !memcmp(before,after,108));
    }
    {
        char *memory=mmap(NULL,4*4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0),*format,*output;
        assert(memory!=MAP_FAILED);assert(!mprotect(memory+4096,4096,PROT_NONE) && !mprotect(memory+12288,4096,PROT_NONE));
        format=memory+4093;output=memory+12288-12;memcpy(format,"%d",3);assert(!mprotect(memory,4096,PROT_READ));
        assert(format_number(output,format,INT_MIN)==11 && !strcmp(output,"-2147483648"));assert(!memcmp(format,"%d",3));
        assert(!munmap(memory,4*4096));
    }
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");return 0;
}
