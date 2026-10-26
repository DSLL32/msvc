/* Counted filename/member comparisons use the librarian's initialized
   ASCII folding table, independent of the CRT locale. */
#include <stdlib.h>
#include <string.h>
#include "toolstate.h"
#include <recon.h>

/* Original0x41334f..0x41337f. */
int NamesEqualIgnoreCase( const unsigned char *first, const unsigned char *second,
                          unsigned first_length, unsigned second_length )
{
    if( first_length != second_length ) return( 0 );
    while( first_length != 0 ) {
        if( UpperCaseMap[*first] != UpperCaseMap[*second] ) return( 0 );
        ++first;
        ++second;
        --first_length;
    }
    return( 1 );
}

SymbolRecord *CurrentModuleSymbol; /* Original0x4302ac. */

/* Original0x4135a2..0x413603 allocates the counted name plus its NUL.
   A module gets its page number; a public symbol links back to that module. */
int AddMemberSymbol( const char *name, unsigned length, unsigned module_name )
{
    SymbolRecord *symbol;
    unsigned i;

    symbol = AllocMemory( length + 11 );
    symbol->NameLength = length;
    i = 0;
    do {
        symbol->Name[i] = name[i];
    } while( name[i++] != 0 );
    if( module_name == 0 ) {
        symbol->Flags &= 0x80;
        symbol->Flags |= 2;
        symbol->Value.Owner = CurrentModuleSymbol;
    } else {
        symbol->Value.Page = CurrentMemberOffset / LibraryPageSize;
    }
    return( InsertSymbol( symbol, module_name ) );
}

/* Original0x413d52..0x413e96. Stronger definitions replace weaker
   ones. Equal public definitions may coexist, with a diagnostic; module
   names instead reject a duplicate. Bucket linkage always stays original. */
int InsertSymbol( SymbolRecord *symbol, unsigned module_name )
{
    SymbolRecord **bucket, **link;
    SymbolRecord *existing;
    unsigned new_strength, old_strength;

    if( module_name != 0 ) bucket = (SymbolRecord **)NameHash;
    else bucket = (SymbolRecord **)SymbolHash;
    bucket += HashSymbol( symbol );
    link = bucket;
    while( (existing = *link) != NULL ) {
        if( !(*NamesEqual)( (unsigned char *)existing->Name,
                           (unsigned char *)symbol->Name,
                           existing->NameLength, symbol->NameLength ) ) {
            link = &existing->Next;
            continue;
        }
        if( module_name == 0 && IsCommonRef() ) {
            link = &existing->Next;
            continue;
        }
        new_strength = symbol->Flags & 0x7f;
        old_strength = existing->Flags & 0x7f;
        if( new_strength > old_strength ) {
            *link = existing->Next;
            FreeMemory( existing );
            continue;
        }
        if( new_strength < old_strength || new_strength != 2 ) {
            existing->Flags |= 0x80;
            FreeMemory( symbol );
            return( 0 );
        }
        BuildFileName( CurrentName );
        ReportDuplicateSymbol( symbol, module_name );
        if( module_name != 0 ) return( 1 );
        link = &existing->Next;
    }
    symbol->Next = *bucket;
    symbol->Flags &= 0x7f;
    *bucket = symbol;
    if( module_name != 0 ) {
        CurrentModuleSymbol = symbol;
        AddModuleIndex( symbol );
    }
    ++SymbolCount;
    if( SymbolCount == 0x23fdc0UL ) {
        ReportError( 2, 19 );
        ToolFlags |= 0x40;
        AbortCommand();
    }
    DictionarySize += (symbol->NameLength + (module_name != 0)) | 1;
    return( 0 );
}

/* Original0x413e96..0x413ec6 selects one of27 buckets from the folded
   first/last characters and length. For zero length the original's last
   byte access lands on the preceding zero NameLength byte. */
unsigned HashSymbol( SymbolRecord *symbol )
{
    unsigned first, last;

    first = (unsigned char)symbol->Name[0];
    last = symbol->NameLength != 0 ?
           (unsigned char)symbol->Name[symbol->NameLength - 1] : 0;
    return( (symbol->NameLength + UpperCaseMap[first] + UpperCaseMap[last]) % 27 );
}

