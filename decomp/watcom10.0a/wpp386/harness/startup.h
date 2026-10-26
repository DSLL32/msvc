#ifndef WPP_NATIVE_STARTUP_H
#define WPP_NATIVE_STARTUP_H
#include <stddef.h>
#include <recon.h>
/* Native test model of the reached original startup sequence.
 * None of these helpers is a reconstructed application function. */
#pragma pack(push,1)
typedef struct WppFile {
    unsigned char *ptr;
    int count;
    unsigned char *base;
    unsigned flag;
    int handle;
    unsigned buffer_size;
    unsigned char ungotten, temporary;
} WppFile;
#pragma pack(pop)
typedef struct WppStreamLink {
    struct WppStreamLink *next;
    WppFile *stream;
} WppStreamLink;
typedef struct WppInitRecord {
    unsigned char kind, priority;
    void (*callback)(void);
} WppInitRecord;
typedef struct WppStartup {
    const char *program, *command;
    char **environment;
    unsigned char os_id;
    unsigned short saved_ds, saved_ss;
    WppFile files[20];
    WppInitRecord initializers[3];
    WppStreamLink *open_streams, *closed_streams;
    unsigned allocation_calls, first_size;
    unsigned file_open_calls, first_open_owner;
    unsigned first_frontier_checked;
    unsigned argc;
    char **argv;
} WppStartup;
extern WppStartup wpp_startup;
int wpp_native_startup(int argc, char **argv);
int main(int argc, char **argv);
#endif
