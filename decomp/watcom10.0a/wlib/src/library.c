/* OMF library layout and member traversal in the1994 librarian. */
#include <malloc.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include "toolstate.h"
#include <recon.h>

unsigned short InputLibraryPageSize; /* Original0x430314. */

/* Original0x4134b2..0x413547 chooses the new library page size,
   starts output bookkeeping, and traverses any existing input members. */
void PrepareLibrary( void )
{
    LibraryPageSize = RequestedPageSize;
    if( ToolFlags & 0x210 ) {
        if( !(ToolFlags & 0x20) ) LibraryPageSize = 16;
    } else {
        if( ReadLibraryHeader() ) return;
        SeekBuffered( CurrentName->File, 0 );
        if( !(ToolFlags & 0x20) ) LibraryPageSize = InputLibraryPageSize;
    }
    if( !(ToolFlags & 2) ) {
        WriteBufferedByte( OutputLibraryName->File, 0xf1 );
        PadLibraryPage( LibraryPageSize );
        ProcessAddedModules();
    }
    if( !(ToolFlags & 0x10) ) CopyLibraryMembers();
}

/* Original0x411746..0x4117e9. LIBHDR's record length plus its3-byte
   prefix defines a power-of-two page; the directory is read separately. */
unsigned char ReadLibraryHeader( void )
{
    unsigned char *header;
    unsigned page, bits, value;

    header = ReadBuffered( CurrentName->File, 10 );
    if( header[0] != 0xf0 ) {
        if( CurrentName == LibraryName ) ReportError( 2, 34 );
        else ReportError( 2, 26, FileErrorName );
        ToolFlags |= 0x40;
        return( 7 );
    }
    page = (header[1] | ((unsigned)header[2] << 8)) + 3;
    bits = 0;
    for( value = page; value != 0; value >>= 1 ) bits += value & 1;
    if( bits != 1 ) {
        if( CurrentName == LibraryName ) ReportError( 2, 35 );
        else ReportError( 2, 33, FileErrorName );
        ToolFlags |= 0x40;
        return( 7 );
    }
    InputLibraryPageSize = page;
    return( 0 );
}

/* Original0x41182d..0x4118a3 plus its7-byte return suffix41173f.
   The period alloca macro checks stackavail and can select the byte loop;
   the native host projects the normal small caller-stack allocation. */
void PadLibraryPage( unsigned size )
{
    unsigned remaining;
    unsigned char *padding;
    OpenFile *file;

    file = OutputLibraryName->File;
    remaining = size - BufferedPosition( file ) % size;
    if( remaining == size ) return;
    padding = alloca( remaining );
    if( padding != NULL ) {
        memset( padding, 0, remaining );
        WriteBuffered( file, padding, remaining );
    } else {
        while( remaining != 0 ) {
            WriteBufferedByte( file, 0 );
            --remaining;
        }
    }
}

/* Original0x41393b..0x413a3b walks pending additions, distinguishing
   synthetic imports, nested libraries, executables and ordinary OMF objects. */
void ProcessAddedModules( void )
{
    FileName *name;
    int first;

    for( name = (FileName *)CommandWork; name != NULL; name = name->Next ) {
        CurrentName = name;
        if( name->Flags == 3 ) {
            BuildImportMember( (ImportCommand *)name );
            continue;
        }
        if( !OpenLibraryFile( name, 0x200 ) ) {
            BuildFileName( CurrentName );
            ReportError( 2, 51, FileErrorName );
            ToolFlags |= 0x40;
            continue;
        }
        first = ReadBufferedByteStrict( CurrentName->File );
        SeekBuffered( CurrentName->File, 0 );
        if( first == 0xf0 ) {
            if( ReadLibraryHeader() ) return;
            NextLibraryPage();
        } else if( first == 'M' || first == 'N' ) {
            if( !LoadExecutableCache( CurrentName->File ) ) {
                CloseLibraryFile( CurrentName );
                BuildFileName( CurrentName );
                ReportError( 2, 53 );
                ToolFlags |= 0x40;
                continue;
            }
            InputLibraryPageSize = 1;
        } else {
            InputLibraryPageSize = 1;
        }
        ProcessModuleRecords( (MemberCommand *)MemberWork, NULL, 0 );
        CloseLibraryFile( CurrentName );
    }
}

