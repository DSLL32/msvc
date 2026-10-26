/* Target1994 single1009-bin table, unlike later global/static tables. */
#include <string.h>
#include "cmdline.h"
#include <recon.h>

typedef struct symbol symbol;
static void *FreeSym;             /* original441f80; role provisional */
symbol **SymPtrs;                 /* original450210 */
int (*CmpSym)( const void *, const void *, unsigned ); /* original450214 */
symbol *HeadSym;                  /* original4500c0 */

void InitSym( void )
{
    FreeSym = 0;
    InitSymStorage();
    SymPtrs = ChkLAlloc( 1009 * sizeof(*SymPtrs) );
    memset( SymPtrs, 0, 1009 * sizeof(*SymPtrs) );
    CmpSym = memicmp;
    HeadSym = 0;
}

/* The original storage initializer at422f0c is a sole RET in this build.
 * Its historical source name is not known; the role name is provisional. */
void InitSymStorage( void )
{
}

#pragma pack(1)
struct symbol {
    struct symbol *hash;
    struct symbol *link;
    unsigned char prefix[10];
    unsigned short info;
    union { char *alias; void *seg; } p;
    union { void *import; unsigned long value; } u;
};
#pragma pack()

void FiniSym( void )
{
    symbol *sym;

    for( sym = HeadSym; sym != 0; sym = sym->link ) {
        if( (sym->info & 15) == 6 && !(FmtData.type & 0x8000) ) {
            if( sym->u.import != (void *)&FmtData ) {
                _LnkFree( sym->u.import );
            }
        } else if( (sym->info & 15) == 5 && (sym->info & 0x10) ) {
            _LnkFree( sym->p.alias );
        }
    }
    RelSymBlock();
    ReleasePass1();
    _LnkFree( SymPtrs );
}
