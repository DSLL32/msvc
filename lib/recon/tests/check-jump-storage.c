#include <assert.h>
#include <recon.h>
struct Context { unsigned prefix; jmp_buf buffer; unsigned suffix; };
static struct Context state;
int main(int argc,char **argv)
{
    int value; (void)argv;
    if (argc>1) _assert("unvisited","source.c",401);
    assert(sizeof(jmp_buf)==RECON_LEGACY_JMP_WORDS*4);
    memset(&state,0xa5,sizeof(state));
    value=setjmp(state.buffer);
    if (!value) longjmp(state.buffer,9);
    assert(value==9 && state.prefix==0xa5a5a5a5 && state.suffix==0xa5a5a5a5);
    return 0;
}
