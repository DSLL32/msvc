#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <float.h>
#include <setjmp.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <recon.h>

/* Build surfaces for the distinct Watcom CRT. No heap, signal delivery or
 * nonlocal-jump semantics have been established by a reached case yet. */
int recon_watcom_heapchk(void)
{ recon_unsupported("unreviewed Watcom _heapchk"); return 0; }
int recon_watcom_nheapchk(void)
{ recon_unsupported("unreviewed Watcom _nheapchk"); return 0; }
ReconSignalHandler recon_watcom_signal(int number,ReconSignalHandler handler)
{ (void)number; (void)handler; recon_unsupported("unreviewed Watcom signal"); return NULL; }
int recon_watcom_raise(int number)
{ (void)number; recon_unsupported("unreviewed Watcom raise"); return 0; }
int recon_watcom_setjmp(unsigned int *environment)
{ (void)environment; recon_unsupported("unreviewed Watcom _setjmp"); return 0; }
void recon_watcom_longjmp(unsigned int *environment,int value)
{ (void)environment; (void)value; recon_unsupported("unreviewed Watcom longjmp"); abort(); }

/* These contexts are external to the 32-byte legacy object, whose address is
 * also stored as an int pointer by the C++ frontend. Addresses may be reused:
 * each new setjmp overwrites the previous saved context at that address.
 * Like the existing tracing runtime, this registry serves single-threaded
 * fixtures. Entries cannot be freed because C has no jump-buffer destructor. */
struct JumpContext {
    void *legacy;
    int opaque_setjmp3;
    recon_host_jmp_buf host;
    struct JumpContext *next;
};
static struct JumpContext *jump_contexts;

recon_host_jmp_buf *recon_jmp_context(void *legacy)
{
    struct JumpContext *context;
    for (context = jump_contexts; context; context = context->next)
        if (context->legacy == legacy) { context->opaque_setjmp3=0; return &context->host; }
    context = malloc(sizeof(*context));
    if (!context) recon_unsupported("setjmp context allocation");
    context->legacy = legacy;
    context->opaque_setjmp3=0;
    context->next = jump_contexts;
    jump_contexts = context;
    return &context->host;
}

recon_host_jmp_buf *recon_setjmp3_context(void *legacy,unsigned size,int count)
{
    struct JumpContext *context; recon_host_jmp_buf *host;
    if (!legacy || size!=64 || count) recon_unsupported("unreviewed _setjmp3 storage/count/unwind");
    host=recon_jmp_context(legacy);
    for (context=jump_contexts;context;context=context->next)
        if (context->legacy==legacy) { context->opaque_setjmp3=1; break; }
    return host;
}

void recon_longjmp(void *legacy, int value)
{
    struct JumpContext *context;
    for (context = jump_contexts; context; context = context->next)
        if (context->legacy == legacy) {
            if (context->opaque_setjmp3) recon_unsupported("unreviewed _setjmp3 longjmp/SEH unwind");
            longjmp(context->host, value);
        }
    recon_unsupported("longjmp without matching setjmp");
    abort();
}

static unsigned abstract_control(unsigned short control)
{
    static const unsigned precision[4] = { _PC_24, _MCW_PC, _PC_53, _PC_64 };
    unsigned result = precision[(control >> 8) & 3];
    if (control & 0x01) result |= _EM_INVALID;
    if (control & 0x02) result |= _EM_DENORMAL;
    if (control & 0x04) result |= _EM_ZERODIVIDE;
    if (control & 0x08) result |= _EM_OVERFLOW;
    if (control & 0x10) result |= _EM_UNDERFLOW;
    if (control & 0x20) result |= _EM_INEXACT;
    result |= (control & 0x0c00) >> 2;
    result |= (control & 0x1000) << 6;
    return result;
}

unsigned recon_control87(unsigned value, unsigned mask)
{
    static const unsigned short precision[4] = { 0x300, 0x200, 0, 0x100 };
    unsigned short control, updated;
    unsigned desired;
    /* x87 control state has no ordinary C interface. Only these transfers use
     * assembly; translation of Microsoft's flags remains ordinary C. */
    __asm__ volatile ("fnstcw %0" : "=m" (control));
    desired = (abstract_control(control) & ~mask) | (value & mask);
    updated = control & ~0x1f3f;
    if (desired & _EM_INVALID) updated |= 0x01;
    if (desired & _EM_DENORMAL) updated |= 0x02;
    if (desired & _EM_ZERODIVIDE) updated |= 0x04;
    if (desired & _EM_OVERFLOW) updated |= 0x08;
    if (desired & _EM_UNDERFLOW) updated |= 0x10;
    if (desired & _EM_INEXACT) updated |= 0x20;
    updated |= precision[(desired & _MCW_PC) >> 16];
    updated |= (desired & _MCW_RC) << 2;
    updated |= (desired & _MCW_IC) >> 6;
    if (updated != control) __asm__ volatile ("fldcw %0" : : "m" (updated));
    return abstract_control(updated);
}

