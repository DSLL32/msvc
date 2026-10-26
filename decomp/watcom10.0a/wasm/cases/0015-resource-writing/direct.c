/* Authored typed resource expectations, licensed App code is separate.
 * Actual32-bit-name reader keeps each UTF16 code unit low byte only. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <unistd.h>
#include <recon-tool.h>
#include "wasm.h"
#include <recon.h>
#define CHECK(x) do { if( !(x) ) return( 0 ); } while(0)
int conversion_smoke(void)
{
    char buffer[6];
    CHECK(ConvToUnicode == DefaultConversion);
    CHECK(DefaultConversion(3,"Ab!",buffer) == 6);
    CHECK(memcmp(buffer,"A\0b\0!\0",6) == 0);
    return 1;
}
static int scalar_roundtrip_unit( void )
{
    unsigned char small = 0xa5, a;
    unsigned short medium = 0x1234, b;
    unsigned long large = 0x89abcdefUL, c;
    int file;
    file = open( "scalars.bin", O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0600 ); CHECK( file >= 0 );
    CHECK( ResWriteUint8( &small, file ) == 0 );
    CHECK( ResWriteUint16( &medium, file ) == 0 );
    CHECK( ResWriteUint32( &large, file ) == 0 );
    CHECK( tell( file ) == 7 && lseek( file, 0, SEEK_SET ) == 0 );
    { unsigned char raw[7];
      CHECK(read(file,raw,7) == 7 && memcmp(raw,"\xa5\x34\x12\xef\xcd\xab\x89",7) == 0);
      CHECK(lseek(file,0,SEEK_SET) == 0); }
    CHECK(small == 0xa5 && medium == 0x1234 && large == 0x89abcdefUL);
    CHECK( ResReadUint8( &a, file ) == 0 && a == small );
    CHECK( ResReadUint16( &b, file ) == 0 && b == medium );
    CHECK( ResReadUint32( &c, file ) == 0 && c == large );
    CHECK( close( file ) == 0 && unlink( "scalars.bin" ) == 0 );
    return( 1 );
}
static int names_and_strings( void )
{
    ResNameOrOrdinal ordinal;
    ResNameOrOrdinal *name;
    char longtext[301], *readback;
    char converted[6] = {1,1,1,1,1,1};
    int file, length;
    unsigned short units[]={0x0141,0x02e9,0};
    unsigned i;
    ordinal.ord.fFlag = 255; ordinal.ord.wOrdinalID = 65530;
    CHECK( DefaultConversion( 3, "Ab!", converted ) == 6 );
    CHECK( memcmp( converted, "A\0b\0!\0", 6 ) == 0 );
    CHECK( DefaultConversion( 3, NULL, NULL ) == 6 );
    file = open( "names.bin", O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0600 ); CHECK( file >= 0 );
    CHECK( ResWriteNameOrOrdinal( (ResNameOrOrdinal *)&ordinal, 1, file ) == 0 );
    CHECK( ResWriteNameOrOrdinal( (ResNameOrOrdinal *)"MixedName", 1, file ) == 0 );
    CHECK( ResWriteNameOrOrdinal( NULL, 1, file ) == 0 );
    for( i = 0; i < 300; ++i ) longtext[i] = 'a' + i % 26;
    longtext[300] = 0;
    CHECK( ResWriteString( longtext, 1, file ) == 0 );
    CHECK(ResWriteUint16(&units[0],file) == 0 && ResWriteUint16(&units[1],file) == 0 && ResWriteUint16(&units[2],file) == 0);
    CHECK( lseek( file, 0, SEEK_SET ) == 0 );
    name = ResRead32NameOrOrdinal( file ); CHECK( name != NULL );
    CHECK( name->ord.fFlag == 255 && name->ord.wOrdinalID == 65530 ); free( name );
    name = ResRead32NameOrOrdinal( file ); CHECK( name != NULL );
    CHECK( strcmp( name->name, "MixedName" ) == 0 ); free( name );
    name = ResRead32NameOrOrdinal( file ); CHECK( name != NULL );
    CHECK( name->name[0] == 0 ); free( name );
    readback = ResRead32String( file, &length ); CHECK( readback != NULL );
    CHECK( length == 300 && strcmp( readback, longtext ) == 0 ); free( readback );
    CHECK(tell(file) == 628);
    readback=ResRead32String(file,&length);CHECK(readback != NULL && length == 2);
    CHECK(memcmp(readback,"A\xe9",3) == 0);free(readback);
    CHECK(tell(file) == 634 && units[0] == 0x0141 && units[1] == 0x02e9 && units[2] == 0);
    CHECK(longtext[0] == 'a' && longtext[299] == 'n' && longtext[300] == 0);
    CHECK(ConvToUnicode == DefaultConversion);
    CHECK( close( file ) == 0 && unlink( "names.bin" ) == 0 );
    return( 1 );
}
static int wres_directory_roundtrip( void )
{
    struct { WResTypeInfo info; char extra[3]; } type;
    struct { WResResInfo info; char extra[4]; } resource;
    WResHeader header = {{0xc3d4c1d7UL,0xc3d2cdcfUL},32,1,1,2};
    WResExtHeader extra = {1,{0,0,0,0}};
    WResLangInfo lang = {{0x409,0},0x10,28,4};
    WResDir dir;
    WResID *id;
    unsigned char raw[32];
    int file;
    memset( &type, 0, sizeof(type) ); memset( &resource, 0, sizeof(resource) );
    type.info.NumResources = 1; type.info.TypeName.IsName = 1;
    type.info.TypeName.ID.Name.NumChars = 4; memcpy( type.info.TypeName.ID.Name.Name, "TYPE", 4 );
    resource.info.NumResources = 1; resource.info.ResName.IsName = 1;
    resource.info.ResName.ID.Name.NumChars = 5; memcpy( resource.info.ResName.ID.Name.Name, "entry", 5 );
    file = open( "roundtrip.res", O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0600 ); CHECK( file >= 0 );
    CHECK( WResWriteHeaderRecord( &header, file ) == 0 );
    CHECK( WResWriteExtHeader( &extra, file ) == 0 );
    CHECK( write( file, "DATA", 4 ) == 4 );
    CHECK( WResWriteTypeRecord( &type.info, file ) == 0 );
    CHECK( WResWriteResRecord( &resource.info, file ) == 0 );
    CHECK( WResWriteLangRecord( &lang, file ) == 0 );
    CHECK(tell(file) == 62 && lseek(file,0,SEEK_SET) == 0);
    CHECK(read(file,raw,32) == 32);
    CHECK(memcmp(raw,"\xd7\xc1\xd4\xc3\xcf\xcd\xd2\xc3\x20\0\0\0\1\0\1\0\2\0",18) == 0);
    CHECK(memcmp(raw+18,"\1\0\0\0\0\0\0\0\0\0DATA",14) == 0);
    dir = WResInitDir(); CHECK( dir != NULL );
    CHECK( WResReadDir( file, dir, NULL ) == 0 );
    CHECK( dir->NumTypes == 1 && dir->NumResources == 1 );
    CHECK( dir->Head->Head->Head->Info.Offset == 28 && dir->Head->Head->Head->Info.Length == 4 );
    CHECK( memcmp( dir->Head->Info.TypeName.ID.Name.Name, "TYPE", 4 ) == 0 );
    CHECK( memcmp( dir->Head->Head->Info.ResName.ID.Name.Name, "entry", 5 ) == 0 );
    CHECK(tell(file) == 62);
    CHECK(lseek(file,28,SEEK_SET) == 28 && read(file,raw,4) == 4 && memcmp(raw,"DATA",4) == 0);
    CHECK(type.info.NumResources == 1 && memcmp(type.info.TypeName.ID.Name.Name,"TYPE",4) == 0);
    CHECK(resource.info.NumResources == 1 && memcmp(resource.info.ResName.ID.Name.Name,"entry",5) == 0);
    __FreeTypeList(dir);CHECK(dir->Head == NULL && dir->Tail == NULL);
    WResFreeDir( dir );
    CHECK( close( file ) == 0 && unlink( "roundtrip.res" ) == 0 );
    file = open( "id.bin", O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0600 ); CHECK( file >= 0 );
    id = WResIDFromStr( "Abc" ); CHECK( id != NULL );
    CHECK( WResWriteWResID( id, file ) == 0 );
    CHECK( WResWriteWResIDNameUni( &id->ID.Name, 1, file ) == 0 ); free( id );
    id = WResIDFromNum( 0x1234 ); CHECK( id != NULL );
    CHECK( WResWriteWResID( id, file ) == 0 ); free( id );
    CHECK( lseek( file, 0, SEEK_SET ) == 0 );
    CHECK( read( file, raw, sizeof(raw) ) == 16 );
    CHECK( memcmp( raw, "\1\3Abc\3\0A\0b\0c\0\0\64\22", 16 ) == 0 );
    CHECK( close( file ) == 0 && unlink( "id.bin" ) == 0 );
    return( 1 );
}
static int mres_header_roundtrip( void )
{
    MResResourceHeader header, *readback;
    ResNameOrOrdinal ordinal;
    char name_owner[]="named";
    int file;
    ordinal.ord.fFlag = 255; ordinal.ord.wOrdinalID = 6;
    header.Type = &ordinal; header.Name = (ResNameOrOrdinal *)name_owner;
    header.MemoryFlags = 0x30; header.Size = 4;
    file = open( "mres.res", O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0600 ); CHECK( file >= 0 );
    CHECK( MResWriteResourceHeader( &header, file ) == 0 );
    CHECK( write( file, "DATA", 4 ) == 4 );
    CHECK( lseek( file, 0, SEEK_SET ) == 0 );
    { char bytes[15];CHECK(read(file,bytes,15) == 15);
      CHECK(memcmp(bytes,"\xff\6\0named\0\60\0\4\0\0\0",15) == 0); }
    CHECK(lseek(file,0,SEEK_SET) == 0);
    readback = MResReadResourceHeader( file ); CHECK( readback != NULL );
    CHECK( readback->Type->ord.wOrdinalID == 6 && readback->Type->ord.fFlag == 255 );
    CHECK( strcmp( readback->Name->name, "named" ) == 0 );
    CHECK( readback->MemoryFlags == 0x30 && readback->Size == 4 );
    CHECK(tell(file) == 15 && header.Type == &ordinal && header.Name == (ResNameOrOrdinal *)name_owner && memcmp(name_owner,"named",6) == 0);
    CHECK(ordinal.ord.fFlag == 255 && ordinal.ord.wOrdinalID == 6);
    { char bytes[4];CHECK(read(file,bytes,4) == 4 && memcmp(bytes,"DATA",4) == 0); }
    MResFreeResourceHeader( readback );
    CHECK( close( file ) == 0 && unlink( "mres.res" ) == 0 );
    return( 1 );
}
const ReconTest writer_tests[] = {
    { "typed scalar descriptor roundtrip", 1, scalar_roundtrip_unit },
    { "wide names and static/allocated conversion buffers", 1, names_and_strings },
    { "valid named WRES v2 directory and counted IDs", 1, wres_directory_roundtrip },
    { "Win16 MRes header write/read ownership", 1, mres_header_roundtrip }
};
