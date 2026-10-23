#include <assert.h>
#include <setjmp.h>
#include <recon.h>
static void snapshot(unsigned char *bytes)
{ __asm__ volatile("fnsave (%0); frstor (%0)"::"r"(bytes):"memory"); }
static int save_in_live_frame(const char *mode)
{
    struct { unsigned prefix;jmp_buf owner;unsigned suffix; } state;
    unsigned char saved[sizeof(state)],before[108],after[108];
    unsigned int *pointer=state.owner;int result;
    int (*indirect)(unsigned int *)=_setjmp;
    memset(&state,0x5a,sizeof(state));memcpy(saved,&state,sizeof(state));
    assert(sizeof(state.owner)==52);
    if(!strcmp(mode,"null"))recon_watcom_setjmp_context(NULL,52);
    if(!strcmp(mode,"size"))recon_watcom_setjmp_context(state.owner,48);
    if(!strcmp(mode,"pointer"))return _setjmp(pointer);
    if(!strcmp(mode,"indirect"))return indirect(state.owner);
    errno=111;recon_doserrno=77;SetLastError(0x12345678);
    __asm__ volatile("fld1");snapshot(before);
    result=setjmp(state.owner);
    snapshot(after);
    assert(result==0 && !memcmp(before,after,108));
    assert(!memcmp(saved,&state,sizeof(state)));
    assert(errno==111 && recon_doserrno==77 && GetLastError()==0x12345678);
    if(!strcmp(mode,"longjmp"))longjmp(state.owner,9);
    if(!strcmp(mode,"generic-longjmp"))recon_longjmp(state.owner,9);
    __asm__ volatile("fstp %%st(0)":::"st");
    return result;
}
int main(int argc,char **argv)
{ return save_in_live_frame(argc>1?argv[1]:"normal"); }
