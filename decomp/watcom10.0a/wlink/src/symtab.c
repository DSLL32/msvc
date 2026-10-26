/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/

/* Target1994 single1009-bin table, unlike later global/static tables. */
#include <string.h>
#include <malloc.h>
#include "diagnostic.h"
#include "fixup.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(memcpy,strlen)
#endif

static symbol *SymList;           /* original441f80 */
symbol **SymPtrs;                 /* original450210 */
int (*CmpSym)( const void *, const void *, unsigned ); /* original450214 */
symbol *HeadSym;                  /* original4500c0 */

void InitSym( void )
{
    SymList = 0;
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

symbol *SymXOp( unsigned char op, char *name, int length )
{
    char *symname;

    symname = alloca( length + 1 );
    if( symname == 0 ) {
        name[length - 1] = '\0';
        LnkMsg( 0x3363, "s", name );
        symname = name;
    } else {
        memcpy( symname, name, length );
        symname[length] = '\0';
    }
    return SymOp( op, symname, length );
}

symbol *SymOp( unsigned char op, char *name, int length )
{
    symbol *sym;

    sym = DoSymOp( op, name, length );
    if( !(op & 4) ) {
        while( sym != 0 && (sym->info & 15) == 5 )
            sym = DoSymOp( op, sym->p.alias, sym->u.value );
    }
    if( sym != 0 ) {
        if( op & 0x20 ) {
            if( (sym->info & 15) == 5 && (sym->info & 0x10) ) {
                _LnkFree( sym->p.alias );
                sym->info &= ~0x10;
            }
            sym->info |= 0x1000;
        }
        if( op & 0x10 ) sym->info |= 0x40;
    }
    return sym;
}

unsigned NameLen;       /* original441280 */

symbol *DoSymOp( unsigned char op, char *symname, int length )
{
    unsigned hash;
    symbol *sym;
    unsigned searchlen;

    searchlen = length + 1;
    if( NameLen != 0 && NameLen < (unsigned)length ) searchlen = NameLen;
    if( NameLen != 0 && NameLen < (unsigned)length )
        hash = HashFn( symname, NameLen );
    else hash = HashFn( symname, length );
    sym = SearchSym( symname, hash, searchlen, (op & 1) ? 0x100 : 0 );
    if( sym == 0 && !(op & 8) ) {
        sym = AddSym( symname, length );
        sym->hash = SymPtrs[hash];
        SymPtrs[hash] = sym;
        if( op & 1 ) sym->info |= 0x100;
    }
    return sym;
}

/* Released ScatterTable agrees with all256 values used by the1994 hash. */
/* External linkage gives this genuine CODE data its own Watcom MAP public. */
const unsigned ScatterTable[] = {
0x1DE82EE1,
0x6E21134B,
0x0F406D5B,
0x07671454,
0x3FE530A8,
0x05D03ED8,
0x63060B8C,
0x02B93A2E,
0x065A15F3,
0x177C1A12,
0x1100789C,
0x3DB02031,
0x54160785,
0x77250A74,
0x091B1715,
0x799E7A0C,
0x5A05780C,
0x5A4D3F42,
0x4F21462B,
0x750F15FC,
0x3EAF75A2,
0x0B6C1F0E,
0x1A394B3E,
0x574C2246,
0x35B72B4F,
0x0D7A6536,
0x64646B5A,
0x0BDA22BF,
0x075E1283,
0x2F6A6F62,
0x6A0034B9,
0x0DFB1646,
0x0800549F,
0x0FAB3B09,
0x1947783E,
0x24294C47,
0x7F935C6B,
0x0FA812EB,
0x71CF72F9,
0x0BA06A38,
0x012E179F,
0x37476C96,
0x540927A8,
0x13D3211D,
0x327E685D,
0x086C75E3,
0x40C46832,
0x29F31306,
0x035046B3,
0x42755278,
0x08A90F2C,
0x546D688D,
0x550C121B,
0x15BD0946,
0x6BBF6956,
0x1E6B165B,
0x123512FC,
0x471C220A,
0x28E74F1D,
0x735152A2,
0x35ED2C29,
0x436372CE,
0x7D5E6E19,
0x6A3F3AA2,
0x436D3C5F,
0x22E21D67,
0x1440428E,
0x01942826,
0x2D8F2FCA,
0x1CE43CF7,
0x660240ED,
0x4A663707,
0x2E47417E,
0x6B2F436A,
0x07F52F53,
0x040D7AA6,
0x0E236D00,
0x1D8706FB,
0x6BC72F06,
0x27986374,
0x5BD00FBC,
0x5D2A7FAE,
0x0F0375FC,
0x6354546B,
0x13973A8F,
0x205334D6,
0x1A8F3856,
0x064A6894,
0x36D9333D,
0x4DB95A8F,
0x722B41DF,
0x5BBF6881,
0x539925F9,
0x500F1F42,
0x33F63F91,
0x76B46FD2,
0x7BF106E2,
0x19842926,
0x27EB390E,
0x6D6542B3,
0x2D992382,
0x174343BB,
0x1F5E3A28,
0x04CD535A,
0x29666450,
0x14F23D4F,
0x44822C5C,
0x4C1F778B,
0x3AC63E2D,
0x1035747C,
0x59E46051,
0x284C4E16,
0x6F4753E9,
0x7C2A15A6,
0x69EF475B,
0x4F815456,
0x3E0F47BD,
0x74EC087F,
0x66685CFC,
0x34A82BB2,
0x1F633CD0,
0x63120D84,
0x37F14060,
0x62E40F1D,
0x142308B4,
0x0F2F0B7F,
0x3D887DDF,
0x49187897,
0x1D4A14E8,
0x25520D06,
0x3C07087C,
0x755F76AC,
0x23707056,
0x28866DF9,
0x3DA4636A,
0x409331F2,
0x4E4A10D5,
0x56513905,
0x61717B84,
0x69C8228E,
0x4C2744A6,
0x7A373525,
0x1ADB30D2,
0x21D075AE,
0x097353F8,
0x3136231E,
0x612C1009,
0x06B7230D,
0x58355266,
0x1D492B01,
0x4F0B3C0A,
0x0F463274,
0x07932076,
0x08E779AB,
0x31F9075F,
0x66823136,
0x6B4B1D1A,
0x7A830E45,
0x09D43DC3,
0x372C57B1,
0x53390731,
0x380C37C6,
0x78551D9A,
0x67415ED2,
0x72D60F05,
0x3A6D426F,
0x20940173,
0x4379058F,
0x38B517CC,
0x110C554C,
0x7681398B,
0x1ACA466D,
0x16062929,
0x354D7FB8,
0x7E6B154A,
0x49E43CFA,
0x361510AA,
0x4E0E76D7,
0x7C7B1CC6,
0x36B43B52,
0x25C9334A,
0x632A031B,
0x14387A3D,
0x3BE40F9C,
0x6B280EEE,
0x00F81DC2,
0x5802139E,
0x0457798B,
0x0CD06BEA,
0x4BCB3E54,
0x489953FF,
0x56B04FE0,
0x097E3E7C,
0x03725827,
0x44952525,
0x50CA55E5,
0x3F89384E,
0x76B707DE,
0x67EE1902,
0x133F40F1,
0x446509F6,
0x66E370E3,
0x5B744330,
0x5CE36DE9,
0x2C225477,
0x5B364AF3,
0x14554279,
0x164746BB,
0x40EC4ABF,
0x64711F90,
0x039C7A51,
0x4D3957E3,
0x2A8B0328,
0x504E378E,
0x35AE7C58,
0x204115E8,
0x05E378FA,
0x4D043889,
0x61CD21DB,
0x1A2452DE,
0x6C733DE6,
0x7CC6726B,
0x69985551,
0x7D6363E3,
0x6BD85B8A,
0x79435CEA,
0x48870BE0,
0x7FBB76A4,
0x6D4964F2,
0x47264505,
0x73716ED8,
0x75A416D8,
0x0D1B3813,
0x39952A8C,
0x28083535,
0x6DD463C1,
0x623C6F4F,
0x45B34D96,
0x2C592D1F,
0x3CBB75B1,
0x35B902F6,
0x46206C0B,
0x7E4E756B,
0x78067381,
0x2A3B1AC2,
0x2E762F1A,
0x732E6719,
0x621F5819,
0x53840F58,
0x2ECC48C2,
0x471A2ED6,
0x45A967ED,
0x3DF41BCC,
0x4735414C,
0x1C90167A,
0x4106567B
};

unsigned HashFn( char *name, int len )
{
    unsigned value;

    value = 0;
    while( len > 0 ) {
        value = (value << 1) ^ ScatterTable[(unsigned char)*name | 0x20];
        ++name;
        --len;
    }
    return value % 1009;
}

symbol *SearchSym( char *name, unsigned hash, unsigned len, unsigned flags )
{
    symbol *sym;

    sym = SymPtrs[hash];
    while( sym != 0 ) {
        if( (!(sym->info & 0x100) || (sym->info & 0x800)) &&
            !((sym->info ^ flags) & 0x100) &&
            CmpSym( name, sym->name, len ) == 0 ) break;
        sym = sym->hash;
    }
    return sym;
}

void SymModStart( void )
{
    if( LastSym == 0 ) SymList = (symbol *)&HeadSym;
    else SymList = LastSym;
}

void ClearSymUnion( symbol *sym )
{
    unsigned type;
    void *save;

    type = sym->info & 15;
    if( type == 6 ) {
        if( FmtData.type & 0x001f ) _LnkFree( sym->u.import );
        sym->u.import = 0;
    } else if( type >= 10 ) {
        save = *(void **)sym->e.def;
        _LnkFree( sym->e.def );
        sym->e.def = save;
    } else if( type <= 3 ) {
        sym->u.import = 0;
    }
}

void SymModEnd( void )
{
    symbol *sym;

    if( SymList == (symbol *)&HeadSym ) sym = HeadSym;
    else sym = SymList;
    for( ; sym != 0; sym = sym->link ) {
        sym->info &= ~0x0820;
        sym->info |= 0x20;
        if( (sym->info & 0x40) && (sym->info & 0x1000) ) {
            sym->info &= ~0x0240;
            sym->info |= 0x0200;
        }
    }
    SymList = 0;
}

void XDefSymAddr( symbol *sym, unsigned long off, unsigned short frame )
{
    sym->addr.seg = frame;
    sym->addr.off = off;
}

void ConvertLazyRefs( void )
{
    symbol *defsym, *sym;
    unsigned type;

    for( sym = HeadSym; sym != 0; sym = sym->link ) {
        type = sym->info & 15;
        if( type >= 7 ) {
            if( type >= 10 ) {
                defsym = *(symbol **)sym->e.def;
                _LnkFree( sym->e.def );
            } else {
                defsym = sym->e.def;
                if( type == 7 && (sym->info & 0x2000) ) {
                    DefStripSym( sym, AllocSegData() );
                }
            }
            WeldSyms( sym, defsym );
            if( LinkFlags & 0x02000000UL ) LnkMsg( 0x2090, "S", sym );
        }
    }
}

void CheckSymbolPass( int pass )
{
    symbol *sym;

    for( sym = HeadSym; sym != 0; sym = sym->link ) {
        if( (sym->info & 15) <= 1 && sym->u.value != 0 ) {
            if( pass == 1 ) BSSPass1( sym );
            else BSSPass2( sym );
        }
    }
}

symbol *RefISymbol( char *name )
{
    return SymOp( 0x12, name, strlen( name ) );
}

void ReportUndefined( void )
{
    symbol *sym;

    for( sym = HeadSym; sym != 0; sym = sym->link ) {
        sym->info &= 0x1f5f;
        if( !(sym->info & 0x1000) ) LnkMsg( 0x321c, "S", sym );
    }
}

extern void WriteSym( symbol *, char );

void XReportSymAddr( symbol *sym )
{
    char star;

    if( sym->info & 0x40 ) star = ' ';
    else if( sym->info & 0x0200 ) star = '+';
    else star = '*';
    WriteSym( sym, star );
    WriteMapNL( 1 );
}

void WriteSym( symbol *sym, char star )
{
    msg_address addr;

    if( sym->info & 0x0100 ) star = 's';
    addr.off = sym->addr.off;
    addr.seg = sym->addr.seg;
    WriteFormat( 0, "%a%c", addr, star );
    WriteFormat( 15, "%S", sym );
}

extern unsigned char SkipSymbol( symbol * );
extern void WriteUndefHead( void );

void WriteUndefinedMap( void )
{
    symbol *sym;
    unsigned char first;

    first = 1;
    for( sym = HeadSym; sym != 0; sym = sym->link ) {
        if( (sym->info & 15) <= 1 && !(sym->info & 0x0100) && !SkipSymbol( sym ) ) {
            if( first ) {
                WriteUndefHead();
                first = 0;
            }
            WriteSym( sym, ' ' );
            WriteMapNL( 1 );
        }
    }
}

symbol *FindISymbol( char *name )
{
    return SymOp( 0x18, name, strlen( name ) );
}

void SetPubCurr( symbol *sym, unsigned char set )
{
    for( ; sym != 0; sym = sym->publink ) {
        if( set ) sym->info |= 0x0800;
        else sym->info &= ~0x0800;
    }
}

void GetSymAddr( symbol *sym, fixup_ref *ref )
{
    unsigned kind;
    unsigned char copy_addr;
    seg_leader *seg;

    if( sym->info & 0x1000 ) ref->flags |= 1;
    else ref->flags &= ~1;
    if( FmtData.type & 0x4200 ) kind = sym->info >> 13;
    else kind = (sym->info & 0x8000) ? 7 : 0;
    ref->flags = (ref->flags & 0x00ffffffUL) | ((unsigned long)kind << 24);
    copy_addr = 1;
    if( (FmtData.type & 0x811f) && (sym->info & 15) == 6 ) {
        ref->import = sym->u.import;
        copy_addr = ref->import == 0;
    }
    if( copy_addr ) {
        ref->addr.off = sym->addr.off;
        ref->addr.seg = sym->addr.seg;
        seg = sym->p.seg;
        if( seg == 0 || !(seg->info & 0x0038) ) ref->flags |= 2;
    }
}

extern char *ImpModuleName( dll_sym_info * );

void XWriteImports( void )
{
    symbol *sym;

    for( sym = HeadSym; sym != 0; sym = sym->link ) {
        if( (sym->info & 15) == 6 && sym->u.import != 0 ) {
            if( !(FmtData.type & 0x100) || sym->u.import != (dll_sym_info *)&FmtData ) {
                WriteFormat( 0, "%s", sym->name );
                if( FmtData.type & 0x1f ) {
                    WriteFormat( 36, "%s", ImpModuleName( sym->u.import ) );
                }
                WriteMapNL( 1 );
            }
        }
    }
}
