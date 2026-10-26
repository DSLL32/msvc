/* 1994 command-line callback, VA4243c8..424525. The initial zero
 * command pointer and the full final link phases belong to this encountered
 * entry; no independent entry into its interior has been established.
 * Later released wlink.c correlates phase names; target order is authoritative. */
#include <stdlib.h>
#include <string.h>
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(strlen,memcpy)
#endif

void FindLibPaths( void )
{
    AddFmtLibPaths();
    if( LinkState & 0x80 ) AddEnvPaths( "LIB386" );
    else {
        AddEnvPaths( "LIB286" );
        HintFormat( 0x00e9 );
    }
    AddEnvPaths( "LIB" );
}

void AddEnvPaths( char *name )
{
    char *value;

    value = getenv( name );
    if( value != 0 ) AddLibPaths( value, strlen( value ), 0 );
}

void AddLibPaths( char *name, unsigned len, unsigned char front )
{
    path_entry *entry;
    file_list *file;

    entry = ChkLAlloc( sizeof(*entry) + len );
    memcpy( entry->name, name, len );
    entry->name[len] = 0;
    if( front ) {
        entry->next = LibPath;
        LibPath = entry;
    } else LinkList( &LibPath, entry );
    if( entry == LibPath ) {
        for( file = ObjLibFiles; file != 0; file = file->next_file )
            file->file->path_list = LibPath;
        for( file = Root->files; file != 0 && (file->file->flags & 1);
             file = file->next_file ) file->file->path_list = LibPath;
    }
}
