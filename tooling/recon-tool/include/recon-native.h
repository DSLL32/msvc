#ifndef RECON_NATIVE_H
#define RECON_NATIVE_H
#ifdef __cplusplus
extern "C"
{
#endif
    /* Host-only direct-test support. One active hook, on one native thread.
     * The callback has EXACTLY the intercepted function's ABI and arguments. It
     * runs after signal return with the original owners/stack/registers live.
     * Finish must be called instead of returning into the intercepted caller.
     */
    typedef void (*ReconNativeInvoke)(void *);
    int recon_native_at_entry(const void *entry, const void *typed_callback,
                              ReconNativeInvoke invoke, void *argument);
    void recon_native_entry_finish(int passed) __attribute__((noreturn));
/* Return: 1 assertions passed, 0 failed/missed marker, -1 configuration error
 * (errno). A callback that returns terminates with status126, never resuming
 * the parser. App marker tracing is supplied automatically through the shared
 * tracer. */
#ifdef __cplusplus
}
#endif
#endif
