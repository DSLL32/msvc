/* Buffered file state in the1994 librarian, before the released nwlib
   stream architecture. Counted FileName components remain App objects. */
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "toolstate.h"
#include <recon.h>

/* Original0x412185..0x412249. Flags bit0x200 selects the buffered
   object; the raw descriptor is stored in the same slot for other modes. */
int OpenLibraryFile( FileName *name, unsigned flags )
{
    int handle;
    OpenFile *file;

    handle = -1;
    file = NULL;
    name->Flags = (flags & 0x200) ? 1 : 2;
    if( name->NameLength != 0 ) {
        BuildFileName( name );
        handle = open( FileErrorName, flags, 0666 );
        if( handle != -1 && name->Flags == 1 ) {
            file = AllocMemory( sizeof(OpenFile) );
            file->Handle = handle;
            file->Available = 0;
            if( !(flags & 2) ) {
                file->Buffer = AllocMemory( 8192 );
                file->Mode = 1;
            } else {
                file->Buffer = AllocMemory( 4096 );
                file->Mode = 2;
            }
            file->Current = file->Buffer;
            file->Position = 0;
        }
    }
    if( name->Flags == 1 ) name->File = file;
    else name->File = (OpenFile *)handle;
    return( handle != -1 );
}

/* Original0x4125c3..0x41262b always emits the separating dot, even
   for an empty extension. Flags3 denotes an intentionally absent name. */
void BuildFileName( FileName *name )
{
    unsigned i, used;
    PathName *path;

    if( name->Flags == 3 ) {
        FileErrorName[0] = 0;
    } else {
        path = name->Path;
        used = 0;
        for( i = 0; i < path->DirectoryLength; ++i ) {
            FileErrorName[used++] = path->Directory[i];
        }
        for( i = 0; i < name->NameLength; ++i ) {
            FileErrorName[used++] = name->Name[i];
        }
        FileErrorName[used++] = '.';
        for( i = 0; i < path->ExtensionLength; ++i ) {
            FileErrorName[used++] = path->Extension[i];
        }
        FileErrorName[used] = 0;
    }
}

/* Original0x412368..0x4123da. Compact any remainder before reading one
   more4096-byte block. A short final request returns its partial span and
   marks Current NULL; Available still records the trailing bytes. */
unsigned char *ReadBuffered( OpenFile *file, int size )
{
    unsigned char *result;
    int count;

    if( file->Current == NULL ) return( NULL );
    if( size > file->Available ) {
        CheckBreak();
        memmove( file->Buffer, file->Current, file->Available );
        file->Current = file->Buffer;
        count = read( file->Handle, file->Buffer + file->Available, 4096 );
        if( count == -1 ) return( NULL );
        file->Available += count;
    }
    result = file->Current;
    if( size <= file->Available ) {
        file->Current += size;
        file->Available -= size;
        file->Position += size;
    } else {
        file->Current = NULL;
        file->Position += file->Available;
    }
    return( result );
}

/* Original0x41253a..0x4125ab releases the buffered object after its
   mode-specific flush/cache shutdown. Raw-descriptor -1 stays untouched. */
void CloseLibraryFile( FileName *name )
{
    OpenFile *file;

    if( name->Flags == 1 ) {
        file = name->File;
        if( file->Mode == 3 ) {
            CloseFileCache( file->Handle );
        } else {
            if( file->Mode == 2 && file->Current > file->Buffer ) {
                WriteFileBlock( file, file->Buffer, file->Current - file->Buffer );
            }
            if( file->Handle != -1 ) close( file->Handle );
        }
        FreeMemory( file->Buffer );
        FreeMemory( file );
        name->File = NULL;
    } else if( name->File != (OpenFile *)-1 ) {
        close( (int)name->File );
        name->File = NULL;
    }
}

/* Original0x4123da..0x412446. Reuse bytes already in the buffer when
   possible; other positions restart at the containing4096-byte block. */
