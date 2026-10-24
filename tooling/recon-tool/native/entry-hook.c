/* Linux i386 direct-test entry interception. No target function/graph changes.
 */
#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include "recon-native.h"
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <ucontext.h>
#include <unistd.h>

extern void __cyg_profile_func_enter(void *, void *);
static sigjmp_buf checkpoint;
static struct sigaction previous_action;
static unsigned char *target;
static const void *replacement;
static void *page;
static size_t page_size;
static unsigned char original_byte;
static volatile sig_atomic_t running, armed, reached, result;

static void __attribute__((noreturn)) fail_closed(void)
{
    static const char message[] = "recon-tool: native entry hook failed\n";
    write(2, message, sizeof(message) - 1);
    _exit(126);
}
/* The handler uses only direct Linux memory-protection syscalls, byte writes,
 * sigaction and the existing preconfigured allocation-free trace callback. */
static void protect(int flags)
{
    if (syscall(SYS_mprotect, page, page_size, flags) != 0)
        fail_closed();
}
static void restore(void)
{
    if (!armed)
        return;
    armed = 0;
    protect(PROT_READ | PROT_WRITE | PROT_EXEC);
    *target = original_byte;
    protect(PROT_READ | PROT_EXEC);
    if (sigaction(SIGTRAP, &previous_action, 0) != 0)
        fail_closed();
}
static void __attribute__((noreturn, force_align_arg_pointer))
callback_returned(void)
{
    /* Returning would resume parser mutation after a state-changing test. */
    fail_closed();
}
static void trapped(int signal, siginfo_t *info, void *context)
{
    ucontext_t *state = context;
    uintptr_t pc = (uintptr_t)state->uc_mcontext.gregs[REG_EIP];
    int saved_errno = errno;
    struct sigaction action;
    if (armed && (info->si_code == SI_KERNEL || info->si_code == TRAP_BRKPT) &&
        pc == (uintptr_t)target + 1 && *target == 0xcc)
    {
        uintptr_t *stack = (uintptr_t *)state->uc_mcontext.gregs[REG_ESP];
        void *caller = (void *)stack[0];
        restore();
        reached = 1;
        /* Redirecting before the prologue skips compiler entry instrumentation.
         * Record the genuine marker once through the SAME shared tracer. */
        __cyg_profile_func_enter(target, caller);
        stack[0] = (uintptr_t)callback_returned;
        state->uc_mcontext.gregs[REG_EIP] = (greg_t)(uintptr_t)replacement;
        errno = saved_errno;
        return;
    }
    action = previous_action;
    restore();
    /* Chain with the original PC/cause, rather than raising a new synthetic
     * signal that would lose frontier ownership. Honor reset-on-delivery. */
    if (action.sa_flags & SA_RESETHAND)
    {
        struct sigaction reset;
        memset(&reset, 0, sizeof(reset));
        reset.sa_handler = SIG_DFL;
        sigemptyset(&reset.sa_mask);
        if (sigaction(signal, &reset, 0) != 0)
            fail_closed();
    }
    errno = saved_errno;
    if (action.sa_handler == SIG_IGN)
        return;
    if (action.sa_handler == SIG_DFL)
    {
        raise(signal);
        return;
    }
    if (action.sa_flags & SA_SIGINFO)
        action.sa_sigaction(signal, info, context);
    else
        action.sa_handler(signal);
}
static int arm(void)
{
    struct sigaction action;
    sigset_t mask, saved;
    int error;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTRAP);
    if (sigprocmask(SIG_BLOCK, &mask, &saved) != 0)
        return -1;
    if (mprotect(page, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    {
        error = errno;
        sigprocmask(SIG_SETMASK, &saved, 0);
        errno = error;
        return -1;
    }
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = trapped;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGTRAP, &action, &previous_action) != 0)
    {
        error = errno;
        protect(PROT_READ | PROT_EXEC);
        sigprocmask(SIG_SETMASK, &saved, 0);
        errno = error;
        return -1;
    }
    original_byte = *target;
    *target = 0xcc;
    armed = 1;
    protect(PROT_READ | PROT_EXEC);
    if (sigprocmask(SIG_SETMASK, &saved, 0) != 0)
        fail_closed();
    return 0;
}
int recon_native_at_entry(const void *entry, const void *typed_callback,
                          ReconNativeInvoke invoke, void *argument)
{
    long size;
    if (running || !entry || !typed_callback || entry == typed_callback ||
        !invoke)
    {
        errno = running ? EBUSY : EINVAL;
        return -1;
    }
    size = sysconf(_SC_PAGESIZE);
    if (size <= 0)
    {
        errno = EINVAL;
        return -1;
    }
    target = (unsigned char *)entry;
    replacement = typed_callback;
    page_size = (size_t)size;
    page = (void *)((uintptr_t)entry & ~(uintptr_t)(page_size - 1));
    running = 1;
    reached = result = 0;
    if (sigsetjmp(checkpoint, 1) == 0)
    {
        if (arm() != 0)
        {
            running = 0;
            return -1;
        }
        invoke(argument);
    }
    restore();
    running = 0;
    return reached ? result : 0;
}
void recon_native_entry_finish(int passed)
{
    if (!running || !reached)
        fail_closed();
    result = passed != 0;
    siglongjmp(checkpoint, 1);
}
