/* Isolated alias/control probe; fopen and diagnostics are explicit mocks. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "listing.h"
#include <recon.h>
#undef printf
#undef fopen
static int diagnostics, reports, fail_open;
static char *request_name;
static unsigned diagnostic_cursor, diagnostic_head, diagnostic_enabled, diagnostic_segment;
FILE *recon_fopen(const char *name, const char *mode)
{
    if (strcmp(name, "requested.c") || strcmp(mode, "rb")) abort();
    return fail_open ? NULL : (FILE *)0x12345678;
}
void __fastcall report_context_error(unsigned code, char *source, char *filename)
{
    if (code != 0x53 || strcmp(source, "source") || strcmp(filename, "requested.c")) abort();
    ++reports;
}
void __fastcall compiler_internal_error(char *source, unsigned code)
{
    if (strcmp(source, "@(#)listing.c:1.76") || code != 0x5ed) abort();
    ++diagnostics;
    diagnostic_cursor = listing_storage.state.cursor == (void *)request_name;
    diagnostic_head = (unsigned)listing_storage.state.lines;
    diagnostic_enabled = listing_enabled;
    diagnostic_segment = (unsigned)listed_segment;
}
int main(int argc, char **argv)
{
    char names[20][24];
    char requested[] = "requested.c";
    int count, existing, i;
    if (argc != 4) return 2;
    count = atoi(argv[1]); existing = atoi(argv[2]); fail_open = atoi(argv[3]);
    request_name = requested;
    if (count < 0 || count > 20 || existing >= count) return 3;
    memset(&listing_storage, 0, sizeof(listing_storage));
    listing_source_count = count;
    for (i = 0; i < count; ++i) {
        sprintf(names[i], "existing%d.c", i);
        listing_sources[i].name = names[i];
        listing_sources[i].file = (FILE *)(0x1000 + i);
        listing_sources[i].lines = (ListingSourceLine *)(0x2000 + i);
        listing_sources[i].line = 10 + i;
    }
    listing_source_index = 0;
    listing_storage.state.cursor = (ListingSourceLine *)0xabc;
    listing_storage.state.lines = (ListingSourceLine *)0xdef;
    listing_enabled = 1; listed_segment = (Segment *)0x444;
    open_listing_source(existing >= 0 ? names[existing] : requested);
    printf("%d %d %d %d %u %u %u %u %u %u %u %u\n", listing_source_count,
        listing_source_index, diagnostics, reports, (unsigned)listing_storage.state.cursor,
        (unsigned)listing_storage.state.lines, listing_enabled, (unsigned)listed_segment,
        diagnostic_cursor, diagnostic_head, diagnostic_enabled, diagnostic_segment);
    for (i = 0; i < 21; ++i)
        printf("%u %u %ld\n", (unsigned)listing_sources[i].file,
            (unsigned)listing_sources[i].lines, listing_sources[i].line);
    return 0;
}
