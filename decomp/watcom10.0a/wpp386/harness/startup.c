/* Native test bootstrap for the original pre-main initialization sequence.
 * The original capture executes its actual CRT; native startup keeps local
 * FILE26 records and consumes five real private-heap allocations as owned
 * stream-list nodes. It neither overlays host FILE nor manufactures entries.
 * XI chooses the last entry at equal priority: stack, files, then EFG. */
#include "startup.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <recon.h>
extern char **environ;
WppStartup wpp_startup;
extern void free_segment_wrapper(void *);
static void init_stack(void)
{
    /* Host selector observation belongs to test transport, not App code. */
    __asm__ __volatile__("mov %%ss,%0" : "=r"(wpp_startup.saved_ss));
}
static void *allocate_node(unsigned amount)
{
    if (!wpp_startup.allocation_calls) {
        /* Validate genuine effects at the first allocation, before App entry. */
        if (amount != 8 || sizeof(WppFile) != 26 || sizeof(WppStreamLink) != 8 ||
            wpp_startup.initializers[2].kind != 2 ||
            wpp_startup.initializers[1].kind != 0 ||
            wpp_startup.initializers[0].kind != 0 ||
            wpp_startup.files[2].flag != 0x402 ||
            wpp_startup.files[0].flag != 1 ||
            wpp_startup.open_streams || wpp_startup.closed_streams ||
            wpp_startup.argc || wpp_startup.argv ||
            !wpp_startup.saved_ss || !wpp_startup.environment)
            abort();
        wpp_startup.first_size = amount;
        wpp_startup.first_frontier_checked = 1;
    }
    ++wpp_startup.allocation_calls;
    /* Corresponds to actual14B ES=DS wrapper; native addresses are alreadyflat. */
    return MemAlloc(amount);
}
static void init_files(void)
{
    WppFile *fp;
    WppStreamLink *link;
    wpp_startup.files[2].flag &= ~0x700u;
    wpp_startup.files[2].flag |= 0x400;
    for (fp=wpp_startup.files; fp->flag; ++fp) {
        link=(WppStreamLink *)allocate_node(8);
        if (!link) link=(WppStreamLink *)allocate_node(8);
        /* Failure subtree9eed2 is not qualified; fail before simulating it. */
        if (!link) abort();
        link->stream=fp;
        link->next=wpp_startup.open_streams;
        wpp_startup.open_streams=link;
    }
    wpp_startup.closed_streams=0;
}
/* Normal shutdown ownership from shipped ioexit/allocfp: standard handles
 * stay open; __freefp unlinks a FILE node and prepends it to closed_streams.
 * These private FILE26 records never overlay public native ShimFILE objects. */
static void close_owned_stream(unsigned index)
{
    WppStreamLink **owner, *link;
    owner = &wpp_startup.open_streams;
    while (*owner && (*owner)->stream != &wpp_startup.files[index])
        owner = &(*owner)->next;
    if (!*owner) recon_unsupported("WPP repeated/unowned standard stream close");
    link = *owner;
    *owner = link->next;
    link->stream->flag |= 3;
    link->next = wpp_startup.closed_streams;
    wpp_startup.closed_streams = link;
}

/* Bound only in SrcFileFClose's native translation unit. */
int wpp_fclose(FILE *stream)
{
    unsigned index;
    int result;
    for (index = 0; index < 5; ++index)
        if (stream == &recon_iob[index]) break;
    if (index == 5) recon_unsupported("WPP unqualified file-close ownership");
    result = recon_fclose(stream);
    close_owned_stream(index);
    return result;
}

static void finish_files(void)
{
    WppStreamLink *link, *next;
    unsigned index, freed;
    /* docloseall(0) flushes the output before purging nodes. The actual
     * standard-handle branch uses close_handle=0, so do not close host FDs. */
    if (fflush(stdout)) recon_unsupported("WPP normal stdout shutdown flush");
    while (wpp_startup.open_streams) {
        link = wpp_startup.open_streams;
        index = (unsigned)(link->stream - wpp_startup.files);
        if (index >= 3) recon_unsupported("WPP unclosed auxiliary stream");
        close_owned_stream(index);
    }
    freed = 0;
    while ((link = wpp_startup.closed_streams) != NULL) {
        next = link->next;
        free_segment_wrapper(link);
        wpp_startup.closed_streams = next;
        ++freed;
    }
    if (freed != 5 || wpp_startup.allocation_calls != 5)
        recon_unsupported("WPP normal stream-node lifetime");
}

static void init_efg(void)
{
    /* The original installs CRT-private formatting/scanning pointers only.
     * App code uses public CRT functions, supplied by lib/recon natively;
     * there is no native copy of these unconsumed private pointer cells. */
}
static void run_initializers(void)
{
    WppInitRecord *record, *next;
    unsigned limit;
    for (;;) {
        next=0;
        limit=255;
        for (record=wpp_startup.initializers;
             record<wpp_startup.initializers+3; ++record) {
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
int wpp_native_startup(int argc, char **argv)
{
    unsigned i, count;
    char **environment;
    if (argc != 2 || strcmp(argv[1],"?")) abort();
    /* Zero only declared runtime state; no copied original data section.
     * Original cstart zeroes its BSS before invoking the initializer table. */
    memset(&wpp_startup,0,sizeof(wpp_startup));
    wpp_startup.program=argv[0];
    wpp_startup.command=argv[1];
    wpp_startup.os_id=2;
    __asm__ __volatile__("mov %%ds,%0" : "=r"(wpp_startup.saved_ds));
    for (count=0; environ[count]; ++count) if (count>=4096) abort();
    /* Original builds environ as stack-resident pointers into loader input. */
    environment=(char **)alloca((count+1)*sizeof(char *));
    /* cstart PUSHes each scanned pointer: savedESP therefore reverses order. */
    for (i=0; i<count; ++i) environment[i]=environ[count-1-i];
    environment[count]=0;
    wpp_startup.environment=environment;
    for (i=0; i<5; ++i) wpp_startup.files[i].handle=i;
    wpp_startup.files[0].flag=1;
    wpp_startup.files[1].flag=2;
    wpp_startup.files[2].flag=2;
    wpp_startup.files[3].flag=3;
    wpp_startup.files[4].flag=2;
    for (i=0; i<3; ++i) wpp_startup.initializers[i].priority=32;
    wpp_startup.initializers[0].callback=init_efg;
    wpp_startup.initializers[1].callback=init_files;
    wpp_startup.initializers[2].callback=init_stack;
    run_initializers();
    /* Registered before main; the later App resetHandlers runs first. */
    if (atexit(finish_files)) abort();
    /* Actual OSI __CMain reads these zero/null BSS cells, not the raw tail. */
    return main(wpp_startup.argc, wpp_startup.argv);
}