unsigned long CurrentMemberOffset; /* Original0x4301b4. */

/* Original0x413603..0x41393b emits THEADR, the DLL-import COMENT,
   and MODEND records directly. Keep their checksums with the corresponding
   record fields, rather than storing a prebuilt object template. */
void BuildImportMember( ImportCommand *command )
{
    OpenFile *file;
    char *symbol, *module, *entry;
    unsigned long ordinal;
    unsigned symbol_size, module_size, entry_size, i, record_size;
    unsigned char symbol_length, module_length, entry_length, checksum;

    ordinal = command->Ordinal;
    symbol = command->Symbol;
    module = command->Module;
    entry = command->Entry;
    for( symbol_size = 0; symbol[symbol_size] != 0; ++symbol_size ) {}
    for( module_size = 0; module[module_size] != 0; ++module_size ) {}
    symbol_length = symbol_size;
    module_length = module_size;
    entry_size = 0;
    if( entry != NULL ) {
        for( entry_size = 0; entry[entry_size] != 0; ++entry_size ) {}
    }
    entry_length = entry_size;
    file = OutputLibraryName->File;
    CurrentMemberOffset = BufferedPosition( file );
    if( AddMemberSymbol( symbol, symbol_length, 1 ) ) return;
    if( AddMemberSymbol( symbol, symbol_length, 0 ) ) return;

    WriteBufferedByte( file, 0x80 );
    record_size = symbol_length + 2;
    WriteBufferedByte( file, record_size );
    WriteBufferedByte( file, record_size >> 8 );
    checksum = 0x80 + record_size + (record_size >> 8);
    WriteBufferedByte( file, symbol_length );
    checksum += symbol_length;
    WriteBuffered( file, symbol, symbol_size );
    for( i = 0; i < symbol_size; ++i ) checksum += symbol[i];
    WriteBufferedByte( file, -checksum );

    WriteBufferedByte( file, 0x88 );
    record_size = symbol_length + module_length + entry_length + 8;
    if( ordinal != 0 ) ++record_size;
    WriteBufferedByte( file, record_size );
    WriteBufferedByte( file, record_size >> 8 );
    checksum = 0x88 + record_size + (record_size >> 8);
    WriteBufferedByte( file, 0x80 );
    WriteBufferedByte( file, 0xa0 );
    WriteBufferedByte( file, 1 );
    checksum += 0x21;
    WriteBufferedByte( file, ordinal != 0 );
    if( ordinal != 0 ) ++checksum;
    WriteBufferedByte( file, symbol_length );
    checksum += symbol_length;
    WriteBuffered( file, symbol, symbol_size );
    for( i = 0; i < symbol_size; ++i ) checksum += symbol[i];
    WriteBufferedByte( file, module_length );
    checksum += module_length;
    WriteBuffered( file, module, module_size );
    for( i = 0; i < module_size; ++i ) checksum += module[i];
    if( ordinal != 0 ) {
        WriteBufferedByte( file, ordinal );
        WriteBufferedByte( file, ordinal >> 8 );
        checksum += ordinal + (ordinal >> 8);
    } else {
        WriteBufferedByte( file, entry_length );
        checksum += entry_length;
        if( entry_length != 0 ) {
            WriteBuffered( file, entry, entry_size );
            for( i = 0; i < entry_size; ++i ) checksum += entry[i];
        }
    }
    WriteBufferedByte( file, -checksum );
    FreeMemory( symbol );
    FreeMemory( module );
    if( entry != NULL ) FreeMemory( entry );
    WriteBufferedByte( file, 0x8a );
    WriteBufferedByte( file, 2 );
    WriteBufferedByte( file, 0 );
    WriteBufferedByte( file, 0 );
    WriteBufferedByte( file, 0x74 );
    PadLibraryPage( LibraryPageSize );
    CurrentMemberOffset = BufferedPosition( file );
    CheckLibrarySize();
}

