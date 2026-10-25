/* Native test bootstrap, not App or handwritten CRT reconstruction.
 * Static qualification: first-frontier d29b8063, actual original capture saved
 * with this case. Source/header26-byte FILE is Watcom10 STDIO.H. Original XI
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
        if (amount != 8 || sizeof(WccFile) != 26 || sizeof(WccStreamLink) != 8 ||
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
    return MemAlloc(amount);
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
static void init_efg(void)
{
    /* Unreached after current pending allocator; no formatter substitute. */
    abort();
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
    if (argc != 2 || strcmp(argv[1],"?")) abort();
    /* Zero only declared runtime state; no copied original data section.
     * Original cstart zeroes its BSS before invoking the initializer table. */
    memset(&wcc_startup,0,sizeof(wcc_startup));
    wcc_startup.program=argv[0];
    wcc_startup.command=argv[1];
    wcc_startup.os_id=2;
    __asm__ __volatile__("mov %%ds,%0" : "=r"(wcc_startup.saved_ds));
    for (count=0; environ[count]; ++count) if (count>=4096) abort();
    /* Original builds environ as stack-resident pointers into loader input. */
    environment=(char **)alloca((count+1)*sizeof(char *));
    /* cstart PUSHes each scanned pointer: savedESP therefore reverses order. */
    for (i=0; i<count; ++i) environment[i]=environ[count-1-i];
    environment[count]=0;
    wcc_startup.environment=environment;
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
    /* Native main is deliberately not reached before App allocation frontier. */
    abort();
}
