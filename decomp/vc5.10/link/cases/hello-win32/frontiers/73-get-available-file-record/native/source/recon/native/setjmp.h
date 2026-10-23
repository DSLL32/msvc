#ifndef RECON_NATIVE_SETJMP_H
#define RECON_NATIVE_SETJMP_H
/* Keep the original target storage size; host context lives out of line. */
#define jmp_buf recon_host_jmp_buf
#include_next <setjmp.h>
#undef jmp_buf
#undef setjmp

#if defined(RECON_CRT_PROFILE_WATCOM_100A) && !defined(RECON_SHIM_IMPL)
typedef unsigned int jmp_buf[13];
typedef char ReconWatcomJumpBufferSize[(sizeof(jmp_buf)==52)?1:-1];
#else
#ifndef RECON_LEGACY_JMP_WORDS
#define RECON_LEGACY_JMP_WORDS 8
#endif
#if RECON_LEGACY_JMP_WORDS != 8 && RECON_LEGACY_JMP_WORDS != 16
#error Unsupported legacy jump buffer size
#endif
typedef int jmp_buf[RECON_LEGACY_JMP_WORDS];
typedef char ReconJumpBufferSize[(sizeof(jmp_buf) == 4*RECON_LEGACY_JMP_WORDS) ? 1 : -1];
#endif

#ifdef __cplusplus
extern "C" {
#endif
int recon_watcom_setjmp(unsigned int *);
void recon_watcom_longjmp(unsigned int *, int) __attribute__((noreturn));
recon_host_jmp_buf *recon_jmp_context(void *);
recon_host_jmp_buf *recon_setjmp3_context(void *, unsigned, int);
/* The libc save executes in the application's frame. A C wrapper around
 * setjmp would return through a dead frame when longjmp later resumes it. */
extern int recon_host_setjmp(recon_host_jmp_buf)
    __asm__("_setjmp") __attribute__((returns_twice));
void recon_longjmp(void *, int) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#ifndef RECON_SHIM_IMPL
#if defined(RECON_CRT_PROFILE_WATCOM_100A)
/* No host context is saved until the Watcom public boundary is proved. */
#define _setjmp recon_watcom_setjmp
#define setjmp(environment) recon_watcom_setjmp(environment)
#define longjmp(environment, value) recon_watcom_longjmp((environment), (value))
#else
#define _setjmp3(environment, count, ...) \
    recon_host_setjmp(*recon_setjmp3_context((void *)(environment), sizeof(environment), (count)))
#if defined(RECON_NATIVE_SETJMP3_COUNT0)
#if RECON_LEGACY_JMP_WORDS != 16
#error Setjmp3 requires the verified 16-word target storage
#endif
#define setjmp(environment) _setjmp3(environment, 0)
#else
#define setjmp(environment) recon_host_setjmp(*recon_jmp_context((void *)(environment)))
#endif
#define longjmp(environment, value) recon_longjmp((void *)(environment), (value))
#endif
#endif
#endif