/* Original0x413ef7..0x413f37, normal HashSymbol epilogue413ec3,
   and the shared InsertSymbol error tail413e6a. Directory page indices
   must fit16 bits; failure deliberately transfers to AbortCommand. */
void CheckLibrarySize( void )
{
    if( CurrentMemberOffset / LibraryPageSize > 0xffffUL ) {
        utoa( LibraryPageSize, FileErrorName, 10 );
        ReportError( 2, 20, FileErrorName );
        ToolFlags |= 0x40;
        AbortCommand();
    }
}

unsigned long InputModuleOffset;    /* Original0x4302bc. */
unsigned char CurrentRecordType;    /* Original0x43031c. */

/* Original0x413a3b..0x413b26 transfers modules until LIBEND/EOF.
   Each module must begin with THEADR; record decoding owns the current
   record state and ProcessOneModule consumes its remaining records. */
void ProcessModuleRecords( MemberCommand *remove, FileName *extract,
                           unsigned from_library )
{
    InputModuleOffset = BufferedPosition( CurrentName->File );
    if( !(ToolFlags & 2) ) {
        CurrentMemberOffset = BufferedPosition( OutputLibraryName->File );
        CheckLibrarySize();
    } else {
        CurrentMemberOffset = InputModuleOffset;
    }
    ReadOmfRecord( 0 );
    while( CurrentRecordType != 0xf1 && CurrentRecordType != 0xff ) {
        if( CurrentRecordType != 0x80 ) {
            BuildFileName( CurrentName );
            ReportError( 2, 18, FileErrorName );
            ToolFlags |= 0x40;
            CurrentRecordType = 0xff;
            return;
        }
        ProcessOneModule( remove, extract, from_library );
        InputModuleOffset = NextLibraryPage();
        if( !(ToolFlags & 2) ) {
            PadLibraryPage( LibraryPageSize );
            CurrentMemberOffset = BufferedPosition( OutputLibraryName->File );
            CheckLibrarySize();
        } else {
            CurrentMemberOffset = InputModuleOffset;
        }
        ReadOmfRecord( 0 );
    }
}

unsigned OmfRecordLength;         /* Original0x4302c8. */
unsigned char *OmfRecordData;     /* Original0x4301c0. */
unsigned char *OmfRecordCursor;   /* Original0x4301a8. */
unsigned char EasyOMF;            /* Original0x43031d. */

/* Original0x4118a3..0x41195a plus the shared7-byte suffix41173f.
   Len includes the checksum byte. Easy OMF-386 is recognized from its
   literal COMENT signature, as in the released symomf format reader. */
void ReadOmfRecord( unsigned strict )
{
    OpenFile *file;
    int (*reader)( OpenFile * );
    unsigned i;
    const unsigned char *signature;

    file = CurrentName->File;
    reader = strict ? ReadBufferedByteStrict : ReadBufferedByte;
    CurrentRecordType = (*reader)( file );
    if( CurrentRecordType == 0xff ) return;
    OmfRecordLength = ReadBufferedByteStrict( file );
    OmfRecordLength += (unsigned)ReadBufferedByteStrict( file ) << 8;
    OmfRecordData = ReadBuffered( file, OmfRecordLength );
    OmfRecordCursor = OmfRecordData;
    if( (CurrentRecordType & 0xfe) == 0x88 && OmfRecordLength == 8 ) {
        signature = (const unsigned char *)"\x80\xaa" "80386";
        for( i = 0; i < 7 && OmfRecordData[i] == signature[i]; ++i ) {}
        if( i == 7 ) EasyOMF = 1;
    }
}

int SkipCurrentModule;            /* Original0x4302c0. */
int ExtractCurrentModule;         /* Original0x4302c4. */

