/* 1994 command-line callback, VA4243c8..424525. The initial zero
 * command pointer and the full final link phases belong to this encountered
 * entry; no independent entry into its interior has been established.
 * Later released wlink.c correlates phase names; target order is authoritative. */
#include <string.h>
#include "cmdline.h"
extern char *RemovePath( char *, int * );
#include <recon.h>
#if defined(__WATCOMC__)
#pragma intrinsic(strlen)
#endif

/* Unlike the later bool-returning import-prefix search, the1994 routine
 * has no defined result when the library list is empty. Callers ignore it. */
void LibFind( char *name, unsigned char old_sym )
{
    file_list *lib;

    for( lib = ObjLibFiles; lib != 0; lib = lib->next_file ) {
        if( lib->file->flags & 4 ) continue;
        if( old_sym && (lib->status & 0x1000) ) continue;
        if( LibProcFile( lib, name ) ) break;
    }
}

/* File-local ProcLibFile in the released source; the command parser has
 * another ProcLibFile, so its native/public identifier is qualified here. */
unsigned char LibProcFile( file_list *lib, char *name )
{
    mod_entry *mod, **prev;

    mod = SearchLib( lib, name );
    if( mod == 0 ) {
        CacheClose( lib, 1 );
        return 0;
    }
    lib->status |= 0x2000;
    if( LinkFlags & 4 ) {
        if( lib->status & 0x0800 ) mod->modinfo |= 0x1000;
        AddModTable( mod, lib->ovlref );
    } else {
        prev = &LibModules;
        while( *prev != 0 ) prev = &(*prev)->next_mod;
        *prev = mod;
    }
    CurrMod = mod;
    IdentifyObject( mod->f.source, mod->location );
    ObjPass1();
    CacheClose( mod->f.source, 1 );
    if( LinkFlags & 4 ) FinishArcs( mod );
    if( FindLibTrace( mod ) ) TraceSymList( mod->publist );
    return 1;
}

unsigned char ModNameCompare( char *name, char *member )
{
    int len;

    name = RemovePath( name, &len );
    return strlen( member ) == len && memicmp( name, member, len ) == 0;
}
