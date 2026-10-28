#ifndef RECON_TRACE_ABI_H
#define RECON_TRACE_ABI_H
/* Clang cannot insert entry instrumentation into naked private-ABI functions.
 * Preserve the complete caller state, including a live x87 stack, before the
 * ordinary trace hook. Empty in canonical Win32 and uninstrumented builds. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__) && defined(RECON_NATIVE_TRACE)
#define RECON_TRACE_STRING_(name) #name
#define RECON_TRACE_STRING(name) RECON_TRACE_STRING_(name)
#define RECON_TRACE_NAKED_ENTRY(name) __asm__ volatile ( \
    "pushfl\n\tpushal\n\tmovl %esp,%eax\n\t" \
    "subl $544,%esp\n\tandl $-16,%esp\n\tmovl %eax,512(%esp)\n\t" \
    "fxsave (%esp)\n\tfninit\n\tcall 991f\n991:\n\tpopl %ecx\n\t" \
    "leal " RECON_TRACE_STRING(name) "-991b(%ecx),%ecx\n\t" \
    "subl $16,%esp\n\tmovl %ecx,(%esp)\n\tmovl 36(%eax),%edx\n\t" \
    "movl %edx,4(%esp)\n\tcall __cyg_profile_func_enter\n\t" \
    "addl $16,%esp\n\tfxrstor (%esp)\n\tmovl 512(%esp),%esp\n\t" \
    "popal\n\tpopfl\n\t");
#else
#define RECON_TRACE_NAKED_ENTRY(name)
#endif
/* A C implementation with a private register ABI cannot allow a compiler-
 * inserted exit hook after it publishes output registers. Suppress automatic
 * hooks on that same function and explicitly trace its named entry instead.
 * This variant runs inside an ordinary C frame: the unused caller is NULL,
 * rather than assuming the naked-entry return-address offset above.
 * The body still owns its actual compiler prolog/epilog ABI qualification. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#define RECON_TRACE_REGISTER_ABI __attribute__((no_instrument_function))
#define RECON_TRACE_REGISTER_ENTRY(name) __asm__ volatile ( \
    "pushfl\n\tpushal\n\tmovl %%esp,%%eax\n\t" \
    "subl $544,%%esp\n\tandl $-16,%%esp\n\tmovl %%eax,512(%%esp)\n\t" \
    "fxsave (%%esp)\n\tfninit\n\tcld\n\tcall 991f\n991:\n\tpopl %%ecx\n\t" \
    "leal " #name "-991b(%%ecx),%%ecx\n\t" \
    "subl $16,%%esp\n\tmovl %%ecx,(%%esp)\n\tmovl $0,4(%%esp)\n\t" \
    "call __cyg_profile_func_enter\n\taddl $16,%%esp\n\t" \
    "fxrstor (%%esp)\n\tmovl 512(%%esp),%%esp\n\tpopal\n\tpopfl\n\t" \
    : : : "memory", "cc");
#else
#define RECON_TRACE_REGISTER_ABI
#define RECON_TRACE_REGISTER_ENTRY(name)
#endif
#endif
