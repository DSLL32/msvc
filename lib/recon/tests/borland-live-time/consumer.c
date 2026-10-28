#ifdef RECON_BORLAND_400_DOS_TIME
#include <dos.h>
#endif
#include <recon.h>
#ifdef RECON_BORLAND_400_DOS_TIME
void consumer_date(struct date *value) {getdate(value);}
void consumer_time(struct time *value) {gettime(value);}
#else
#ifdef getdate
#error unselected getdate remap
#endif
#ifdef gettime
#error unselected gettime remap
#endif
#endif
