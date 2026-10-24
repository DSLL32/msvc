/* Actual i386 ABI/live-owner tests for shared entry interception. */
#define _GNU_SOURCE
#include "recon-native.h"
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <unistd.h>

#define NOINST __attribute__((no_instrument_function))
#define STDCALL __attribute__((stdcall))
#define FASTCALL __attribute__((fastcall))
struct Owner
{
    int value;
    int *live;
};
static const void *watched;
static int entered, mutated, callback_count;
static int trap_pipe[2];
void NOINST __attribute__((visibility("protected")))
__cyg_profile_func_enter(void *function, void *caller)
{
    (void)caller;
    if (function == watched)
        ++entered;
}
void NOINST __attribute__((visibility("protected")))
__cyg_profile_func_exit(void *function, void *caller)
{
    (void)function;
    (void)caller;
}
static void marker_cdecl(struct Owner *owner, int value)
{
    owner->value = value;
    ++mutated;
}
static void STDCALL marker_stdcall(struct Owner *owner, int value)
{
    marker_cdecl(owner, value);
}
static int FASTCALL marker_fastcall(struct Owner *owner, unsigned edx,
                                    int value)
{
    marker_cdecl(owner, value + edx);
    return value;
}
static void callback_cdecl(struct Owner *owner, int value)
{
    ++callback_count;
    recon_native_entry_finish(owner->value == 17 && *owner->live == 31 &&
                              value == 43);
}
static void STDCALL callback_stdcall(struct Owner *owner, int value)
{
    callback_cdecl(owner, value);
}
static int FASTCALL callback_fastcall(struct Owner *owner, unsigned edx,
                                      int value)
{
    ++callback_count;
    recon_native_entry_finish(owner->value == 17 && *owner->live == 31 &&
                              edx == 59 && value == 43);
}
static void invoke(void *kind)
{
    int live = 31;
    struct Owner owner = {17, &live};
    switch ((uintptr_t)kind)
    {
    case 0:
        marker_cdecl(&owner, 43);
        break;
    case 1:
        marker_stdcall(&owner, 43);
        break;
    case 2:
        marker_fastcall(&owner, 59, 43);
        break;
    }
    ++mutated; /* Must never resume after callback state mutation. */
}
static void missed(void *argument) { (void)argument; }
static void callback_returns(struct Owner *owner, int value)
{
    (void)owner;
    (void)value;
}
static void observe(int signal, siginfo_t *info, void *context)
{
    ucontext_t *state = context;
    uintptr_t record[3];
    record[0] = state->uc_mcontext.gregs[REG_EIP];
    record[1] = (uintptr_t)info->si_code;
    record[2] = *(const unsigned char *)(record[0] - 1);
    if (write(trap_pipe[1], record, sizeof(record)) != sizeof(record))
        _exit(127);
    raise(signal); /* Previous SA_RESETHAND action must already be default. */
}
static void unrelated(void *argument)
{
    (void)argument;
    __asm__ volatile("int $3");
}
int main(void)
{
    const void *entries[] = {marker_cdecl, marker_stdcall, marker_fastcall};
    const void *callbacks[] = {callback_cdecl, callback_stdcall,
                               callback_fastcall};
    struct sigaction original, before, after, action;
    int i, status;
    pid_t child;
    uintptr_t trap_record[3];
    struct rlimit core = {0, 0};
    assert(setrlimit(RLIMIT_CORE, &core) == 0);
    for (i = 0; i != 3; ++i)
    {
        unsigned char first = *(const unsigned char *)entries[i];
        watched = entries[i];
        entered = callback_count = mutated = 0;
        assert(sigaction(SIGTRAP, 0, &before) == 0);
        assert(recon_native_at_entry(entries[i], callbacks[i], invoke,
                                     (void *)(uintptr_t)i) == 1);
        assert(entered == 1 && callback_count == 1 && mutated == 0);
        assert(*(const unsigned char *)entries[i] == first);
        assert(sigaction(SIGTRAP, 0, &after) == 0);
        assert(before.sa_handler == after.sa_handler &&
               before.sa_flags == after.sa_flags);
        assert(recon_native_at_entry(entries[i], callbacks[i], missed, 0) == 0);
        assert(*(const unsigned char *)entries[i] == first);
    }
    assert(recon_native_at_entry(0, callback_cdecl, invoke, 0) == -1 &&
           errno == EINVAL);
    child = fork();
    assert(child >= 0);
    if (!child)
    {
        recon_native_at_entry(marker_cdecl, callback_returns, invoke, 0);
        _exit(99);
    }
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
           WEXITSTATUS(status) == 126);
    /* An unrelated INT3 while armed must deliver its actual PC/cause to the
     * previous observer, preserve reset-on-delivery and terminate via SIGTRAP.
     */
    assert(pipe(trap_pipe) == 0);
    action.sa_sigaction = observe;
    action.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigemptyset(&action.sa_mask);
    assert(sigaction(SIGTRAP, &action, &original) == 0);
    child = fork();
    assert(child >= 0);
    if (!child)
    {
        recon_native_at_entry(marker_cdecl, callback_cdecl, unrelated, 0);
        _exit(99);
    }
    assert(waitpid(child, &status, 0) == child && WIFSIGNALED(status) &&
           WTERMSIG(status) == SIGTRAP);
    assert(read(trap_pipe[0], trap_record, sizeof(trap_record)) ==
           sizeof(trap_record));
    assert(trap_record[0] > (uintptr_t)unrelated &&
           trap_record[0] < (uintptr_t)unrelated + 256);
    assert((trap_record[1] == SI_KERNEL || trap_record[1] == TRAP_BRKPT) &&
           trap_record[2] == 0xcc);
    close(trap_pipe[0]);
    close(trap_pipe[1]);
    assert(sigaction(SIGTRAP, &original, 0) == 0);
    return 0;
}
