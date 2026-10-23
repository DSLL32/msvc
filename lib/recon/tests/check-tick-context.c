#include <assert.h>
#include <recon.h>
int main(void)
{
    DWORD first,second; struct timespec now; uint32_t expected;
    assert(sizeof(FLOATING_SAVE_AREA)==112 && sizeof(CONTEXT)==204);
    assert(offsetof(CONTEXT,Eax)==0xb0 && offsetof(CONTEXT,Esp)==0xc4);
    assert(offsetof(CONTEXT,FloatSave)==28 && offsetof(CONTEXT,Eip)==0xb8);
    errno=111; SetLastError(456); first=GetTickCount(); second=GetTickCount();
    assert(errno==111 && GetLastError()==456 && (DWORD)(second-first)<10000);
#ifdef CLOCK_BOOTTIME
    assert(clock_gettime(CLOCK_BOOTTIME,&now)==0);
#else
    assert(clock_gettime(CLOCK_MONOTONIC,&now)==0);
#endif
    expected=(uint32_t)((uint64_t)now.tv_sec*1000+now.tv_nsec/1000000);
    assert((uint32_t)(expected-(uint32_t)second)<10000);
    return 0;
}
