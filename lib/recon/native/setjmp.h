#ifndef RECON_NATIVE_SETJMP_H
#define RECON_NATIVE_SETJMP_H
/* Keep the original target storage size; host context lives out of line. */
#define jmp_buf recon_host_jmp_buf
#include_next <setjmp.h>
#undef jmp_buf
#undef setjmp

#ifndef RECON_LEGACY_JMP_WORDS
#define RECON_LEGACY_JMP_WORDS 8
#endif
#if RECON_LEGACY_JMP_WORDS != 8 && RECON_LEGACY_JMP_WORDS != 16
#error Unsupported legacy jump buffer size
#endif
typedef int jmp_buf[RECON_LEGACY_JMP_WORDS];
typedef char ReconJumpBufferSize[(sizeof(jmp_buf) == 4*RECON_LEGACY_JMP_WORDS) ? 1 : -1];

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
