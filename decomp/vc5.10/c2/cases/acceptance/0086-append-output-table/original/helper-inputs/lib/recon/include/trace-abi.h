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
#endif
