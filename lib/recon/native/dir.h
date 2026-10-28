#ifndef RECON_NATIVE_BORLAND_DIR_H
#define RECON_NATIVE_BORLAND_DIR_H
#ifndef RECON_BORLAND_400_FIND
#error Native dir.h currently requires RECON_BORLAND_400_FIND
#endif
#include <win32.h>
/* BC4 DIR.H __FLAT__ branch: #pragma option -a-. */
#pragma pack(push,1)
struct ffblk {
    long ff_reserved;
    long ff_fsize;
    unsigned long ff_attrib;
    unsigned short ff_ftime,ff_fdate;
    char ff_name[256];
};
#pragma pack(pop)
#ifdef __cplusplus
extern "C" {
#endif
int __cdecl recon_borland_400_findfirst(const char *,struct ffblk *,int);
#ifdef __cplusplus
}
#endif
#endif
