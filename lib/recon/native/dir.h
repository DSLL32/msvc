#ifndef RECON_NATIVE_BORLAND_DIR_H
#define RECON_NATIVE_BORLAND_DIR_H
#if !defined(RECON_BORLAND_400_FIND) && !defined(RECON_BORLAND_400_FNSPLIT)
#error Native dir.h requires explicit Borland400 directory support
#endif
#include <win32.h>
/* Public BC4 DIR.H flat-model sizes and fnsplit result flags. */
#define WILDCARDS 0x01
#define EXTENSION 0x02
#define FILENAME 0x04
#define DIRECTORY 0x08
#define DRIVE 0x10
#define MAXDRIVE 3
#define MAXPATH 260
#define MAXDIR 256
#define MAXFILE 256
#define MAXEXT 256
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
int __cdecl recon_borland_400_findnext(struct ffblk *);
int __cdecl recon_borland_400_fnsplit(const char *,char *,char *,char *,char *);
#ifdef __cplusplus
}
#endif
#endif
