#ifndef IDE_TEST_PROBE_H
#define IDE_TEST_PROBE_H
/* External test dependency. None of these callbacks are LINK vertices. */
#include <windows.h>
#include <stddef.h>
#include <recon.h>
#ifdef RECON_NATIVE_TEST
#define TEST_EXPORT
#else
#define TEST_EXPORT __declspec(dllexport)
#endif
typedef struct IdeTestApi {
    unsigned long (__cdecl *get_channel)(unsigned long kind);
    int (__cdecl *write)(unsigned long channel,const char *bytes,unsigned long count);
    unsigned long reserved;
    void (__cdecl *assertion)(const char *source,unsigned long line);
    char *buffer;
    unsigned long capacity;
} IdeTestApi;
typedef struct IdeTestWrite {
    unsigned long channel,count,result,offset,owned_buffer;
} IdeTestWrite;
typedef struct IdeTestProbe {
    IdeTestApi api;
    unsigned char before[32];
    char buffer[1024];
    unsigned char after[32];
    unsigned long get_calls,last_kind,write_calls,assert_calls,failed,used;
    IdeTestWrite writes[16];
    unsigned char sink[512];
} IdeTestProbe;
typedef char ide_test_api_size[(sizeof(IdeTestApi)==24)?1:-1];
typedef char ide_test_write_size[(sizeof(IdeTestWrite)==20)?1:-1];
typedef char ide_test_buffer_offset[(offsetof(IdeTestProbe,buffer)==56)?1:-1];
typedef char ide_test_probe_size[(sizeof(IdeTestProbe)==1968)?1:-1];
TEST_EXPORT IdeTestProbe * __cdecl GetIdeProbe(void);
#endif
