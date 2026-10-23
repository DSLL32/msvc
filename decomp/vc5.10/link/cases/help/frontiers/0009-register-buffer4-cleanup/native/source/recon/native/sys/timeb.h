#ifndef RECON_NATIVE_SYS_TIMEB_H
#define RECON_NATIVE_SYS_TIMEB_H
#include_next <sys/timeb.h>
#include <time.h>
/* VC1's time_t is a signed 32-bit long, including when host time_t changes. */
struct _timeb {
    long time;
    unsigned short millitm;
    short timezone;
    short dstflag;
};
typedef char ReconTimebIs12[(sizeof(struct _timeb) == 12) ? 1 : -1];
void _ftime(struct _timeb *);
#endif
