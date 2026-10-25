/* Object-file index arrays; actual1994 element sizes differ from2002. */
#include "link.h"
#include <recon.h>

nodearray *GrpNodes;       /* original4500c8 */
nodearray *SegNodes;       /* original4500cc */
nodearray *ExtNodes;       /* original450118 */
nodearray *NameNodes;      /* original4500c4 */
static void *NodeBlocks;   /* original441f50; role remains provisional */

void InitNodes( void )
{
    NodeBlocks = 0;
    GrpNodes = MakeArray( 4 );
    SegNodes = MakeArray( 20 );   /* target segnode, not the later16B record */
    ExtNodes = MakeArray( 6 );    /* target external entry and16-bit flags */
    NameNodes = MakeArray( 4 );
}
