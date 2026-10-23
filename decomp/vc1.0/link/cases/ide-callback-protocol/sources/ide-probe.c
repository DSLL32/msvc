/* Readable external dependency for direct minimal-code tests, not a LINK
 * reconstruction implementation. Original tests load the VC1-compiled DLL;
 * native cached tests link these same callbacks directly. */
#include "ide-probe.h"
#include <recon.h>
static IdeTestProbe probe;

TEST_EXPORT unsigned long __cdecl IdeProbeChannel(unsigned long kind)
{
    ++probe.get_calls;
    probe.last_kind=kind;
    if (kind!=2) ++probe.failed;
    return 7;
}
TEST_EXPORT int __cdecl IdeProbeWrite(unsigned long channel,const char *bytes,
                                    unsigned long count)
{
    IdeTestWrite *record;
    unsigned long i;
    if (probe.write_calls>=16 || count>1024 || probe.used+count>sizeof(probe.sink)) {
        ++probe.failed;
        return -1;
    }
    record=&probe.writes[probe.write_calls++];
    record->channel=channel;
    record->count=count;
    record->result=count;
    record->offset=probe.used;
    record->owned_buffer=(bytes==probe.buffer);
    if (channel!=7 || !record->owned_buffer) ++probe.failed;
    for (i=0;i<count;++i) probe.sink[probe.used++]=(unsigned char)bytes[i];
    return count;
}
TEST_EXPORT void __cdecl IdeProbeAssertion(const char *source,unsigned long line)
{
    /* The valid fixture must never invoke an assertion. Record unexpected
     * entry; no assertion/failure-path behavior is being emulated or claimed. */
    ++probe.assert_calls;
    ++probe.failed;
}
TEST_EXPORT IdeTestProbe * __cdecl GetIdeProbe(void)
{
    unsigned long i;
    unsigned char *data=(unsigned char *)&probe;
    for (i=0;i<sizeof(probe);++i) data[i]=0;
    for (i=0;i<32;++i) probe.before[i]=probe.after[i]=0xa5;
    for (i=0;i<sizeof(probe.buffer);++i) probe.buffer[i]=(char)0xa5;
    probe.api.get_channel=IdeProbeChannel;
    probe.api.write=IdeProbeWrite;
    probe.api.assertion=IdeProbeAssertion;
    probe.api.buffer=probe.buffer;
    probe.api.capacity=sizeof(probe.buffer);
    return &probe;
}
#ifndef RECON_NATIVE_TEST
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,void *reserved)
{
    return TRUE;
}
#endif
