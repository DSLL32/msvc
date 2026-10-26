#ifndef WCC_CFLOAT_H
#define WCC_CFLOAT_H
#include <stddef.h>
#include <recon.h>
/* cfloat.h's decimal representation, before the later callback allocator. */
#pragma pack(4)
struct cfloat {
    int exp,len,alloc;
    signed char sign;
    char mant[1];
};
#pragma pack()
typedef char old_cfloat_size[(sizeof(struct cfloat)==16)?1:-1];
typedef char old_cfloat_digits[(offsetof(struct cfloat,mant)==13)?1:-1];
#endif
