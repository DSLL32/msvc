#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static int target_error;
int *recon_errno(void) {return &target_error;}
#ifdef RECON_BORLAND_400_DOS_TIME
extern void consumer_date(struct date *);
extern void consumer_time(struct time *);
static void fp(unsigned char v[108]) {__asm__ volatile("fnsave %0;frstor %0":"=m"(*(unsigned char (*)[108])v));}
static int same_date(struct date *date,const struct timespec *instant)
{struct tm local;assert(localtime_r(&instant->tv_sec,&local));return date->da_year==local.tm_year+1900 && date->da_mon==local.tm_mon+1 && date->da_day==local.tm_mday;}
static unsigned centiseconds(const struct timespec *instant)
{struct tm local;assert(localtime_r(&instant->tv_sec,&local));return (local.tm_hour*3600+local.tm_min*60+local.tm_sec)*100+instant->tv_nsec/10000000;}
static unsigned distance(unsigned a,unsigned b) {return (a+8640000-b)%8640000;}
#endif
int main(int argc,char **argv)
{
    if(argc>1) {
        if(!strcmp(argv[1],"null-date"))recon_borland_400_getdate(NULL);
        else if(!strcmp(argv[1],"null-time"))recon_borland_400_gettime(NULL);
        else if(!strcmp(argv[1],"wrap-date"))recon_borland_400_getdate((void *)0xfffffffcUL);
        else if(!strcmp(argv[1],"wrap-time"))recon_borland_400_gettime((void *)0xfffffffeUL);
        else assert(0);
        return 1;
    }
#ifdef RECON_BORLAND_400_DOS_TIME
    {
        unsigned sample,n,seen_nonzero=0;unsigned char guarded_date[10],guarded_time[8],before_fp[108],after_fp[108];
        struct date *date=(void *)(guarded_date+1);struct time *time=(void *)(guarded_time+1);
        struct timespec before,after,initial,delay={0,20000000};unsigned low,high,actual;
        int fmode=recon_fmode;
        assert(sizeof(*date)==6 && sizeof(*time)==4 && __alignof__(*date)==1 && __alignof__(*time)==1);
        tzset();assert(!clock_gettime(CLOCK_REALTIME,&initial));
        /* Ensure a zero-millisecond implementation cannot pass merely because
         * this small test happened to begin in the first hundredth of a second. */
        if(initial.tv_nsec<20000000)assert(!nanosleep(&delay,NULL));
        __asm__ volatile("fld1;fldpi":::"st");
        for(sample=0;sample<16;++sample) {
            memset(guarded_date,0xa5,sizeof(guarded_date));memset(guarded_time,0xa5,sizeof(guarded_time));
            assert(!clock_gettime(CLOCK_REALTIME,&before));errno=E2BIG;target_error=123;recon_doserrno=456;recon_SetLastError(789);fp(before_fp);
            consumer_date(date);fp(after_fp);
            assert(errno==E2BIG && target_error==123 && recon_doserrno==456 && recon_GetLastError()==789);
            assert(!memcmp(before_fp,after_fp,2) && !memcmp(before_fp+4,after_fp+4,2) && !memcmp(before_fp+8,after_fp+8,2) && !memcmp(before_fp+28,after_fp+28,20));
            assert(!clock_gettime(CLOCK_REALTIME,&after));assert(same_date(date,&before) || same_date(date,&after));
            assert(guarded_date[0]==0xa5);for(n=7;n<sizeof(guarded_date);++n)assert(guarded_date[n]==0xa5);
            assert(!clock_gettime(CLOCK_REALTIME,&before));errno=E2BIG;target_error=123;recon_doserrno=456;recon_SetLastError(789);fp(before_fp);
            consumer_time(time);fp(after_fp);
            assert(errno==E2BIG && target_error==123 && recon_doserrno==456 && recon_GetLastError()==789);
            assert(!memcmp(before_fp,after_fp,2) && !memcmp(before_fp+4,after_fp+4,2) && !memcmp(before_fp+8,after_fp+8,2) && !memcmp(before_fp+28,after_fp+28,20));
            assert(!clock_gettime(CLOCK_REALTIME,&after));
            assert(time->ti_hour<=23 && time->ti_min<=59 && time->ti_sec<=59 && time->ti_hund<=99);
            low=centiseconds(&before);high=centiseconds(&after);actual=(time->ti_hour*3600+time->ti_min*60+time->ti_sec)*100+time->ti_hund;
            assert(distance(actual,low)<=distance(high,low));if(time->ti_hund)seen_nonzero=1;
            assert(guarded_time[0]==0xa5);for(n=5;n<sizeof(guarded_time);++n)assert(guarded_time[n]==0xa5);
        }
        assert(seen_nonzero && recon_fmode==fmode);__asm__ volatile("fstp %%st(0);fstp %%st(0)":::"st");return 0;
    }
#else
    recon_borland_400_getdate(NULL);return 1;
#endif
}