unsigned recon_controlfp(unsigned value, unsigned mask)
{
    /* Unlike _control87, _controlfp leaves the denormal mask unchanged. */
    return recon_control87(value, mask & ~_EM_DENORMAL);
}

static char *local_clock_configuration;
static uint64_t local_clock_calls;

static unsigned clock_field(const char **cursor, int separator)
{
    const char *start = *cursor;
    char *end;
    unsigned long value;
    if (*start < '0' || *start > '9') recon_unsupported("RECON_LOCAL_CLOCK field");
    errno = 0;
    value = strtoul(start, &end, 10);
    if (errno || value > 0x7fffffffUL || *end != separator)
        recon_unsupported("RECON_LOCAL_CLOCK syntax");
    *cursor = end + (separator != 0);
    return (unsigned)value;
}

static int calendar_leap(int year)
{
    return !(year % 4) && ((year % 100) || !(year % 400));
}

static unsigned calendar_month_days(int year, unsigned month)
{
    static const unsigned days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    return days[month - 1] + (month == 2 && calendar_leap(year));
}

static int64_t calendar_seconds(int year, unsigned month, unsigned day,
    unsigned hour, unsigned minute, unsigned second)
{
    int current;
    unsigned index;
    int64_t days = day - 1;
    for (current = 1970; current < year; ++current) days += 365 + calendar_leap(current);
    for (current = year; current < 1970; ++current) days -= 365 + calendar_leap(current);
    for (index = 1; index < month; ++index) days += calendar_month_days(year, index);
    return ((days * 24 + hour) * 60 + minute) * 60 + second;
}

static void calendar_breakdown(int64_t seconds, struct tm *calendar)
{
    int64_t days = seconds / 86400, within_day = seconds % 86400;
    int weekday, year = 1970, month = 1;
    unsigned month_days;
    if (within_day < 0) { within_day += 86400; --days; }
    weekday = (int)((days + 4) % 7);
    if (weekday < 0) weekday += 7;
    memset(calendar, 0, sizeof(*calendar));
    while (days < 0) { --year; days += 365 + calendar_leap(year); }
    while (days >= 365 + calendar_leap(year)) {
        days -= 365 + calendar_leap(year);
        ++year;
    }
    calendar->tm_yday = (int)days;
    while (days >= (month_days = calendar_month_days(year, month))) {
        days -= month_days;
        ++month;
    }
    calendar->tm_year = year - 1900;
    calendar->tm_mon = month - 1;
    calendar->tm_mday = (int)days + 1;
    calendar->tm_wday = weekday;
    calendar->tm_hour = within_day / 3600;
    calendar->tm_min = (within_day % 3600) / 60;
    calendar->tm_sec = within_day % 60;
}

static int local_clock_dst(int64_t local_seconds, int64_t utc_seconds)
{
    struct tm calendar, hint;
    time_t host_seconds = (time_t)utc_seconds;
    calendar_breakdown(local_seconds, &calendar);
    /* The weak shared _isindst consumes tm_isdst; give it a host hint.
     * Reconstructed strong versions inspect this calendar themselves, keeping
     * their historical transition rules independent of the host's rules. */
    tzset();
    if (localtime_r(&host_seconds, &hint)) calendar.tm_isdst = hint.tm_isdst;
    return recon_daylight && recon_isindst(&calendar);
}

static void captured_local_ftime(struct _timeb *result, const char *configuration)
{
    const char *cursor = configuration;
    unsigned year, month, day, hour, minute = 0, second = 0, initial = 900, step = 1225;
    uint64_t elapsed;
    int64_t local_seconds, utc_seconds;
    int dst;
    if (strchr(configuration, ',')) {
        year = clock_field(&cursor, ',');
        month = clock_field(&cursor, ',');
        day = clock_field(&cursor, ',');
        hour = clock_field(&cursor, 0);
    } else {
        year = clock_field(&cursor, '-');
        month = clock_field(&cursor, '-');
        day = clock_field(&cursor, 'T');
        hour = clock_field(&cursor, ':');
        minute = clock_field(&cursor, ':');
        second = clock_field(&cursor, '.');
        initial = clock_field(&cursor, '/');
        step = clock_field(&cursor, 0);
    }
    if (year < 1 || year > 9999 || month < 1 || month > 12 || day < 1 ||
        day > calendar_month_days(year, month) || hour > 23 || minute > 59 ||
        second > 59 || initial > 999)
        recon_unsupported("RECON_LOCAL_CLOCK calendar");
    if (!local_clock_configuration || strcmp(local_clock_configuration, configuration)) {
        free(local_clock_configuration);
        local_clock_configuration = strdup(configuration);
        if (!local_clock_configuration) recon_unsupported("local clock configuration allocation");
        local_clock_calls = 0;
    }
    if (step && local_clock_calls > (UINT64_MAX - initial) / step)
        recon_unsupported("local clock sequence overflow");
    elapsed = initial + local_clock_calls++ * step;
    if (elapsed / 1000 > 0x7fffffffUL)
        recon_unsupported("local clock sequence duration");
    recon_tzset();
    result->timezone = (short)(recon_timezone / 60);
    result->millitm = (unsigned short)(elapsed % 1000);
    result->time = -1;
    result->dstflag = -1;
    /* Shipped _ftime uses mktime with tm_isdst=-1. That CRT rejects years
     * outside 1969..2039 before normalization, and negative/overflowed time_t. */
    if (year < 1969 || year > 2039) return;
    local_seconds = calendar_seconds(year, month, day, hour, minute, second) + (int64_t)(elapsed / 1000);
    utc_seconds = local_seconds + recon_timezone;
    if (utc_seconds < 0 || utc_seconds > 0x7fffffffL) return;
    dst = local_clock_dst(local_seconds, utc_seconds);
    if (dst) {
        utc_seconds -= 3600;
        if (utc_seconds < 0) return;
        /* mktime's second localtime pass classifies the adjusted standard
         * calendar, including the old CRT's ambiguous transition hours. */
        dst = local_clock_dst(local_seconds - 3600, utc_seconds);
    }
    result->time = (long)utc_seconds;
    result->dstflag = (short)(dst != 0);
}

