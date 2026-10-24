/* Driver pass timing. The two original static records are _timeb objects. */
#include <sys/timeb.h>
#include "cl.h"
#include <recon.h>

static struct _timeb timer_started;
static struct _timeb timer_stopped;

/* 004053fd..0040540a */
void start_timer(void)
{
    _ftime(&timer_started);
}

/* 0040540a..00405495 */
void stop_timer(char *name)
{
    long seconds;
    int milliseconds, length;
    char digits[8];

    if (default_c_language)
        return;
    write_message(1, "time(%s)=", name);
    _ftime(&timer_stopped);
    seconds = timer_stopped.time - timer_started.time;
    milliseconds = timer_stopped.millitm - timer_started.millitm;
    if (milliseconds < 0) {
        milliseconds += 1000;
        --seconds;
    }
    strcpy(digits, "00");
    length = format_number(milliseconds, digits + 2, 10);
    write_message(1, "%d.%ss\n", seconds, digits + length - 1);
}
