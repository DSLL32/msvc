#include <signal.h>
#ifdef RECON_BORLAND_400_DOS_TIME
#include <dos.h>
#endif
#include <recon.h>
ReconSignalHandler consumer_signal(int number,ReconSignalHandler handler) {return signal(number,handler);}
ReconSignalHandler (*consumer_binding(void))(int,ReconSignalHandler) {return signal;}
#ifdef RECON_BORLAND_400_DOS_TIME
void consumer_date(struct date *value) {getdate(value);}
void consumer_time(struct time *value) {gettime(value);}
#else
#ifdef getdate
#error unselected Borland getdate alias
#endif
#ifdef gettime
#error unselected Borland gettime alias
#endif
#endif
