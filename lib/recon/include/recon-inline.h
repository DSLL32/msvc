#ifndef RECON_INLINE_H
#define RECON_INLINE_H
/* Only for construction/helpers proved inlined into original App vertices.
 * Instrumenting an inline helper materializes its address and an extra native
 * vertex, so those helpers alone omit instrumentation. Real App functions must
 * remain outlined and instrumented. Canonical inlining still needs /Ob1. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#define RECON_FORCEINLINE __attribute__((always_inline,no_instrument_function)) inline
#else
#define RECON_FORCEINLINE __inline
#endif
#endif
