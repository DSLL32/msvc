/* Isolated native writer probe; not linked into C23232.EXE. */
#include "coff-writer.h"
#include <stdlib.h>
#include <string.h>
#include <recon.h>
/* Keep the probe's report on host stdout, independent of the tested shim. */
#undef printf
static unsigned write_sizes[8], writes, blocks;
static unsigned char expected_fill;
static const void *first_data;
static void *__fastcall probe_allocate(unsigned bytes)
{
    void *result;
    if (bytes != sizeof(CoffDataBlock)) abort();
    ++blocks;
    result = calloc(1, bytes);
    if (!result) abort();
    return result;
}
size_t recon_fwrite(const void *data, size_t size, size_t count, FILE *file)
{
    /* Deliberately do not read beyond the original80-byte scratch. This
       observes direct-path call requests, not its out-of-contract overread. */
    const unsigned char *bytes = data;
    unsigned i;
    if (writes == 8 || count != 1 || file != coff_file) abort();
    if (writes && first_data != data) abort();
    first_data = data;
    for (i = 0; i < 80; ++i) if (bytes[i] != expected_fill) abort();
    write_sizes[writes++] = size;
    return 1;
}
void __fastcall compiler_internal_error(char *source, unsigned code) { abort(); }
static void __fastcall probe_error(unsigned code, char *source, unsigned line) { abort(); }
int main(int argc,char **argv)
{
    CoffScope scope;
    CoffDataBlock *initial,*block;
    unsigned bytes,value,i;
    if (argc != 7) return 2;
    memset(&scope,0,sizeof(scope));
    scope.kind = strtoul(argv[1],0,10);
    scope.value = strtoul(argv[2],0,10);
    scope.buffered_bytes = strtoul(argv[3],0,10);
    bytes = strtoul(argv[4],0,10);
    value = strtoul(argv[5],0,10);
    expected_fill = (unsigned char)value;
    scope.completed_bytes = strtoul(argv[6],0,10);
    scope.write_count = 7;
    initial = calloc(1,sizeof(*initial));
    if (!initial) abort();
    initial->next = initial;
    memset(initial->bytes,0xcc,sizeof(initial->bytes));
    scope.block = initial;
    coff_active_scope = &scope;
    coff_free_blocks = NULL;
    coff_allocator = probe_allocate;
    coff_error_handler = probe_error;
    coff_file = (FILE *)1;
    coff_fill_bytes((unsigned char)value,bytes);
    printf("%u %u %u %u\n",scope.buffered_bytes,scope.completed_bytes,scope.write_count,blocks);
    printf("%u",writes);for(i=0;i<writes;++i)printf(" %u",write_sizes[i]);printf("\n");
    block = initial;
    do {
        for(i=0;i<500;++i)printf("%02x",block->bytes[i]);
        printf("\n");
        block = block->next;
    } while(block != initial);
    return 0;
}
