#define _GNU_SOURCE
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <float.h>
#include <fcntl.h>
#include <share.h>
#include <sys/timeb.h>
#include <recon.h>

unsigned recon_control87(unsigned, unsigned), recon_controlfp(unsigned, unsigned);
void recon_ftime(struct _timeb *);
size_t recon_wcslen(const WCHAR *);
WCHAR *recon_wcscpy(WCHAR *, const WCHAR *);
WCHAR *recon_wcsncpy(WCHAR *, const WCHAR *, size_t);

/* Strong test overrides verify that captured local time reaches the target's
 * timezone boundary instead of being interpreted with the host's DST rules. */
static int target_clock_active, target_dst, target_daylight;
static long target_timezone;
static unsigned timezone_calls, dst_calls;
static struct tm first_dst_calendar;

void recon_tzset(void)
{
    ++timezone_calls;
    if (target_clock_active) {
        recon_timezone = target_timezone;
        recon_daylight = target_daylight;
    } else {
        tzset();
        recon_timezone = timezone;
        recon_daylight = daylight;
    }
}

int recon_isindst(struct tm *calendar)
{
    if (!dst_calls++) first_dst_calendar = *calendar;
    return target_clock_active ? target_dst : calendar->tm_isdst > 0;
}

static int *outer_context;

static void jump_from_nested_call(int *context, int value)
{
    volatile unsigned padding[128];
    unsigned index;
    for (index = 0; index < 128; ++index) padding[index] = index;
    assert(padding[127] == 127);
    longjmp(context, value);
}

static void nested_recovery(void)
{
    jmp_buf inner;
    if (!setjmp(inner)) jump_from_nested_call(inner, 0);
    else jump_from_nested_call(outer_context, -7);
}

static void check_contexts(void)
{
    struct {
        unsigned before;
        jmp_buf context;
        unsigned after;
    } guarded;
    volatile unsigned visits = 0;
    int result;
    assert(sizeof(jmp_buf) == 32);
    guarded.before = 0x11223344;
    guarded.after = 0xaabbccdd;
    outer_context = guarded.context;
    result = setjmp(guarded.context);
    if (!result) nested_recovery();
    assert(result == -7);
    assert(guarded.before == 0x11223344 && guarded.after == 0xaabbccdd);
    /* Save repeatedly at the same legacy address, with zero normalized to 1. */
    while (visits < 40) {
        result = setjmp(guarded.context);
        if (!result) jump_from_nested_call(guarded.context, 0);
        assert(result == 1);
        ++visits;
    }
}

static void check_controls(void)
{
    unsigned short saved, before, after;
    unsigned abstract;
    __asm__ volatile ("fnstcw %0" : "=m" (saved));
    recon_control87(_MCW_EM, _MCW_EM);
    recon_control87(0, _EM_OVERFLOW);
    __asm__ volatile ("fnstcw %0" : "=m" (before));
    assert(!(before & 8));
    abstract = recon_control87(4, 4);
    __asm__ volatile ("fnstcw %0" : "=m" (after));
    assert(after == (before | 8));
    assert((abstract & _MCW_EM) == _MCW_EM);
    assert((recon_control87(_PC_53 | _RC_DOWN, _MCW_PC | _MCW_RC) &
        (_MCW_PC | _MCW_RC)) == (_PC_53 | _RC_DOWN));
    __asm__ volatile ("fnstcw %0" : "=m" (after));
    assert((after & 0xf00) == 0x600);
    assert((recon_control87(_PC_24 | _RC_UP, _MCW_PC | _MCW_RC) &
        (_MCW_PC | _MCW_RC)) == (_PC_24 | _RC_UP));
    assert(recon_controlfp(0, _EM_DENORMAL) & _EM_DENORMAL);
    recon_control87(0, _EM_DENORMAL);
    assert(!(recon_controlfp(_EM_DENORMAL, _EM_DENORMAL) & _EM_DENORMAL));
    __asm__ volatile ("fldcw %0" : : "m" (saved));
}

static void check_time(void)
{
    struct _timeb value;
    assert(sizeof(value) == 12);
    assert(offsetof(struct _timeb, millitm) == 4);
    assert(offsetof(struct _timeb, timezone) == 6);
    assert(offsetof(struct _timeb, dstflag) == 8);
    unsetenv("RECON_LOCAL_CLOCK");
    setenv("TZ", "UTC0", 1);
    setenv("RECON_FIXED_TIME", "1704067200", 1);
    unsetenv("RECON_FIXED_MILLISECONDS");
    recon_ftime(&value);
    assert(value.time == 1704067200 && value.millitm == 0);
    assert(value.timezone == 0 && value.dstflag == 0);
    setenv("RECON_FIXED_MILLISECONDS", "731", 1);
    recon_ftime(&value);
    assert(value.millitm == 731);
    setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
    setenv("RECON_FIXED_TIME", "1719792000", 1);
    recon_ftime(&value);
    assert(value.timezone == 300 && value.dstflag == 1);
    unsetenv("RECON_FIXED_TIME");
    unsetenv("RECON_FIXED_MILLISECONDS");
    recon_ftime(&value);
    assert(value.millitm < 1000 && value.time > 0);
}

