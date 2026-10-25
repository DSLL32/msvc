#ifndef RECON_INLINE_H
#define RECON_INLINE_H
/* Only for construction/helpers proved inlined into original App vertices.
 * Instrumenting an inline helper materializes its address and an extra native
 * vertex, so those helpers alone omit instrumentation. Real App functions must
 * remain outlined and instrumented. Canonical inlining still needs /Ob1. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#define RECON_FORCEINLINE __attribute__((always_inline,no_instrument_function)) inline
/* Retain a real function's native symbol without changing its instrumentation. */
#define RECON_RETAIN __attribute__((used))
#else
#define RECON_FORCEINLINE __inline
#define RECON_RETAIN
#endif
#endif