/* Original0x413b26..0x413c78 resolves the module's true name, applies
   pending removal/extraction commands, and dispatches its OMF body. */
void ProcessOneModule( MemberCommand *remove, FileName *extract,
                       unsigned from_library )
{
    SymbolRecord *initial, *module;
    FileName temporary;
    unsigned owns_name, skip;
    unsigned char comment_class;

    initial = ReadRecordName( 1 );
    module = initial;
    ReadOmfRecord( 1 );
    if( CurrentRecordType == 0x88 ) {
        ++OmfRecordCursor;
        comment_class = *OmfRecordCursor++;
        if( comment_class == 0xa3 ) {
            FreeMemory( initial );
            module = ReadRecordName( 1 );
        } else if( !__is_mangled( initial->Name, initial->NameLength ) ) {
            NormalizeModuleName( initial );
        }
    } else {
        NormalizeModuleName( initial );
    }
    SeekBuffered( CurrentName->File, InputModuleOffset );
    if( from_library == 0 ) {
        owns_name = MatchRemoveCommand( CurrentName->Name,
                         (signed char)CurrentName->NameLength, remove );
        if( SkipCurrentModule ) {
            temporary.Name = module->Name;
            temporary.NameLength = module->NameLength;
            AddRemoveCommand( &temporary, owns_name );
        }
        SkipCurrentModule = 0;
    } else {
        MatchRemoveCommand( module->Name, module->NameLength, remove );
    }
    MatchExtractCommand( module->Name, module->NameLength, extract );
    if( !SkipCurrentModule ) {
        if( InsertSymbol( module, 1 ) ) SkipCurrentModule = 1;
        else module->Value.Page = CurrentMemberOffset / LibraryPageSize;
    }
    skip = SkipCurrentModule;
    if( ToolFlags & 0x80 ) SkipCurrentModule = 1;
    ProcessOmfModule( module );
    if( skip != 0 ) FreeMemory( module );
}

/* Original0x411ee1..0x411f2f and the shared five-register return
   at411740. OMF names are length-prefixed; the extra byte belongs to
   the caller's module-specific extension. */
SymbolRecord *ReadRecordName( unsigned extra )
{
    SymbolRecord *record;
    unsigned length, i;

    length = *OmfRecordCursor++;
    record = AllocMemory( length + 11 + extra );
    record->Flags = (record->Flags & 0x80) | 2;
    record->NameLength = length;
    for( i = 0; i < length; ++i ) record->Name[i] = OmfRecordCursor[i];
    OmfRecordCursor += length;
    record->Name[length] = '\0';
    return( record );
}

unsigned short SegDefCount;          /* Original0x430308. */
unsigned short CurrSegRef;           /* Original0x420360. */
unsigned char Rec32Bit;              /* Original0x43000c. */
struct OmfName *LName_Head;           /* Original0x4301b0. */
struct OmfName **LName_Owner;         /* Original0x4301ac. */

/* Original0x41155c..0x411746. OMF records supply symbols and, when
   requested, bytes for the new library and an extracted object. */
