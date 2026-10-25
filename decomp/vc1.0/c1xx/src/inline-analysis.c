/* Shared qc state encountered by address-taking. */
#include <stdlib.h>
#include "inline-analysis.h"
#include <recon.h>
unsigned short inline_base_region; /* original 0x912f0 */
int inline_scope_depth; /* original 0x912f8 */
InlineScope *inline_scopes; /* original 0x912fc */
unsigned inline_parameter_slots[10]; /* original 0x91304..0x9132c */
unsigned char inline_slot_count; /* original 0x91354 */

void allocate_inline_storage(void);

/* Original0x44630..0x44635. The original tail entry shares the ordinary
   no-argument cdecl contract with its storage allocator. */
void initialize_inline_queries(void)
{
    allocate_inline_storage();
}

InlineRegion *inline_regions; /* original0x912f4 */

/* Original0x450b7..0x4510f. Allocate the255 region rows and eight scope
   records, clearing the complete owned allocations before per-function setup. */
void allocate_inline_storage(void)
{
    unsigned i;
    inline_regions = malloc(255 * sizeof(InlineRegion));
    inline_scopes = malloc(8 * sizeof(InlineScope));
    if (!inline_regions || !inline_scopes) fatal_error(60);
    for (i = 0; i < 255 * sizeof(InlineRegion); ++i)
        ((unsigned char *)inline_regions)[i] = 0;
    for (i = 0; i < 8 * sizeof(InlineScope); ++i)
        ((unsigned char *)inline_scopes)[i] = 0;
}
