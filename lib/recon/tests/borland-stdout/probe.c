#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <recon.h>
static int error_slot;
int *recon_errno(void) {return &error_slot;}
extern int consumer_print(const char *);
extern void consumer_quick_exit(int);
extern int (*consumer_binding(void))(const char *,...);
static void final_callback(void) {write(2,"atexit\n",7);}
static void fp(unsigned char v[108]) {__asm__ volatile("fnsave %0;frstor %0":"=m"(*(unsigned char (*)[108])v));}
int main(int argc,char **argv)
{
#ifdef RECON_BORLAND_400_STDOUT
    unsigned n,k;char *text;size_t capacity;int code;unsigned char a[108],b[108];
    assert(argc>=3 && consumer_binding()==recon_borland_400_printf);assert(!atexit(final_callback));code=(int)strtoul(argv[2],NULL,0);
    if(!strcmp(argv[1],"write-error")) {
        assert(consumer_print("actual output error\n")==EOF);
        assert(recon_ferror(&recon_iob[1]) && errno!=0);
        assert(consumer_print("retry\n")==EOF && recon_ferror(&recon_iob[1]));
        consumer_quick_exit(0);
    }
    if(!strcmp(argv[1],"closed")) {
        char text[561];memset(text,'x',560);text[560]=0;assert(!close(1));
        assert(consumer_print(text)==EOF && recon_ferror(&recon_iob[1]) && errno==EBADF);
        consumer_quick_exit(0);
    }
    if(!strcmp(argv[1],"format")) {recon_borland_400_printf("%d",7);return 1;}
    if(!strcmp(argv[1],"null")) {consumer_print(NULL);return 1;}
    if(!strcmp(argv[1],"wrap")) {consumer_print((char *)0xfffffff0UL);return 1;}
    if(!strcmp(argv[1],"unterminated")) {text=mmap(NULL,65536,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(text!=MAP_FAILED);memset(text,'a',65536);consumer_print(text);return 1;}
    if(!strcmp(argv[1],"dirty")) {recon_printf("%s","old");consumer_print("new");return 1;}
    if(!strcmp(argv[1],"state") || !strcmp(argv[1],"overlap")) {
        consumer_print("");if(argv[1][0]=='s')++recon_iob[1]._ptr;
        else {recon_iob[1]._base[0]='x';recon_iob[1]._base[1]=0;consumer_print(recon_iob[1]._base);return 1;}
        consumer_print("new");return 1;
    }
    __asm__ volatile("fld1;fldpi":::"st");
    for(k=3;k<(unsigned)argc;++k) {
        n=(unsigned)strtoul(argv[k],NULL,0);capacity=((n+1+4095)/4096)*4096;
        text=mmap(NULL,capacity,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(text!=MAP_FAILED);
        {unsigned i;for(i=0;i<n;++i)text[i]="A%\nB\r\n"[i%6];}text[n]=0;assert(!mprotect(text,capacity,PROT_READ));
        errno=E2BIG;error_slot=123;recon_doserrno=456;recon_SetLastError(789);fp(a);
        assert(consumer_print(text)==(int)n);fp(b);
        assert(!memcmp(a,b,2) && !memcmp(a+4,b+4,2) && !memcmp(a+8,b+8,2) && !memcmp(a+28,b+28,20));
        assert(errno==E2BIG && error_slot==123 && recon_doserrno==456 && recon_GetLastError()==789);assert(!munmap(text,capacity));
    }
    __asm__ volatile("fstp %%st(0);fstp %%st(0)":::"st");
    if(!strcmp(argv[1],"normal")) {recon_record_terminal_status((uint32_t)code);return code;}
    if(!strcmp(argv[1],"flush"))assert(!recon_fflush(&recon_iob[1]));
    consumer_quick_exit(code);return 1;
#else
    assert(consumer_binding()==recon_printf);
    if(argc>1 && !strcmp(argv[1],"quick"))recon_borland_400_quick_exit(0);
    recon_borland_400_printf("%s","text");return 1;
#endif
}
