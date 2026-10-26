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

struct nodearray {
    unsigned num;
    unsigned short elsize, arraymax;
    char *array[128];
};
typedef char nodearray_is_520[(sizeof(nodearray)==520)?1:-1];

nodearray *MakeArray( unsigned size )
{
    nodearray *nodes;

    nodes = ChkLAlloc( sizeof( *nodes ) );
    nodes->num = 0;
    nodes->arraymax = 0;
    nodes->elsize = size;
    /* The original leaves array slots alone until they are allocated. */
    return nodes;
}

void FreeNodes( nodearray *nodes )
{
    nodes->num = 0;
}

void BurnNodes( void )
{
    BurnNodeArray( GrpNodes );
    BurnNodeArray( SegNodes );
    BurnNodeArray( ExtNodes );
    BurnNodeArray( NameNodes );
}

void BurnNodeArray( nodearray *list )
{
    int index;

    /* This older arraymax is a count, not the later final-array index. */
    for( index = 0; index < list->arraymax; ++index ) {
        _LnkFree( list->array[index] );
    }
    _LnkFree( list );
}
