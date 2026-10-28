#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static int target_error,calls;
int *recon_errno(void) {return &target_error;}
static ULONG WINAPI first(EXCEPTION_RECORD *a,void *b,CONTEXT *c,void *d)
{(void)a;(void)b;(void)c;(void)d;++calls;__builtin_trap();}
static ULONG WINAPI second(EXCEPTION_RECORD *a,void *b,CONTEXT *c,void *d)
{(void)a;(void)b;(void)c;(void)d;++calls;__builtin_trap();}
#ifdef RECON_BORLAND_400_USER_HANDLER
extern ReconBorland400UserHandler consumer_set(ReconBorland400UserHandler);
#endif
static void fp(unsigned char state[108])
{__asm__ volatile("fnsave %0;frstor %0":"=m"(*(unsigned char (*)[108])state));}
int main(int argc,char **argv)
{
#ifdef RECON_BORLAND_400_USER_HANDLER
    ReconBorland400UserHandler initial,gp;unsigned char before[108],after[108];
    __asm__ volatile("fld1;fldpi":::"st");fp(before);
    errno=E2BIG;target_error=123;recon_doserrno=456;recon_SetLastError(789);
    initial=consumer_set(first);assert(initial && initial!=first && initial!=second);
    if(argc>1 && !strcmp(argv[1],"initial"))return initial((void *)1,(void *)2,(void *)3,(void *)4);
    assert(recon_borland_400_SetUserHandler(second)==first);
    assert(consumer_set(second)==second);
    assert(recon_borland_400_SetUserHandler(NULL)==second);
    assert(consumer_set(NULL)==NULL);
    assert(recon_borland_400_SetUserHandler(initial)==NULL);
    assert(consumer_set(first)==initial);
    recon_borland_400_initialize_gp_handler();
    gp=consumer_set(second);assert(gp && gp!=initial && gp!=first && gp!=second);
    if(argc>1 && !strcmp(argv[1],"gp"))return gp((void *)1,(void *)2,(void *)3,(void *)4);
    assert(consumer_set(gp)==second);
    recon_borland_400_initialize_gp_handler();assert(consumer_set(first)==gp);
    assert(consumer_set(initial)==first);assert(consumer_set(initial)==initial);
    fp(after);assert(!memcmp(before,after,2) && !memcmp(before+4,after+4,2) && !memcmp(before+8,after+8,2) && !memcmp(before+28,after+28,20));
    assert(!calls && errno==E2BIG && target_error==123 && recon_doserrno==456 && recon_GetLastError()==789);
    __asm__ volatile("fstp %%st(0);fstp %%st(0)":::"st");return 0;
#else
    if(argc>1)recon_borland_400_initialize_gp_handler();
    else recon_borland_400_SetUserHandler(first);
    return 1;
#endif
}