static void check_local_clock(void)
{
    static const unsigned short milliseconds[4] = { 900, 125, 350, 575 };
    static const int seconds[4] = { 0, 2, 3, 4 };
    struct _timeb value;
    unsigned index;
    target_clock_active = 1;
    target_timezone = 19815; /* +05:30:15; timeb truncates to whole minutes. */
    target_daylight = 1;
    target_dst = 0;
    timezone_calls = dst_calls = 0;
    setenv("TZ", "EST5EDT", 1);
    setenv("RECON_LOCAL_CLOCK", "2026,1,15,0", 1);
    for (index = 0; index < 4; ++index) {
        recon_ftime(&value);
        assert(value.time == 1768435200L + 19815 + seconds[index]);
        assert(value.millitm == milliseconds[index]);
        assert(value.timezone == 330 && value.dstflag == 0);
    }
    assert(timezone_calls == 4 && dst_calls == 4);
    assert(first_dst_calendar.tm_year == 126 && first_dst_calendar.tm_mon == 0);
    assert(first_dst_calendar.tm_mday == 15 && first_dst_calendar.tm_yday == 14);
    /* A strong predicate can disagree with the host's summer DST policy. */
    setenv("RECON_LOCAL_CLOCK", "2026-07-15T03:04:05.731/0", 1);
    recon_ftime(&value);
    assert(value.time == 1784084645L + 19815 && value.millitm == 731);
    assert(value.dstflag == 0);
    target_dst = 1;
    timezone_calls = dst_calls = 0;
    recon_ftime(&value);
    assert(value.time == 1784084645L + 19815 - 3600 && value.dstflag == 1);
    assert(timezone_calls == 1 && dst_calls == 2);
    target_timezone = -13520; /* -03:45:20, preserving the seconds. */
    target_daylight = 0;
    dst_calls = 0;
    recon_ftime(&value);
    assert(value.time == 1784084645L - 13520 && value.timezone == -225);
    assert(value.dstflag == 0 && dst_calls == 0);
    setenv("RECON_LOCAL_CLOCK", "1960,7,15,0", 1);
    recon_ftime(&value);
    assert(value.time == -1 && value.dstflag == -1 && value.millitm == 900);
    recon_ftime(&value);
    assert(value.time == -1 && value.dstflag == -1 && value.millitm == 125);
    /* Leaving captured mode resets the sequence when the same input returns. */
    unsetenv("RECON_LOCAL_CLOCK");
    recon_ftime(&value);
    setenv("RECON_LOCAL_CLOCK", "1960,7,15,0", 1);
    recon_ftime(&value);
    assert(value.millitm == 900);
    unsetenv("RECON_LOCAL_CLOCK");
    target_clock_active = 0;
}

static void check_wide(void)
{
    static const WCHAR text[] = { 'A', 0x20ac, 0xd800, 0xdc00, 0 };
    WCHAR copy[8];
    assert(sizeof(WCHAR) == 2);
    assert(recon_wcslen(text) == 4);
    memset(copy, 0x55, sizeof(copy));
    assert(recon_wcscpy(copy, text) == copy);
    assert(!memcmp(copy, text, sizeof(text)) && copy[5] == 0x5555);
    assert(recon_wcsncpy(copy, text, 2) == copy);
    assert(copy[0] == 'A' && copy[1] == 0x20ac && copy[2] == 0xd800);
    recon_wcsncpy(copy, text, 8);
    assert(!memcmp(copy, text, sizeof(text)));
    assert(copy[5] == 0 && copy[6] == 0 && copy[7] == 0);
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--clock")) {
        struct _timeb value;
        recon_ftime(&value);
        printf("%ld %u %d %d\n", value.time, value.millitm, value.timezone, value.dstflag);
        return 0;
    }
    assert(_O_CREAT == 0x100 && _O_APPEND == 8 && _O_BINARY == 0x8000);
    assert(_SH_DENYWR == 0x20 && _SH_DENYNO == 0x40);
    check_contexts();
    check_controls();
    check_time();
    check_local_clock();
    check_wide();
    return 0;
}
