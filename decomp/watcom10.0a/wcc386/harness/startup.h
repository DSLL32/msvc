#ifndef WCC_NATIVE_STARTUP_H
#define WCC_NATIVE_STARTUP_H
#include <stddef.h>
/* Test-only native equivalent of the five qualified original startup owners.
 * None of these helpers is a reconstructed application function. */
#pragma pack(push,1)
typedef struct WccFile {
    unsigned char *ptr;
    int count;
    unsigned char *base;
    unsigned flag;
    int handle;
    unsigned buffer_size;
    unsigned char ungotten, temporary;
} WccFile;
#pragma pack(pop)
typedef struct WccStreamLink {
    struct WccStreamLink *next;
    WccFile *stream;
} WccStreamLink;
#pragma pack(push,1)
typedef struct WccInitRecord {
    unsigned char kind, priority;
    void (*callback)(void);
} WccInitRecord;
#pragma pack(pop)
typedef struct WccStartup {
    const char *program, *command;
    char **environment;
    unsigned char os_id;
    unsigned short saved_ds, saved_ss;
    WccFile files[20];
    WccInitRecord initializers[3];
    WccStreamLink *open_streams, *closed_streams;
    unsigned allocation_calls, first_size;
    unsigned first_frontier_checked;
    unsigned file_open_calls,first_open_owner;
    /* Original ftos.h uses these unspecified-argument C89 pointer types.
     * No formatter body is claimed before a consumer actually reaches it. */
    char *(*efg_printf)();
    void (*efg_scanf)();
    unsigned argc;
    char **argv;
} WccStartup;
extern WccStartup wcc_startup;
int wcc_native_startup(int argc, char **argv);
#endif
