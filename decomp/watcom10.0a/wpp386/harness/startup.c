/* Native test bootstrap, not App or handwritten CRT reconstruction.
 * Static qualification: WPP finite first-frontier witness under harness/; original capture pending. Source/header26-byte FILE is Watcom10 STDIO.H. Original XI
 * chooses the last entry at equal priority, running stack then files then EFG.
 * This performs initialization and consumes the allocation result as a real
 * FILE list node; it never calls MemAlloc just to manufacture a trace entry. */
#include "startup.h"
#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <recon.h>
extern char **environ;
WppStartup wpp_startup;
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
static void init_efg(void)
{
    /* Unreached after current pending allocator; no formatter substitute. */
    abort();
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
    /* Native main is deliberately not reached before App allocation frontier. */
    abort();
}
