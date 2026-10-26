/* Executable export conversion in the 1994 librarian. These application
   routines precede the released nwlib import-file architecture. */
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include "imports.h"
#include <recon.h>

int ImportInputHandle;                 /* Original0x430330. */
unsigned char *ImportReadBuffer;        /* Original0x430328,4096 bytes. */
unsigned char *ImportRecordBuffer;      /* Original0x430324,790 bytes. */
char *ImportModuleName;                 /* Original0x43032c. */
unsigned ImportModuleNameLength;        /* Original0x430320. */
char ImportTemporaryName[13];           /* Original0x430334,L_tmpnam. */

/* Original0x41466a..0x414a05. Convert NLM/NE/LE/LX/PE exports into a
   temporary OMF stream, then reopen that stream for the normal member reader.
   The PE section search deliberately requires an exact export RVA match. */
int OpenFileCache( int input )
{
    union {
        unsigned short signature;
        unsigned long pe_signature;
        nlm_header nlm;
        os2_exe_header ne;
        os2_flat_header flat;
        pe_header pe;
    } header;
    unsigned long header_offset;
    pe_object *objects, *object;
    unsigned i;
    int output;

    ImportModuleName = NULL;
    ImportModuleNameLength = 0;
    ImportReadBuffer = AllocMemory( 4096 );
    ImportRecordBuffer = AllocMemory( 790 );
    ImportTemporaryName[0] = 0;
    ImportInputHandle = input;
    /* Original failure leaves these allocations for process cleanup. */
    if( tmpnam( ImportTemporaryName ) == NULL ) return( -1 );
    output = open( ImportTemporaryName, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666 );
    if( output < 0 ) return( -1 );
    if( !ReadImportData( &header, 2 ) ) goto failure;
    if( header.signature != 0x5a4d ) {
        if( !SeekImportInput( 0 ) ) goto failure;
        if( !ReadImportData( &header, 24 ) ) goto failure;
        for( i = 0; i < 24; ++i ) {
            if( header.nlm.signature[i] != "NetWare Loadable Module\x1a"[i] ) goto failure;
        }
        if( !SeekImportInput( 0 ) ) goto failure;
        if( !ReadImportData( &header.nlm, sizeof(nlm_header) ) ) goto failure;
        if( !SeekImportInput( 28 ) ) goto failure;
        if( !ReadImportModuleName( 1 ) ) goto failure;
        if( !SeekImportInput( header.nlm.publicsOffset ) ) goto failure;
        if( !ConvertNlmExports( output, header.nlm.numberOfPublics ) ) goto failure;
    } else {
        if( !SeekImportInput( 0x3c ) ) goto failure;
        if( !ReadImportData( &header_offset, 4 ) ) goto failure;
        if( !SeekImportInput( header_offset ) ) goto failure;
        if( !ReadImportData( &header, 4 ) ) goto failure;
        if( header.signature == 0x454e ) {
            if( !ReadImportData( (char *)&header + 4, sizeof(os2_exe_header) - 4 ) ) goto failure;
            if( !SeekImportInput( header_offset + header.ne.resident_off ) ) goto failure;
            if( !ReadImportModuleName( 0 ) ) goto failure;
            if( !ConvertNameTable( output, FirstCommand ) ) goto failure;
            if( header.ne.nonres_off != 0 ) {
                if( !SeekImportInput( header.ne.nonres_off ) ) goto failure;
                if( !ConvertNameTable( output, InCommand ) ) goto failure;
            }
        } else if( header.signature == 0x454c || header.signature == 0x584c ) {
            if( !ReadImportData( (char *)&header + 4, sizeof(os2_flat_header) - 4 ) ) goto failure;
            if( !SeekImportInput( header_offset + header.flat.resname_off ) ) goto failure;
            if( !ReadImportModuleName( 0 ) ) goto failure;
            if( !ConvertNameTable( output, FirstCommand ) ) goto failure;
            if( header.flat.nonres_off != 0 ) {
                if( !SeekImportInput( header.flat.nonres_off ) ) goto failure;
                if( !ConvertNameTable( output, InCommand ) ) goto failure;
            }
        } else if( header.pe_signature == 0x4550 ) {
            if( !ReadImportData( (char *)&header + 4, sizeof(pe_header) - 4 ) ) goto failure;
            objects = AllocMemory( header.pe.num_objects * sizeof(pe_object) );
            if( !ReadImportData( objects, header.pe.num_objects * sizeof(pe_object) ) ) {
                CloseFileCache( output );
                FreeMemory( objects );
                return( -1 );
            }
            object = objects;
            while( header.pe.num_objects != 0 ) {
                if( object->rva == header.pe.table[0].rva ) {
                    if( !SeekImportInput( object->physical_offset ) ) {
                        CloseFileCache( output );
                        FreeMemory( objects );
                        return( -1 );
                    }
                    header.pe.table[0].rva -= object->physical_offset;
                    if( !ConvertPeExports( output, &header.pe.table[0].rva, FirstCommand ) ) {
                        CloseFileCache( output );
                        FreeMemory( objects );
                        return( -1 );
                    }
                    break;
                }
                --header.pe.num_objects;
                ++object;
            }
            FreeMemory( objects );
            if( header.pe.num_objects == 0 ) goto failure;
        } else {
            goto failure;
        }
    }
    if( close( output ) != 0 ) goto failure;
    CloseImportInput();
    return( open( ImportTemporaryName, O_BINARY, 0666 ) );
failure:
    CloseFileCache( output );
    return( -1 );
}

