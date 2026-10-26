/* Object-file index arrays; actual1994 element sizes differ from2002. */
#include <string.h>
#include "cmdline.h"
#include <recon.h>

nodearray *GrpNodes;       /* original4500c8 */
nodearray *SegNodes;       /* original4500cc */
nodearray *ExtNodes;       /* original450118 */
nodearray *NameNodes;      /* original4500c4 */
static segdata *FreeSegDataList;  /* original441f50 */

void InitNodes( void )
{
    FreeSegDataList = 0;
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

mod_entry *NewModEntry( void )
{
    mod_entry *entry;

    entry = PermAlloc( sizeof( mod_entry ) );
    memset( entry, 0, sizeof( mod_entry ) );
    if( LinkFlags & 0x10000UL ) InitArcBuffer( entry );
    return entry;
}

void *AllocNode( nodearray *list )
{
    if( (list->num & 0xff) == 0 && (list->num >> 8) >= list->arraymax ) {
        list->array[list->arraymax] = ChkLAlloc( (unsigned)list->elsize << 8 );
        ++list->arraymax;
    }
    ++list->num;
    return FindNode( list, (unsigned short)list->num );
}

void *FindNode( nodearray *list, unsigned index )
{
    index = (unsigned short)(index - 1);
    return list->array[index >> 8] + (index & 0xff) * list->elsize;
}

segdata *AllocSegData( void )
{
    segdata *seg;

    if( FreeSegDataList != 0 ) {
        seg = FreeSegDataList;
        FreeSegDataList = seg->next;
    } else if( LinkState & 0x1000 ) {
        seg = PermAlloc( sizeof( segdata ) );
    } else {
        seg = Pass1Alloc( sizeof( segdata ) );
    }
    memset( seg, 0, sizeof( segdata ) );
    return seg;
}

void FreeSegData( segdata *seg )
{
    seg->next = FreeSegDataList;
    FreeSegDataList = seg;
}

void CollapseLazyExtdefs( void )
{
    IterateNodelist( ExtNodes, CollapseLazy );
}

void IterateNodelist( nodearray *list, void (*fn)(void *) )
{
    int index, limit;

    if( list->num == 0 ) return;
    limit = list->num >> 8;
    for( index = 0; index < limit; ++index ) {
        IterateNodeArray( list->array[index], fn, list->elsize, 256 );
    }
    IterateNodeArray( list->array[limit], fn, list->elsize, list->num & 0xff );
}

void IterateNodeArray( void *nodes, void (*fn)(void *), unsigned short size,
                        int limit )
{
    int index;
    char *node;

    node = nodes;
    for( index = 0; index < limit; ++index ) {
        (*fn)( node );
        node += size;
    }
}

void CollapseLazy( void *node )
{
    extnode *curr;
    symbol *sym;

    curr = node;
    sym = curr->entry;
    if( (sym->info & 15) >= 7 && !(curr->flags & 0x1000) ) {
        ClearSymUnion( sym );
        sym = curr->entry;
        sym->info = (sym->info & ~15) | 4;
        if( LinkState & 2 ) {
            LinkState |= 4;
            sym->info &= ~0x80;
        }
    }
}

/* Role names for the old address-info markers; these walkers disappeared
 * from the released source when per-module segment storage changed. */
void ResetAddrInfo( void )
{
    IterateNodelist( SegNodes, ResetSegAddr );
    WalkSavedSegs( ResetSavedSegAddr );
}

void ResetSegAddr( void *node )
{
    segnode *seg;
    seg_leader *leader;

    seg = node;
    if( !(seg->info & 0x0180) && seg->entry != 0 ) {
        leader = seg->entry;
        leader->info &= ~0x40;
    }
}

void ReleaseNames( void )
{
    IterateNodelist( NameNodes, ReleaseNamelist );
    FreeNodes( NameNodes );
}

void ReleaseNamelist( void *node )
{
    list_of_names *name;

    name = *(list_of_names **)node;
    if( name->flags != 0 ) {
        name->next_name = SavedNames;
        SavedNames = name;
    } else {
        _LnkFree( name );
    }
}

void ResetSegDataList( void )
{
    FreeSegDataList = 0;
}

extern segnode *FindSegdataArray( segnode *, int, seg_leader * );

segnode *FindSegdata( seg_leader *seg )
{
    int limit, i;
    segnode *node;

    if( SegNodes->num == 0 ) return 0;
    limit = SegNodes->num >> 8;
    for( i = 0; i < limit; ++i ) {
        node = FindSegdataArray( (segnode *)SegNodes->array[i], 256, seg );
        if( node != 0 ) return node;
    }
    return FindSegdataArray( (segnode *)SegNodes->array[limit], SegNodes->num & 255, seg );
}

segnode *FindSegdataArray( segnode *base, int count, seg_leader *seg )
{
    int i;

    for( i = 0; i < count; ++i, ++base )
        if( base->entry == seg ) return base;
    return 0;
}

unsigned GetNumNodes( nodearray *nodes )
{
    return nodes->num;
}

snamelist *IndexSName( snamelist *list, unsigned index )
{
    while( list != 0 && --index != 0 ) list = list->next;
    return list;
}