ModuleIndex *ModuleIndexTail; /* Original0x4301bc. */

/* Original0x413f37..0x413f80 retains module order separately from the
   hashed names. Index nodes borrow their symbol entries; cleanup owns nodes. */
void AddModuleIndex( SymbolRecord *symbol )
{
    ModuleIndex *index;

    index = AllocMemory( sizeof(ModuleIndex) );
    if( ModuleIndexHead == NULL ) ModuleIndexHead = (WorkItem *)index;
    else ModuleIndexTail->Next = index;
    ModuleIndexTail = index;
    index->Symbol = symbol;
    index->Next = NULL;
}

/* Original0x416b1b..0x416b46. The1994 lib_misc helper recognizes only
   the W? and truncated T? prefixes; its length formal is unused. */
int __is_mangled( const char *name, unsigned length )
{
    unsigned prefix;

    prefix = 2;
    if( name[0] == 'T' ) prefix = 6;
    else if( name[0] != 'W' ) return( 0 );
    return( name[1] == '?' ? prefix : 0 );
}

/* Original0x413f80..0x41400c shares the seven-byte frame return at413934.
   The counted OMF spelling may contain a source path and extension. */
void NormalizeModuleName( SymbolRecord *module )
{
    char original[260], parts[264];
    char *name, *extension;
    unsigned i, length;

    for( i = 0; i < module->NameLength; ++i ) original[i] = module->Name[i];
    original[i] = '\0';
    _splitpath2( original, parts, NULL, NULL, &name, &extension );
    length = 0;
    while( name[length] != '\0' ) ++length;
    if( length != module->NameLength ) {
        i = 0;
        do { module->Name[i] = name[i]; } while( name[i++] != '\0' );
        module->NameLength = length;
    }
}

/* Original0x4133c1..0x41341d. A matching command records that it has
   been consumed; bit1 specifies ownership of its separately stored name. */
unsigned MatchRemoveCommand( const char *name, unsigned length,
                             MemberCommand *remove )
{
    for( ; remove != NULL; remove = remove->Next ) {
        if( (signed char)remove->NameLength == -1 ) continue;
        if( NamesEqualIgnoreCase( name, remove->Name, length,
                                  (signed char)remove->NameLength ) ) {
            SkipCurrentModule = 1;
            remove->Flags |= 1;
            return( (remove->Flags >> 1) & 1 );
        }
    }
    SkipCurrentModule = 0;
    return( 0 );
}

/* Original0x41341d..0x4134b2. Explicit extracts borrow their command
   node; extract-all uses the reusable node and configured output path. */
void MatchExtractCommand( const char *name, unsigned length, FileName *extract )
{
    if( ToolFlags & 0x4000 ) {
        ExplodeName->NameLength = length;
        ExplodeName->Name = (char *)name;
        ExplodeName->Path = &ExtractionPath;
        ExtractCurrentModule = 1;
        CurrentExtractName = ExplodeName;
        if( ToolFlags & 2 ) SkipCurrentModule = 1;
        return;
    }
    for( ; extract != NULL; extract = extract->Next ) {
        if( (signed char)extract->NameLength == -1 ) continue;
        if( NamesEqualIgnoreCase( name, extract->Name, length,
                                  (signed char)extract->NameLength ) ) {
            CurrentExtractName = extract;
            ExtractCurrentModule = 1;
            return;
        }
    }
    ExtractCurrentModule = 0;
}

/* Original0x413ec6..0x413ef7. Message pairs distinguish duplicate
   module/public names and whether a containing filename is available. */
void ReportDuplicateSymbol( SymbolRecord *symbol, unsigned module_name )
{
    unsigned message;

    message = module_name ? 0 : 2;
    if( FileErrorName[0] == '\0' ) ++message;
    ReportError( 3, message + 36, symbol->Name, FileErrorName );
}

/* Original0x41337f..0x4133a8 uses the librarian's own folding table,
   returning the first byte difference for case-independent listing order. */
int CompareNames( const unsigned char *first, const unsigned char *second )
{
    unsigned left, right;

    for( ;; ) {
        left = UpperCaseMap[*first];
        right = UpperCaseMap[*second];
        if( left != right || right == 0 ) return( left - right );
        ++first;
        ++second;
    }
}