/* Original0x4140e8..0x41410c. Executable header reads require the
   complete requested count; a short read is a conversion failure. */
int ReadImportData( void *buffer, unsigned length )
{
    CheckBreak();
    return( read( ImportInputHandle, buffer, length ) == length );
}

/* Original0x41410c..0x414128 seeks the original executable stream. */
int SeekImportInput( unsigned long offset )
{
    return( lseek( ImportInputHandle, offset, 0 ) != -1 );
}

/* Original0x41454f..0x41466a. The old converter groups adjacent name
   pointers by their distance, retaining its actual start/end selection. */
int ConvertPeExports( int handle, unsigned long *offset, unsigned char by_name )
{
    pe_export_directory directory;
    unsigned long bytes, *names, *current, position, name;
    unsigned count;
    int result;

    if( !ReadImportData( &directory, sizeof(directory) ) ) return( 0 );
    if( !ReadImportNameAtOffset( directory.name_rva - offset[0] ) ) return( 0 );
    bytes = directory.num_name_ptrs * 4;
    if( bytes == 0 || bytes > 0xfe00UL ) return( 0 );
    names = AllocMemory( bytes + 4 );
    if( !SeekImportInput( directory.name_ptr_table_rva - offset[0] ) ||
        !ReadImportData( names, bytes ) ) {
        FreeMemory( names );
        return( 0 );
    }
    count = 1;
    position = directory.ordinal_table_rva - offset[0];
    current = names;
    *current -= offset[0];
    name = *current++;
    bytes -= 4;
    while( bytes != 0 ) {
        *current -= offset[0];
        if( *current - name > 4096 ) {
            if( !ConvertPeNameBlock( name, current[-1], position, count,
                                    directory.ordinal_base, handle, by_name ) ) {
                FreeMemory( names );
                return( 0 );
            }
            position += count * 2;
            count = 1;
            name = current[-1];
        }
        ++count;
        ++current;
        bytes -= 4;
    }
    result = ConvertPeNameBlock( name, names[0] + offset[1], position, count,
                                directory.ordinal_base, handle, by_name );
    FreeMemory( names );
    return( result );
}

/* Original0x4141c9..0x41421f. Keep the DLL name as counted bytes;
   its allocated copy intentionally has no trailing NUL. */
int ReadImportNameAtOffset( unsigned long offset )
{
    unsigned char *p;
    unsigned i;

    if( !SeekImportInput( offset ) ) return( 0 );
    p = ImportReadBuffer;
    for( ;; ) {
        *p = ReadImportByte();
        if( *p == 0 ) break;
        ++p;
    }
    ImportModuleNameLength = p - ImportReadBuffer;
    ImportModuleName = AllocMemory( ImportModuleNameLength );
    for( i = 0; i < ImportModuleNameLength; ++i ) {
        ImportModuleName[i] = ImportReadBuffer[i];
    }
    return( 1 );
}

/* Original0x414128..0x414149 returns0xff after an incomplete byte. */
unsigned char ReadImportByte( void )
{
    unsigned char ch;

    if( !ReadImportData( &ch, 1 ) ) return( 0xff );
    return( ch );
}