void ProcessOmfModule( SymbolRecord *module )
{
    OpenFile *input;
    unsigned trim_name, copy, symbols;

    input = CurrentName->File;
    if( ExtractCurrentModule ) {
        CurrentExtractName->Flags = 4;
        if( !OpenLibraryFile( CurrentExtractName, 0x0262 ) ) {
            BuildFileName( CurrentExtractName );
            ReportError( 2, 51, FileErrorName );
            ToolFlags |= 0x40;
            ExtractCurrentModule = 0;
        }
    }
    trim_name = ToolFlags & 0x2000;
    copy = ExtractCurrentModule || (!SkipCurrentModule && !(ToolFlags & 2));
    symbols = !SkipCurrentModule || ((ToolFlags & 0x80) && (ToolFlags & 1));
    if( !copy ) BufferedPosition( input );
    EasyOMF = 0;
    SegDefCount = 0;
    CurrSegRef = 0;
    LName_Head = NULL;
    LName_Owner = &LName_Head;
    do {
        ReadOmfRecord( 1 );
        Rec32Bit = (CurrentRecordType & 1) ? 1 : EasyOMF;
        if( symbols ) {
            switch( CurrentRecordType ) {
            case 0x98: case 0x99: procsegdef(); break;
            case 0x90: case 0x91: getpubdef(); break;
            case 0x88: GetComent(); break;
            case 0xb0: getcomdef(); break;
            case 0xc2: case 0xc3: getcomdat(); break;
            case 0x96: getlname( 0 ); break;
            case 0xca: getlname( 1 ); break;
            }
        }
        if( copy ) CopyOmfRecord( module, trim_name );
        if( CurrentRecordType == 0x80 ) trim_name = 0;
    } while( CurrentRecordType != 0x8a && CurrentRecordType != 0x8b );
    FreeCommonBlk();
    FreeLNames();
    if( ExtractCurrentModule ) {
        CloseLibraryFile( CurrentExtractName );
        CurrentExtractName->NameLength = 0xff;
    }
}

/* Original0x411ca0..0x411ee1. Retained records are copied verbatim,
   except a trimmed THEADR gets a freshly computed OMF checksum. */
void CopyOmfRecord( SymbolRecord *module, unsigned trim_name )
{
    OpenFile *output, *extracted;
    unsigned i;
    unsigned char sum;

    OmfRecordCursor = OmfRecordData;
    if( (ToolFlags & 0x1000) &&
        ((CurrentRecordType >= 0x94 && CurrentRecordType <= 0x95) ||
         (CurrentRecordType >= 0xc4 && CurrentRecordType <= 0xc5)) ) return;
    if( (ToolFlags & 2) && !ExtractCurrentModule ) return;
    if( CurrentRecordType == 0x80 && trim_name ) {
        OmfRecordLength = module->NameLength + 2;
    }
    output = extracted = NULL;
    if( !SkipCurrentModule ) {
        output = OutputLibraryName->File;
        WriteBufferedByte( output, CurrentRecordType );
        WriteBufferedByte( output, OmfRecordLength & 0xff );
        WriteBufferedByte( output, (OmfRecordLength >> 8) & 0xff );
    }
    if( ExtractCurrentModule ) {
        extracted = CurrentExtractName->File;
        WriteBufferedByte( extracted, CurrentRecordType );
        WriteBufferedByte( extracted, OmfRecordLength & 0xff );
        WriteBufferedByte( extracted, (OmfRecordLength >> 8) & 0xff );
    }
    if( CurrentRecordType == 0x80 && trim_name ) {
        sum = CurrentRecordType + (OmfRecordLength & 0xff) +
                                  (OmfRecordLength >> 8);
        if( !SkipCurrentModule ) WriteBufferedByte( output, module->NameLength );
        if( ExtractCurrentModule ) WriteBufferedByte( extracted, module->NameLength );
        sum += module->NameLength;
        for( i = 0; i < module->NameLength; ++i ) {
            sum += (unsigned char)module->Name[i];
            if( !SkipCurrentModule ) WriteBufferedByte( output, (unsigned char)module->Name[i] );
            if( ExtractCurrentModule ) WriteBufferedByte( extracted, (unsigned char)module->Name[i] );
        }
        if( !SkipCurrentModule ) WriteBufferedByte( output, (unsigned char)-sum );
        if( ExtractCurrentModule ) WriteBufferedByte( extracted, (unsigned char)-sum );
    } else {
        if( !SkipCurrentModule ) WriteBuffered( output, OmfRecordData, OmfRecordLength );
        if( ExtractCurrentModule ) WriteBuffered( extracted, OmfRecordData, OmfRecordLength );
    }
}

/* Original0x411aa3..0x411aea. Import-definition comments contribute
   their public import name; the DLL and ordinal metadata need no symbol. */
