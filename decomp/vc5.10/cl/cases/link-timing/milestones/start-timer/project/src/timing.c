/* Driver pass timing. The two original static records are _timeb objects. */
#include <sys/timeb.h>
#include "cl.h"
#include <recon.h>

static struct _timeb timer_started;

/* 004053fd..0040540a */
void start_timer(void)
{
    _ftime(&timer_started);
}
