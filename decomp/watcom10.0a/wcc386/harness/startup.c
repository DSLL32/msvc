/* Native test bootstrap, not App or handwritten CRT reconstruction.
 * Static qualification: first-frontier d29b8063, actual original capture saved
 * with this case. Source/header26-byte FILE is Watcom10 STDIO.H. Original XI
 * chooses the last entry at equal priority, running stack then files then EFG.
 * This performs initialization and consumes the allocation result as a real
 * FILE list node; it never calls MemAlloc just to manufacture a trace entry. */
#include "startup.h"
#include "memory.h"
#include "allocator.h"
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <recon.h>
extern char **environ;
extern int main(void);
WccStartup wcc_startup;
static void init_stack(void)
{
    /* Host selector observation belongs to test transport, not App code. */
    __asm__ __volatile__("mov %%ss,%0" : "=r"(wcc_startup.saved_ss));
}
static void *allocate_node(unsigned amount)
{
    if (!wcc_startup.allocation_calls) {
        /* Validate genuine effects at the first allocation, before App entry. */
        if (amount != 8 || sizeof(WccInitRecord) != 6 || sizeof(WccFile) != 26 || sizeof(WccStreamLink) != 8 ||
            wcc_startup.initializers[2].kind != 2 ||
            wcc_startup.initializers[1].kind != 0 ||
            wcc_startup.initializers[0].kind != 0 ||
            wcc_startup.files[2].flag != 0x402 ||
            wcc_startup.files[0].flag != 1 ||
            wcc_startup.open_streams || wcc_startup.closed_streams ||
            wcc_startup.argc || wcc_startup.argv ||
            !wcc_startup.saved_ss || !wcc_startup.environment)
            abort();
        wcc_startup.first_size = amount;
        wcc_startup.first_frontier_checked = 1;
    }
    ++wcc_startup.allocation_calls;
    /* Corresponds to actual14B ES=DS wrapper; native addresses are alreadyflat. */
    return malloc_segment_wrapper(amount);
}
static void init_files(void)
{
    WccFile *fp;
    WccStreamLink *link;
    wcc_startup.files[2].flag &= ~0x700u;
    wcc_startup.files[2].flag |= 0x400;
    for (fp=wcc_startup.files; fp->flag; ++fp) {
        link=(WccStreamLink *)allocate_node(8);
        if (!link) link=(WccStreamLink *)allocate_node(8);
        /* Failure subtree66c12 is not qualified; fail before simulating it. */
        if (!link) abort();
        link->stream=fp;
        link->next=wcc_startup.open_streams;
        wcc_startup.open_streams=link;
    }
    wcc_startup.closed_streams=0;
}
static char *pending_efg_format()
{
    recon_unsupported("WCC floating formatter consumer not reached");
    return 0;
}
static void pending_cnvs2d()
{
    recon_unsupported("WCC floating scanner consumer not reached");
}
/* The original shutdown_stream calls freefp even when doclose fails.
 * Private FILE26 ownership remains distinct from public native ShimFILE. */
static void close_owned_stream(unsigned index)
{
    WccStreamLink **owner,*link;
    owner=&wcc_startup.open_streams;
    while (*owner && (*owner)->stream!=&wcc_startup.files[index]) owner=&(*owner)->next;
    if (!*owner) recon_unsupported("WCC repeated/unowned standard stream close");
    link=*owner;
    *owner=link->next;
    link->stream->flag|=3;
    link->next=wcc_startup.closed_streams;
    wcc_startup.closed_streams=link;
}
int wcc_fclose(FILE *stream)
{
    unsigned index;
    int result;
    for (index=0;index<5;++index) if (stream==&recon_iob[index]) break;
    if (index==5) recon_unsupported("WCC unqualified file-close ownership");
    result=recon_fclose(stream);
    close_owned_stream(index);
    return result;
}
static void finish_files(void)
{
    WccStreamLink *link,*next;
    unsigned index,freed=0;
    /* docloseall(0) leaves host standard handles open and flushes output.
     * Unlink each real node, preserving the original purge order0,1,2,4,3. */
    if (fflush(stdout)) recon_unsupported("WCC stdout shutdown flush");
    while (wcc_startup.open_streams) {
        link=wcc_startup.open_streams;
        index=(unsigned)(link->stream-wcc_startup.files);
        if (index>=3) recon_unsupported("WCC unclosed auxiliary stream");
        close_owned_stream(index);
    }
    while ((link=wcc_startup.closed_streams)!=NULL) {
        next=link->next;
        free_segment_wrapper(link);
        wcc_startup.closed_streams=next;
        ++freed;
    }
    if (freed!=5 || wcc_startup.allocation_calls!=5)
        recon_unsupported("WCC stream-node lifetime");
}

