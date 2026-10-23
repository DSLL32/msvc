/* Native host configuration, outside the reconstructed function graph.
 * Original-only bind-apis capture proves FlushViewOfFile receives this loaded
 * module's36-byte workspace atVA6d470 (RVA5d470), not a file mapping. No PE is loaded here.
 */
#include "../src/editor-image.h"
#include "../src/import-library.h"
#include <recon.h>
static void __attribute__((constructor(200))) register_link_module_memory(void)
{
    static const ReconModuleSpec module={"LINK.EXE",
        "R:\\toolchains\\1.0-nt\\bin\\LINK.EXE",
        "3a53166175a2e98179d4da41bb0f3bfafec0ecf78884441736f138c8ab4d3718",
        RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS};
    static const ReconModuleMemorySpec workspace={
        "3a53166175a2e98179d4da41bb0f3bfafec0ecf78884441736f138c8ab4d3718",
        0x5d470,36,0x10000,0x6d470};
    HMODULE handle=recon_register_module(&module);
    if(!handle)recon_unsupported("LINK module workspace registration");
    recon_register_module_memory(handle,&workspace,&editor_workspace,sizeof(editor_workspace));
}

/* Native-only policy, outside the original graph. The buffered function-export
 * path serializes a saved original heap pointer from _lseek's stack frame.
 * Until that undefined path has a reviewable transport, fail before it emits
 * a false equivalent auxiliary record. Mapped scalar residue remains supported.
 */
extern unsigned char mapped_files;
void __real_write_import_members(Image *,const ImportMachine *);
void __wrap_write_import_members(Image *image,const ImportMachine *machine)
{
    HashTable *hash=image->symbols->hash;
    unsigned long bucket;
    HashNode *node;
    External *external;
    if (!mapped_files) {
        for (bucket=0;bucket<hash->bucket_count;++bucket) {
            node=hash->directory[bucket/hash->buckets_per_segment]->buckets[
                bucket%hash->buckets_per_segment];
            for (;node;node=node->next) {
                external=node->value;
                if ((external->flags&EXTERNAL_DEFINED) &&
                    !(external->flags&0x800) && !(external->flags&0x2008))
                    recon_unsupported("LINK buffered function-export auxiliary leaks original heap pointer");
            }
        }
    }
    __real_write_import_members(image,machine);
}
