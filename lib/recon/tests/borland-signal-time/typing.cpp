#include <signal.h>
#ifdef RECON_BORLAND_400_DOS_TIME
#include <dos.h>
#endif
#include <recon.h>
#ifdef RECON_BORLAND_400_DOS_TIME
typedef char date_size[sizeof(struct date)==6?1:-1];
typedef char date_align[__alignof__(struct date)==1?1:-1];
typedef char date_day[offsetof(struct date,da_day)==4?1:-1];
typedef char date_month[offsetof(struct date,da_mon)==5?1:-1];
typedef char time_size[sizeof(struct time)==4?1:-1];
typedef char time_align[__alignof__(struct time)==1?1:-1];
typedef char time_hour[offsetof(struct time,ti_hour)==1?1:-1];
typedef char time_hund[offsetof(struct time,ti_hund)==2?1:-1];
typedef char time_sec[offsetof(struct time,ti_sec)==3?1:-1];
struct packing_restored {char c;int value;};
typedef char pack_restored[offsetof(struct packing_restored,value)==4?1:-1];
static void (__cdecl *date_call)(struct date *)=getdate;
static void (__cdecl *time_call)(struct time *)=gettime;
#endif
static ReconSignalHandler (__cdecl *signal_call)(int,ReconSignalHandler)=signal;
void type_check(void) {(void)signal_call;
#ifdef RECON_BORLAND_400_DOS_TIME
(void)date_call;(void)time_call;
#endif
}
