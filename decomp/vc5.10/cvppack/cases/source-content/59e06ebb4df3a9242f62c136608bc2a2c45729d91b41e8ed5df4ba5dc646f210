#ifndef RECON_NATIVE_SETJMP_H
#define RECON_NATIVE_SETJMP_H
/* Consume the host API under a distinct storage type. VC1 exposes eight
 * 32-bit words; glibc's larger context must never be written into that array. */
#define jmp_buf recon_host_jmp_buf
#include_next <setjmp.h>
#undef jmp_buf
#undef setjmp

typedef int jmp_buf[8];
typedef char ReconJumpBufferIs32[(sizeof(jmp_buf) == 32) ? 1 : -1];

recon_host_jmp_buf *recon_jmp_context(void *);
/* The libc save executes in the application's frame. A C wrapper around
 * setjmp would return through a dead frame when longjmp later resumes it. */
extern int recon_host_setjmp(recon_host_jmp_buf)
    __asm__("_setjmp") __attribute__((returns_twice));
void recon_longjmp(void *, int) __attribute__((noreturn));

#ifndef RECON_SHIM_IMPL
#define setjmp(environment) recon_host_setjmp(*recon_jmp_context((void *)(environment)))
#define longjmp(environment, value) recon_longjmp((void *)(environment), (value))
#endif
#endif