static void init_efg(void)
{
    /* Actual setefg writes two distinct typed callback identities. Their
     * bodies remain unsupported: this bootstrap only models initialization. */
    wcc_startup.efg_printf=pending_efg_format;
    wcc_startup.efg_scanf=pending_cnvs2d;
}
static void run_initializers(void)
{
    WccInitRecord *record, *next;
    unsigned limit;
    for (;;) {
        next=0;
        limit=255;
        for (record=wcc_startup.initializers;
             record<wcc_startup.initializers+3; ++record) {
            if (record->kind != 2 && record->priority <= limit) {
                next=record;
                limit=record->priority;
            }
        }
        if (!next) return;
        next->callback();
        next->kind=2;
    }
}
int wcc_native_startup(int argc, char **argv)
{
    unsigned i, count;
    char **environment;
    char *empty_argv[1];
    char *command_tail;
    if (argc != 2 || strcmp(argv[1],"?")) abort();
    /* Zero only declared runtime state; no copied original data section.
     * Original cstart zeroes its BSS before invoking the initializer table. */
    memset(&wcc_startup,0,sizeof(wcc_startup));
    wcc_startup.program=argv[0];
    /* Borrow the exact retained CreateProcess text. The qualified Win32
     * loader skips its literal first token and leading whitespace; the
     * shared getcmd boundary consumes this same tail, including quotes. */
    command_tail=GetCommandLineA();
    while (*command_tail && *command_tail!=' ' &&
           !(*command_tail>=9 && *command_tail<=13)) ++command_tail;
    while (*command_tail==' ' || (*command_tail>=9 && *command_tail<=13)) ++command_tail;
    wcc_startup.command=command_tail;
    wcc_startup.os_id=2;
    __asm__ __volatile__("mov %%ds,%0" : "=r"(wcc_startup.saved_ds));
    for (count=0; environ[count]; ++count) if (count>=4096) abort();
    /* Original builds environ as stack-resident pointers into loader input. */
    environment=(char **)alloca((count+1)*sizeof(char *));
    /* cstart PUSHes each scanned pointer: savedESP therefore reverses order. */
    for (i=0; i<count; ++i) environment[i]=environ[count-1-i];
    environment[count]=0;
    wcc_startup.environment=environment;
    /* Shared shim binding for the reached ASCII WCGMEMORY getenv consumer.
     * Preserve reversed order and borrowed value identity. The binder needs a
     * nonnull argv vector; original unconsumed ___Argv is still NULL in our
     * target state. A future consumer/rebinding must qualify that difference. */
    empty_argv[0]=0;
    recon_crt_bind_arguments(0,empty_argv,environment);
    for (i=0; i<5; ++i) wcc_startup.files[i].handle=i;
    wcc_startup.files[0].flag=1;
    wcc_startup.files[1].flag=2;
    wcc_startup.files[2].flag=2;
    wcc_startup.files[3].flag=3;
    wcc_startup.files[4].flag=2;
    for (i=0; i<3; ++i) wcc_startup.initializers[i].priority=32;
    wcc_startup.initializers[0].callback=init_efg;
    wcc_startup.initializers[1].callback=init_files;
    wcc_startup.initializers[2].callback=init_stack;
    run_initializers();
    /* Five live FILE nodes and the completed XI callbacks are genuine CRT
     * effects before __CMain calls the application's argument-free main. */
    if (wcc_startup.allocation_calls!=5 || !wcc_startup.efg_printf ||
        !wcc_startup.efg_scanf) abort();
    /* App atexit callbacks are registered later and therefore run first. */
    if (atexit(finish_files)) abort();
    main();
    /* Main/CRT exit and finalizer behavior is still an unreached boundary. */
    recon_unsupported("WCC main return/finalization not reached");
    return 0;
}