void GetComent( void )
{
    SymbolRecord *symbol;

    ++OmfRecordCursor;
    if( *OmfRecordCursor++ == 0xa0 && *OmfRecordCursor++ == 1 ) {
        ++OmfRecordCursor;
        symbol = ReadRecordName( 0 );
        symbol->Value.Owner = CurrentModuleSymbol;
        InsertSymbol( symbol, 0 );
    }
}

#pragma pack(1)
struct OmfName {
    struct OmfName *Next;
    unsigned char Local;
    unsigned char Length;
    char Name[1];
};
#pragma pack()

/* Original0x411c12..0x411c7e plus the shared frame return at41173f.
   LNAMES storage is counted and intentionally has no trailing NUL. */
void getlname( unsigned local )
{
    struct OmfName *name;
    unsigned length, i;

    while( (unsigned)(OmfRecordCursor - OmfRecordData) < OmfRecordLength - 1 ) {
        length = *OmfRecordCursor++;
        name = AllocMemory( length + 6 );
        name->Local = local;
        name->Length = length;
        for( i = 0; i < length; ++i ) name->Name[i] = OmfRecordCursor[i];
        name->Next = NULL;
        *LName_Owner = name;
        LName_Owner = &name->Next;
        OmfRecordCursor += length;
    }
}

/* Original0x411a12..0x411a5d. COMMON segments are remembered by index
   for the public-symbol binding used by FORTRAN common blocks. */
void procsegdef( void )
{
    CommonBlock **owner, *block;

    ++SegDefCount;
    if( ((*OmfRecordCursor & 0x1f) >> 2) == 6 ) {
        owner = &CurrCommonBlk;
        while( *owner != NULL ) owner = &(*owner)->Next;
        block = AllocMemory( 6 );
        block->Segment = SegDefCount;
        block->Next = NULL;
        *owner = block;
    }
}

/* Original0x411a5d..0x411aa3 plus the two-byte shared return at4119f2.
   Public records carry group/segment, followed by name/offset/type tuples. */
void getpubdef( void )
{
    SymbolRecord *symbol;

    GetIdx();
    while( (unsigned)(OmfRecordCursor - OmfRecordData) < OmfRecordLength - 1 ) {
        symbol = ReadRecordName( 0 );
        symbol->Value.Owner = CurrentModuleSymbol;
        GetOffset();
        GetIndex();
        InsertSymbol( symbol, 0 );
    }
}

/* Original0x4119a2..0x4119cc. A zero group and segment are followed
   by an absolute frame; all other addresses use their segment index. */
void GetIdx( void )
{
    unsigned short group;

    group = GetIndex();
    CurrSegRef = GetIndex();
    if( group == 0 && CurrSegRef == 0 ) OmfRecordCursor += 2;
}

/* Original0x41195a..0x41198a. An OMF index is either one byte or
   a high-bit marker plus fifteen bits spread over two bytes. */
unsigned short GetIndex( void )
{
    unsigned short index;

    index = *OmfRecordCursor++;
    if( index > 0x7f ) index = ((index & 0x7f) << 8) | *OmfRecordCursor++;
    return( index );
}

/* Original0x41198a..0x4119a2. The record width also applies to offsets. */
void GetOffset( void )
{
    OmfRecordCursor += 2;
    if( Rec32Bit ) OmfRecordCursor += 2;
}

/* Original0x411c7e..0x411ca0 and shared POPEDX/RET at411ae8. */
void FreeLNames( void )
{
    struct OmfName *next;

    while( LName_Head != NULL ) {
        next = LName_Head->Next;
        FreeMemory( LName_Head );
        LName_Head = next;
    }
}

/* Original0x4117e9..0x41182d and the shared return at411740. */
unsigned long NextLibraryPage( void )
{
    OpenFile *file;
    unsigned long position;
    unsigned padding;

    file = CurrentName->File;
    position = BufferedPosition( file );
    padding = InputLibraryPageSize - position % InputLibraryPageSize;
    if( padding != InputLibraryPageSize ) {
        position += padding;
        ReadBuffered( file, padding );
    }
    return( position );
}

