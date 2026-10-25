#ifndef LINK_SYMBOL_BUFFER_H
#define LINK_SYMBOL_BUFFER_H
#include <stddef.h>
#include <recon.h>

/* The processor's scoped buffer has the same encountered three-DWORD layout
 * as ByteBuffer. Its cleanup does not reset the pointer or counters. */
class ScopedBuffer {
public:
    void *data;
    unsigned used,capacity;
    RECON_FORCEINLINE ScopedBuffer() { data=0; used=capacity=0; }
    RECON_THISCALL ~ScopedBuffer();
};
typedef char ScopedBufferSize[(sizeof(ScopedBuffer)==12)?1:-1];
typedef char ScopedBufferUsed[(offsetof(ScopedBuffer,used)==4)?1:-1];
typedef char ScopedBufferCapacity[(offsetof(ScopedBuffer,capacity)==8)?1:-1];
#endif
