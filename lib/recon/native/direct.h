#ifndef RECON_NATIVE_DIRECT_H
#define RECON_NATIVE_DIRECT_H
/* Watcom's NT directory object is also its returned entry. Keep the native
 * host directory implementation separate from this SDK data layout. */
#ifdef RECON_CRT_PROFILE_WATCOM_100A
struct ReconWatcomDirent {
    char d_dta[21];
    char d_attr;
    unsigned short d_time,d_date;
    long d_size;
    char d_name[256];
    unsigned short d_ino;
    char d_first;
} __attribute__((packed));
#ifndef RECON_SHIM_IMPL
typedef struct ReconWatcomDirent DIR;
#define dirent ReconWatcomDirent
#undef NAME_MAX
#define NAME_MAX 255
#define _A_NORMAL 0x00
#define _A_RDONLY 0x01
#define _A_HIDDEN 0x02
#define _A_SYSTEM 0x04
#define _A_VOLID 0x08
#define _A_SUBDIR 0x10
#define _A_ARCH 0x20
#endif
#endif
/* Function declarations/remapping follow the caller's other includes. */
#endif
