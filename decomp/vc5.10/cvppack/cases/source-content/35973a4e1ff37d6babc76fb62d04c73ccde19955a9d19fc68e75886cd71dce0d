#ifndef CV510_WIDEN_H
#define CV510_WIDEN_H
#include "cvpack.h"
#include <recon.h>

typedef struct WidenTi WidenTi;
typedef struct WidenOutput {
    unsigned bytes, fixups;
    unsigned char *output;
    unsigned long *records;
} WidenOutput;

#ifdef __cplusplus
extern "C" {
#endif
int create_widen(WidenTi **object);
int record_widen(WidenTi *object, void *output, const void *input, unsigned *size);
void release_widen(WidenTi *object);
int query_widen(WidenTi *object, WidenOutput *output, const void *input, unsigned size);
int emit_widen(WidenTi *object, WidenOutput *output, const void *input, unsigned size, unsigned flags);
void *resize_bytes(void *buffer, unsigned size);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* The actual DLL supplies opaque objects. These declarations generate virtual
 * calls only: no constructor, destructor, RTTI or target-side vtable is emitted.
 * RECON_THISCALL is implicit for VC5 members, explicit for native i386. */
struct WidenTi {
#ifndef RECON_NATIVE_TEST
    __declspec(dllimport)
#endif
    static int __cdecl fCreate(WidenTi *&, unsigned, int);
    virtual void RECON_THISCALL Release(void) = 0;
    virtual void RECON_THISCALL Reserved1(void) = 0;
    virtual void RECON_THISCALL Reserved2(void) = 0;
    virtual int RECON_THISCALL Record(void *, const void *, unsigned *) = 0;
    virtual void RECON_THISCALL Reserved4(void) = 0;
    virtual void RECON_THISCALL Reserved5(void) = 0;
    virtual void RECON_THISCALL Reserved6(void) = 0;
    virtual void RECON_THISCALL Reserved7(void) = 0;
    virtual int RECON_THISCALL Query(WidenOutput *, const void *, unsigned, unsigned) = 0;
    virtual int RECON_THISCALL Emit(WidenOutput *, const void *, unsigned, unsigned) = 0;
};
#endif
#endif
