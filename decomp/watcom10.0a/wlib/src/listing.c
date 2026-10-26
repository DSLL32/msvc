/* Alphabetical and per-module listing of the final public symbol set. */
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <io.h>
#include "toolstate.h"
#include <recon.h>

/* Original0x411462..0x41150c plus the shared return at4113e8. The
   original keeps this temporary pointer array until process termination. */
void ListLibrary( void )
{
    SymbolRecord *symbol;
    SymbolRecord **symbols;
    unsigned count, bucket, index;

    count = 0;
    for( bucket = 0; bucket < 27; ++bucket ) {
        for( symbol = (SymbolRecord *)SymbolHash[bucket]; symbol != NULL; symbol = symbol->Next ) {
            if( !(symbol->Flags & 0x80) ) ++count;
        }
    }
    symbols = AllocMemory( count * sizeof(*symbols) + sizeof(*symbols) );
    index = 0;
    for( bucket = 0; bucket < 27; ++bucket ) {
        for( symbol = (SymbolRecord *)SymbolHash[bucket]; symbol != NULL; symbol = symbol->Next ) {
            if( !(symbol->Flags & 0x80) ) symbols[index++] = symbol;
        }
    }
    qsort( symbols, count, sizeof(*symbols), CompareSymbols );
    PrintPublicListing( symbols, count );
    qsort( symbols, count, sizeof(*symbols), CompareModuleSymbols );
    PrintModuleListing( symbols, count );
}

/* Original0x41108d..0x41109c is the two-register qsort callback.
   The pointed-to array elements are SymbolRecord pointers. */
int CompareSymbols( const void *first, const void *second )
{
    SymbolRecord *left, *right;

    left = *(SymbolRecord * const *)first;
    right = *(SymbolRecord * const *)second;
    return( CompareNames( left->Name, right->Name ) );
}

/* Original0x41112b..0x411226 lays out two symbol/module pairs per
   line, extending a long name into the second column when necessary. */
void PrintPublicListing( SymbolRecord **symbols, unsigned count )
{
    SymbolRecord *module;
    const char *name;
    unsigned index, length;
    int column, padding;

    column = 0;
    for( index = 0; index < count; ++index ) {
        module = symbols[index]->Value.Owner;
        name = FormatSymbolName( symbols[index]->Name );
        length = 0;
        while( name[length] != '\0' ) ++length;
        padding = 28 - length - module->NameLength;
        while( padding <= 0 ) {
            if( !column ) {
                padding += 40;
                if( padding <= 0 ) padding = 2;
                column = 1;
            } else {
                column = 0;
                ListingNewline( ListingHandle );
            }
        }
        WriteListingText( name, length, ListingHandle );
        FillListing( padding, '.' );
        WriteListingText( module->Name, module->NameLength, ListingHandle );
        if( !column ) {
            WriteListingText( "            ", 12, ListingHandle );
            column = 1;
        } else {
            ListingNewline( ListingHandle );
            column = 0;
        }
    }
    if( column == 1 ) ListingNewline( ListingHandle );
}

/* Original0x4120bf..0x4120d3. Text translation belongs to the open
   descriptor, keeping the application byte strings unchanged. */
void WriteListingText( const char *text, unsigned length, int handle )
{
    write( handle, text, length );
}

/* Original0x4110c3..0x41112b. The period alloca intrinsic supplies
   caller-frame storage; the original low-stack fallback writes one byte. */
void FillListing( int count, unsigned char fill )
{
    char *buffer;

    if( count <= 0 ) count = 1;
    buffer = alloca( count );
    if( buffer != NULL ) {
        memset( buffer, fill, count );
        WriteListingText( buffer, count, ListingHandle );
    } else {
        while( count != 0 ) {
            WriteListingCharacter( fill, ListingHandle );
            --count;
        }
    }
}

/* Original0x4120d3..0x4120f0. */
void ListingNewline( int handle )
{
    CheckBreak();
    write( handle, "\n", 1 );
}

/* Original0x41109c..0x4110c3 groups symbols by the owning module's
   page and uses the alphabetical callback to order each group. */
int CompareModuleSymbols( const void *first, const void *second )
{
    int left, right;

    left = (*(SymbolRecord * const *)first)->Value.Owner->Value.Page;
    right = (*(SymbolRecord * const *)second)->Value.Owner->Value.Page;
    if( left == right ) return( CompareSymbols( first, second ) );
    return( left - right );
}

int ListingColumn;                  /* Original0x4302b4. */

/* Original0x4113ef..0x411462 separates module groups and emits the
   module heading before its public symbols. */
void PrintModuleListing( SymbolRecord **symbols, unsigned count )
{
    SymbolRecord *module, *current;
    unsigned index;

    ListingColumn = 0;
    current = NULL;
    for( index = 0; index < count; ++index ) {
        module = symbols[index]->Value.Owner;
        if( module != current ) {
            if( ListingColumn > 0 ) ListingNewline( ListingHandle );
            ListingColumn = 0;
            ListingNewline( ListingHandle );
            PrintModuleHeading( module );
            current = module;
        }
        PrintModuleSymbol( symbols[index] );
    }
    if( ListingColumn > 0 ) ListingNewline( ListingHandle );
}

/* Original0x411226..0x4112ce prints the module's library byte offset
   as eight lowercase hexadecimal digits followed by the historical H suffix. */
void PrintModuleHeading( SymbolRecord *module )
{
    char offset[12];
    unsigned length;

    WriteListingText( module->Name, module->NameLength, ListingHandle );
    FillListing( 14 - module->NameLength, ' ' );
    WriteListingText( "Offset=", 7, ListingHandle );
    utoa( (unsigned)module->Value.Page * LibraryPageSize, offset, 16 );
    length = 0;
    while( offset[length] != '\0' ) ++length;
    FillListing( 8 - length, '0' );
    offset[length] = 'H';
    WriteListingText( offset, length + 1, ListingHandle );
    ListingNewline( ListingHandle );
}

/* Original0x4112ce..0x4113ef. The final seven-byte return sequence is
   also used by ListLibrary; each logical function keeps its real entry. */
void PrintModuleSymbol( SymbolRecord *symbol )
{
    const char *text;
    unsigned length;
    int padding, columns, adjusted;

    if( ListingColumn > 3 ) {
        ListingNewline( ListingHandle );
        ListingColumn = 0;
    }
    text = FormatSymbolName( symbol->Name );
    length = 0;
    while( text[length] != '\0' ) ++length;
    padding = 16 - length;
    if( padding >= 0 ) {
        WriteListingText( "  ", 2, ListingHandle );
        WriteListingText( text, length, ListingHandle );
        FillListing( padding, ' ' );
        ++ListingColumn;
    } else {
        adjusted = length - 14;
        if( adjusted < 0 ) adjusted = 1;
        columns = (adjusted + 19) / 20 + 1;
        if( (unsigned)(4 - columns) < (unsigned)ListingColumn &&
            ListingColumn != 0 ) {
            ListingNewline( ListingHandle );
            ListingColumn = 0;
        }
        WriteListingText( "  ", 2, ListingHandle );
        WriteListingText( text, length, ListingHandle );
        FillListing( columns * 20 - length - (columns * 2 - 2) - 4, ' ' );
        ListingColumn += columns;
    }
}