void SeekBuffered( OpenFile *file, unsigned long position )
{
    unsigned long distance, aligned;

    if( position > file->Position ) {
        distance = position - file->Position;
        if( distance <= (unsigned)file->Available ) {
            ReadBuffered( file, distance );
            return;
        }
    } else {
        distance = file->Position - position;
        if( distance < (unsigned long)file->Current -
                       (unsigned long)file->Buffer ) {
            file->Current -= distance;
            file->Available += distance;
            file->Position -= distance;
            return;
        }
    }
    aligned = position & ~0xfffUL;
    lseek( file->Handle, aligned, 0 );
    file->Current = file->Buffer;
    file->Available = 0;
    file->Position = aligned;
    ReadBuffered( file, position - aligned );
}

/* Original0x4125ab..0x4125c3 deliberately uses the general buffered
   writer for one byte, retaining the original helper edge. */
void WriteBufferedByte( OpenFile *file, unsigned char ch )
{
    WriteBuffered( file, &ch, 1 );
}

/* Original0x4124a1..0x41253a keeps a4096-byte output window. Complete
   intervening blocks bypass that window while the final remainder stays. */
void WriteBuffered( OpenFile *file, const void *data, int size )
{
    const unsigned char *source;
    int space, whole, i;

    source = data;
    space = 4096 - (file->Current - file->Buffer);
    file->Position += size;
    if( space < size ) {
        for( i = 0; i < space; ++i ) file->Current[i] = source[i];
        WriteFileBlock( file, file->Buffer, 4096 );
        file->Current = file->Buffer;
        size -= space;
        source += space;
        whole = size - size % 4096;
        WriteFileBlock( file, source, whole );
        size -= whole;
        source += whole;
    }
    for( i = 0; i < size; ++i ) file->Current[i] = source[i];
    file->Current += size;
}

/* Original0x412364..0x412368. Position includes buffered input/output
   consumed so far rather than only the underlying descriptor offset. */
unsigned long BufferedPosition( OpenFile *file )
{
    return( file->Position );
}

/* Original0x41231e..0x41233e, displaced diagnostic head412308 and
   terminal error block412471. A short strict read aborts the command. */
int ReadBufferedByteStrict( OpenFile *file )
{
    unsigned char *data;

    data = ReadBuffered( file, 1 );
    if( data == NULL || file->Current == NULL ) {
        BuildFileName( CurrentName );
        ReportError( 2, 54, FileErrorName );
        ToolFlags |= 0x40;
        AbortCommand();
    }
    return( *data );
}

/* Original0x41233e..0x412364 permits a clean EOF between records.
   The sentinel deliberately shares the value of a literal0xff byte. */
int ReadBufferedByte( OpenFile *file )
{
    unsigned char *data;

    data = ReadBuffered( file, 1 );
    if( data == NULL || file->Current == NULL ) return( 0xff );
    return( *data );
}

/* Original0x412446..0x4124a1 shares its diagnostic/abort tail with
   the strict reader. Partial writes use the original resource diagnostic. */
void WriteFileBlock( OpenFile *file, const void *data, unsigned size )
{
    char error[80];
    int count;

    CheckBreak();
    count = write( file->Handle, data, size );
    if( count == -1 ) {
        ReportError( 2, 103, strerror( errno ) );
        ToolFlags |= 0x40;
        AbortCommand();
    } else if( count != size ) {
        MsgGet( 104, error );
        ReportError( 2, 103, error );
        ToolFlags |= 0x40;
        AbortCommand();
    }
}

/* Original0x412249..0x4122b9. Create an exclusive scratch file,
   advancing only the last two basename digits on name collisions. */
int CreateTemporaryLibrary( FileName *name )
{
    int handle, length;

    for( ;; ) {
        BuildFileName( name );
        handle = open( FileErrorName, O_WRONLY | O_CREAT | O_BINARY | O_EXCL, 0666 );
        if( handle != -1 ) {
            close( handle );
            return( 1 );
        }
        length = (signed char)name->NameLength;
        ++name->Name[length - 1];
        if( name->Name[length - 1] <= '9' ) continue;
        name->Name[length - 1] = '0';
        ++name->Name[length - 2];
        if( name->Name[length - 2] > '9' ) return( 0 );
    }
}

/* Original0x4127ea..0x412836. Preserve the input name while giving the
   output its own path record for subsequent extension changes. */
void CopyOutputFileName( void )
{
    *OutputLibraryName = *LibraryName;
    OutputLibraryName->Path = AllocPoolItem( &PathPool, sizeof(PathName) );
    *OutputLibraryName->Path = *LibraryName->Path;
}
