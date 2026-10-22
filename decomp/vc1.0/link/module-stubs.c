/* Unreached module-list and Macintosh section operations. */
#include "modules.h"
void initialize_mac_section(Section *section) { __asm { int 3 } }
void set_mac_resource(IMAGE_SECTION_HEADER *header,Section *section,unsigned char apply) { __asm { int 3 } }
void end_contribution_enumerator(ContributionEnumerator *scan) { __asm { int 3 } }
