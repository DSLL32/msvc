#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static unsigned delivered;
static void first(int number){(void)number;++delivered;}
static void second(int number){(void)number;delivered+=2;}
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static void registration(int number,ReconSignalHandler handler,ReconSignalHandler expected,unsigned row)
{
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,333}};
    unsigned char before[108],after[108];
    errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
    fp(before);assert(recon_watcom_signal(number,handler)==expected);fp(after);
    assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] && recon_GetLastError()==errors[row][2]);
    assert(!memcmp(before,after,108) && !delivered);
}
int main(int argc,char **argv)
{
    static const int numbers[]={1,4,6,7};
    static const int host_numbers[]={SIGINT,SIGTERM,SIGABRT,SIGQUIT};
    struct sigaction original[4],after;unsigned row,index,bit;
    int live=argc>1 && !strcmp(argv[1],"live");
    if(argc>1 && !live) {
        if(!strcmp(argv[1],"fpe")){recon_watcom_signal(2,first);return 42;}
        if(!strcmp(argv[1],"ill")){recon_watcom_signal(3,first);return 42;}
        if(!strcmp(argv[1],"negative")){recon_watcom_signal(-1,first);return 42;}
        if(!strcmp(argv[1],"zero")){recon_watcom_signal(0,first);return 42;}
        if(!strcmp(argv[1],"large")){recon_watcom_signal(13,first);return 42;}
        if(!strcmp(argv[1],"null")){recon_watcom_signal(4,NULL);return 42;}
        if(!strcmp(argv[1],"err")){recon_watcom_signal(4,(ReconSignalHandler)3);return 42;}
        assert(!strcmp(argv[1],"raise"));recon_watcom_raise(4);return 42;
    }
    for(index=0;index<4;++index)assert(!sigaction(host_numbers[index],NULL,original+index));
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(row=0;row<3;++row)for(index=0;index<4;++index) {
        registration(numbers[index],first,(ReconSignalHandler)(row?2:index?1:2),row);
        registration(numbers[index],second,first,row);
        registration(numbers[index],(ReconSignalHandler)1,second,row);
        registration(numbers[index],(ReconSignalHandler)2,(ReconSignalHandler)1,row);
    }
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    for(index=0;index<4;++index) {
        assert(!sigaction(host_numbers[index],NULL,&after));
        assert(after.sa_handler==original[index].sa_handler && after.sa_flags==original[index].sa_flags);
        for(bit=1;bit<NSIG;++bit)assert(sigismember(&after.sa_mask,bit)==sigismember(&original[index].sa_mask,bit));
    }
    assert(!delivered);return 0;
}
