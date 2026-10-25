/* Application declarations for the 1994 WLIB reconstruction.
   Names other than main and documented resource/CRT APIs are descriptive. */
#ifndef WLIB_RECONSTRUCTION_H
#define WLIB_RECONSTRUCTION_H
#include <malloc.h>
#include <signal.h>
#include <setjmp.h>
#include <recon.h>

typedef char WlibJumpBufferSize[(sizeof(jmp_buf) == 52) ? 1 : -1];
extern jmp_buf Env;                 /* Original VA 0x430014. */
extern unsigned long ToolFlags;     /* Original VA 0x4302b8. */

typedef struct handle_info {
    int handle;
    const char *filename;
} HANDLE_INFO;
typedef char WlibResourceHandleSize[(sizeof(HANDLE_INFO) == 8) ? 1 : -1];

/* Only the encountered prefix of the resource callback table is defined. */
struct WResRoutines {
    int (__cdecl *open)( const char *, int, ... );
    int (*close)( int );
    int (*write)( int, const void *, unsigned );
    int (*read)( int, void *, unsigned );
    long (*seek)( int, long, int );
};
extern struct WResRoutines WResRtns;
extern long FileShift;             /* Original VA 0x430440. */
typedef enum {
    WRS_OPEN_FAILED = 5
} WResStatus;

int OpenResFile( HANDLE_INFO *instance );
int ResOpenFileRO( const char *filename );
void WresRecordError( WResStatus status );
int FindResources( HANDLE_INFO *instance );
long res_seek( int handle, long position, int where );
int InitResources( HANDLE_INFO *instance );

void CheckMemoryHook( void );       /* Original VA 0x411fdf; name unknown. */
int InitMsg( const char *image_name );
void HandleSignal( int signal_number );
void InitToolState( int argc, char **argv );
unsigned char ProcessCommands( void );
void FiniToolState( void );
int FiniMsg( void );

#if defined(__WATCOMC__)
/* Inline frontier marker; adds no helper vertex to the function graph. */
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#define WLIB_FRONTIER() reconstruction_trap()
#else
#define WLIB_FRONTIER() __asm__ volatile ("int3")
#endif
#endif