void recon_ftime(struct _timeb *result)
{
    const char *local_clock = getenv("RECON_LOCAL_CLOCK");
    const char *fixed = getenv("RECON_FIXED_TIME");
    const char *milliseconds = getenv("RECON_FIXED_MILLISECONDS");
    struct timeval current;
    struct tm local;
    time_t seconds;
    long millis;
    char *end;
    if (local_clock) {
        captured_local_ftime(result, local_clock);
        return;
    }
    if (local_clock_configuration) {
        free(local_clock_configuration);
        local_clock_configuration = NULL;
        local_clock_calls = 0;
    }
    if (fixed) {
        seconds = (time_t)strtoll(fixed, NULL, 10);
        millis = 0;
    } else {
        if (gettimeofday(&current, NULL)) recon_unsupported("_ftime clock");
        seconds = current.tv_sec;
        millis = current.tv_usec / 1000;
    }
    if (milliseconds) {
        errno = 0;
        millis = strtol(milliseconds, &end, 10);
        if (errno || !*milliseconds || *end || millis < 0 || millis > 999)
            recon_unsupported("RECON_FIXED_MILLISECONDS must be 0..999");
    }
    tzset();
    if (!localtime_r(&seconds, &local)) recon_unsupported("_ftime local time");
    result->time = (long)seconds;
    result->millitm = (unsigned short)millis;
    /* The timeb offset describes standard time, even during daylight time. */
    result->timezone = (short)(timezone / 60);
    result->dstflag = local.tm_isdst > 0;
}

size_t recon_wcslen(const WCHAR *text)
{
    const WCHAR *end = text;
    while (*end) ++end;
    return (size_t)(end - text);
}

__attribute__((weak)) WCHAR *recon_wcscpy(WCHAR *destination, const WCHAR *source)
{
    WCHAR *cursor = destination;
    while ((*cursor++ = *source++) != 0) { }
    return destination;
}

WCHAR *recon_wcsncpy(WCHAR *destination, const WCHAR *source, size_t count)
{
    size_t index = 0;
    while (index < count && source[index]) {
        destination[index] = source[index];
        ++index;
    }
    while (index < count) destination[index++] = 0;
    return destination;
}

/* Captured host input before any target DLL instruction. Only the observed
 * architectural words are supplied; native pointers and empty payload stay. */
static struct {
    char reference_sha256[65];
    unsigned applications;
} native_x87_empty_entry;
void recon_apply_x87_empty_entry_profile(const char *reference_sha256,
                                        unsigned control,unsigned status,unsigned tag)
{
    unsigned char environment[28];
    unsigned short current_control,current_status,current_tag,word;
    unsigned n;int saved_errno=errno;unsigned long saved_dos=recon_doserrno;
    DWORD saved_error=recon_GetLastError();
    if (native_x87_empty_entry.applications || !reference_sha256 ||
        strlen(reference_sha256)!=64 || control!=0x027f || status!=0x0020 || tag!=0xffff)
        recon_unsupported("unreviewed x87 empty-entry profile");
    for(n=0;n<64;++n)if(!((reference_sha256[n]>='0' && reference_sha256[n]<='9') ||
        (reference_sha256[n]>='a' && reference_sha256[n]<='f')))
        recon_unsupported("x87 empty-entry reference SHA256");
    __asm__ __volatile__("fnstenv %0":"=m"(environment));
    memcpy(&current_control,environment,2);memcpy(&current_status,environment+4,2);
    memcpy(&current_tag,environment+8,2);
    if(current_control!=0x027f || current_status || current_tag!=0xffff)
        recon_unsupported("x87 empty-entry native prestate");
    word=(unsigned short)status;memcpy(environment+4,&word,2);
    __asm__ __volatile__("fldenv %0"::"m"(environment));
    memcpy(native_x87_empty_entry.reference_sha256,reference_sha256,65);
    native_x87_empty_entry.applications=1;
    errno=saved_errno;recon_doserrno=saved_dos;recon_SetLastError(saved_error);
}