/* Original0x413547..0x4135a2. Additions are already written; traverse
   the existing library with its removal and extraction command lists. */
void CopyLibraryMembers( void )
{
    if( ToolFlags & 0x200 ) {
        CloseLibraryFile( LibraryName );
    } else {
        CurrentName = LibraryName;
        if( ReadLibraryHeader() == 0 ) {
            SeekBuffered( CurrentName->File, InputLibraryPageSize );
            ProcessModuleRecords( (MemberCommand *)MemberWork,
                                  (FileName *)ExtractWork, 1 );
            CloseLibraryFile( CurrentName );
        }
    }
}

/* Original0x4119cc..0x4119f4. Common-segment indices are recorded in
   input order; once the requested index is passed it cannot be present. */
int IsCommonRef( void )
{
    CommonBlock *block;

    for( block = CurrCommonBlk; block != NULL; block = block->Next ) {
        if( block->Segment > CurrSegRef ) break;
        if( block->Segment == CurrSegRef ) return( 1 );
    }
    return( 0 );
}

/* Original0x413cf3..0x413d52. LIBEND consumes the remainder of the
   next512-byte boundary, ready for the block-based symbol dictionary. */
void PrepareOutput( void )
{
    OpenFile *file;
    unsigned long position, padding;

    file = OutputLibraryName->File;
    position = BufferedPosition( file );
    padding = ((position >> 9) + 1) * 512 - position - 3;
    WriteBufferedByte( file, 0xf1 );
    WriteBufferedByte( file, padding & 0xff );
    WriteBufferedByte( file, (padding >> 8) & 0xff );
    PadLibraryPage( 512 );
}

#pragma pack(1)
typedef struct LibraryHeader {
    unsigned char Type;
    unsigned short PageLength;
    unsigned long Dictionary;
    unsigned short Blocks;
    unsigned char Flags;
} LibraryHeader;
#pragma pack()
typedef char WlibLibraryHeaderSize[(sizeof(LibraryHeader) == 10) ? 1 : -1];

/* Original0x413c78..0x413cf3. Flush the remaining output then replace
   the provisional header with the completed dictionary's location and size. */
void FinishOutput( void )
{
    OpenFile *file;
    LibraryHeader header;

    file = OutputLibraryName->File;
    header.Type = 0xf0;
    header.PageLength = LibraryPageSize - 3;
    header.Dictionary = DictionaryOffset;
    header.Blocks = DictionaryBlockCount;
    header.Flags = NamesEqual == NamesEqualCaseSensitive;
    write( file->Handle, file->Buffer, file->Current - file->Buffer );
    file->Current = file->Buffer;
    lseek( file->Handle, 0, 0 );
    write( file->Handle, &header, sizeof(header) );
}

/* Original0x411b77..0x411c12. Public COMDAT names are taken from
   LNAMES; continuation and local records contribute no new public symbol. */
void getcomdat( void )
{
    unsigned allocation, index, i;
    struct OmfName *name;
    SymbolRecord *symbol;

    if( *OmfRecordCursor & 5 ) return;
    ++OmfRecordCursor;
    allocation = *OmfRecordCursor++ & 15;
    ++OmfRecordCursor;
    GetOffset();
    GetIndex();
    if( allocation == 0 ) GetIdx();
    index = GetIndex() - 1;
    name = LName_Head;
    while( index != 0 ) {
        name = name->Next;
        --index;
    }
    if( name->Local ) return;
    symbol = AllocMemory( name->Length + 11 );
    symbol->Flags &= 0x80;
    symbol->Flags |= 1;
    symbol->NameLength = name->Length;
    for( i = 0; i < name->Length; ++i ) symbol->Name[i] = name->Name[i];
    symbol->Name[name->Length] = 0;
    symbol->Value.Owner = CurrentModuleSymbol;
    InsertSymbol( symbol, 0 );
}
