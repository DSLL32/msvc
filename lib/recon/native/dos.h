#ifndef RECON_NATIVE_BORLAND_DOS_H
#define RECON_NATIVE_BORLAND_DOS_H
#ifndef RECON_BORLAND_400_DOS_TIME
#error Native dos.h currently requires RECON_BORLAND_400_DOS_TIME
#endif
#include <win32.h>
/* BC4 DOS.H flat-model branch uses #pragma option -a-. */
#pragma pack(push,1)
struct date {int da_year;char da_day,da_mon;};
struct time {unsigned char ti_min,ti_hour,ti_hund,ti_sec;};
#pragma pack(pop)
#ifdef __cplusplus
extern "C" {
#endif
/* Native names avoid the unrelated GNU getdate parser declaration. recon.h,
 * included last, maps original call spellings only in the selected build. */
void __cdecl recon_borland_400_getdate(struct date *);
void __cdecl recon_borland_400_gettime(struct time *);
#ifdef __cplusplus
}
#endif
#endif
