#ifndef LINK_INITIALIZERS_H
#define LINK_INITIALIZERS_H
#include <stdio.h>
#include <windows.h>
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif


typedef void (__cdecl *Initializer)(void);
typedef struct BufferBlock {
    void *base;
    unsigned remaining;
    unsigned capacity;
} BufferBlock;
typedef struct TimingState {
    DWORD start_tick, end_tick;
    FILE *output;
    int options;
    unsigned char display;
} TimingState;

extern BufferBlock buffer12, buffer8, buffer4;
extern TimingState timing;
extern Initializer application_initializers[];
void __cdecl initialize_buffer12(void);
void __cdecl initialize_buffer8(void);
void __cdecl initialize_buffer4(void);
void __cdecl initialize_timing(void);
void __cdecl allocate_buffer12(void);
void __cdecl register_buffer12_cleanup(void);
void __cdecl free_buffer12(void);
void __cdecl allocate_buffer8(void);
void __cdecl register_buffer8_cleanup(void);
void __cdecl free_buffer8(void);
void __cdecl allocate_buffer4(void);
void __cdecl register_buffer4_cleanup(void);
void __cdecl free_buffer4(void);
void __cdecl setup_timing(void);
void __cdecl register_timing_cleanup(void);
void __cdecl close_timing(void);
/* Zero-section COFF alias binds this C spelling to ??2@YAPAXI@Z. */
void *__cdecl crt_operator_new(unsigned bytes);
#ifdef __cplusplus
}
#endif
#endif