/* Original0x41448c..0x41454f. Ordinal indices and the associated name
   block are temporary; the count governs the contiguous NUL strings. */
int ConvertPeNameBlock( unsigned long start, unsigned long end, unsigned long ordinals,
                       unsigned count, unsigned ordinal_base, int handle, unsigned char by_name )
{
    unsigned short *indices, *index;
    char *names, *name;
    unsigned length;

    indices = AllocMemory( count * 2 );
    if( !SeekImportInput( ordinals ) || !ReadImportData( indices, count * 2 ) ||
        !SeekImportInput( start ) ) {
        FreeMemory( indices );
        return( 0 );
    }
    names = AllocMemory( end - start );
    if( !ReadImportData( names, end - start ) ) {
        /* Original failure also leaves the name allocation unreleased. */
        FreeMemory( indices );
        return( 0 );
    }
    index = indices;
    name = names;
    while( count != 0 ) {
        for( length = 0; name[length] != 0; ++length ) {}
        WriteImportSymbol( name, length, (short)(*index + ordinal_base), handle, by_name );
        name += length + 1;
        ++index;
        --count;
    }
    FreeMemory( names );
    FreeMemory( indices );
    return( 1 );
}

/* Original0x41421f..0x4143d2. Build THEADR/IMPDEF/MODEND records in
   the encountered790-byte scratch buffer, including each OMF checksum. */
void WriteImportSymbol( const char *name, unsigned length, int ordinal, int handle, unsigned char by_name )
{
    unsigned char *p;
    unsigned record_length, checksum, i, total;

    if( (short)ordinal == 0 ) return;
    total = length * 2 + 22 + (unsigned short)ImportModuleNameLength;
    p = ImportRecordBuffer;
    record_length = length + 2;
    *p++ = 0x80;
    *p++ = record_length;
    *p++ = record_length >> 8;
    checksum = 0x80 + (record_length & 0xff) + (record_length >> 8);
    *p++ = length;
    checksum += length;
    for( i = 0; i < length; ++i ) {
        *p++ = name[i];
        checksum += (unsigned char)name[i];
    }
    *p++ = -checksum;

    record_length = length + ImportModuleNameLength + 9;
    *p++ = 0x88;
    *p++ = record_length;
    *p++ = record_length >> 8;
    checksum = 0x88 + (record_length & 0xff) + (record_length >> 8);
    *p++ = 0x80; *p++ = 0xa0; *p++ = 1;
    checksum += 0x80 + 0xa0 + 1;
    *p++ = !by_name;
    checksum += !by_name;
    *p++ = length;
    checksum += length;
    for( i = 0; i < length; ++i ) {
        *p++ = name[i];
        checksum += (unsigned char)name[i];
    }
    *p++ = ImportModuleNameLength;
    checksum += ImportModuleNameLength;
    for( i = 0; i < ImportModuleNameLength; ++i ) {
        *p++ = ImportModuleName[i];
        checksum += (unsigned char)ImportModuleName[i];
    }
    if( !by_name ) {
        *p++ = ordinal;
        *p++ = (short)ordinal >> 8;
        checksum += (ordinal & 0xff) + ((short)ordinal >> 8);
    } else {
        *p++ = 0; *p++ = 0;
    }
    *p++ = -checksum;
    *p++ = 0x8a; *p++ = 2; *p++ = 0; *p++ = 0; *p = 0x74;
    write( handle, ImportRecordBuffer, (short)total );
}

/* Original0x414149..0x41416c. Closing is idempotent for the current
   original-input slot; the return always reports successful teardown. */
int CloseImportInput( void )
{
    if( ImportInputHandle != -1 ) {
        close( ImportInputHandle );
        ImportInputHandle = -1;
    }
    return( 1 );
}

/* Original0x414a05..0x414a55. The generated stream is a private cache;
   remove it and release the conversion buffers after closing its reader. */
void CloseFileCache( int handle )
{
    CloseImportInput();
    if( ImportTemporaryName[0] != 0 ) {
        if( handle != -1 ) close( handle );
        unlink( ImportTemporaryName );
    }
    FreeMemory( ImportRecordBuffer );
    FreeMemory( ImportReadBuffer );
    if( ImportModuleName != NULL ) FreeMemory( ImportModuleName );
}
