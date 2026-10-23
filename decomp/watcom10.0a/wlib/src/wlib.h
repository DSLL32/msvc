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
