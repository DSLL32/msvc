#include <assert.h>
#include <recon.h>
extern void test_host_longjmp(recon_host_jmp_buf,int) __asm__("longjmp") __attribute__((noreturn));
static jmp_buf saved;
int main(int argc,char **argv)
{
    int result; unsigned n; recon_host_jmp_buf *context;
    if (argc>1 && !strcmp(argv[1],"acp")) GetACP();
    if (argc>1 && !strcmp(argv[1],"valid")) IsValidCodePage(1252);
    if (argc>1 && !strcmp(argv[1],"next")) CharNextA("x");
    memset(saved,0xa5,sizeof(saved));
    if (argc>1 && !strcmp(argv[1],"count")) _setjmp3(saved,1);
    if (argc>1 && !strcmp(argv[1],"size")) recon_setjmp3_context(saved,32,0);
    result=setjmp(saved);
    if (argc>1 && !strcmp(argv[1],"longjmp")) longjmp(saved,7);
    if (!result) {
        context=recon_setjmp3_context(saved,sizeof(saved),0);
        test_host_longjmp(*context,7);
    }
    assert(result==7);
    for (n=0;n<sizeof(saved)/sizeof(saved[0]);++n) assert((unsigned)saved[n]==0xa5a5a5a5);
    return 0;
}
