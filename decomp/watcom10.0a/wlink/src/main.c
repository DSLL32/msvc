/* Independently recovered from WLINK 10.0a VA4242e0..4243c8. The later
 * released wlink.c supplies name correlations, not this version's main graph.
 * The cleanup is reached by fallthrough after Spawn. No separate entry at
 * VA4242f0 is established; a later observed entry must revise this extent. */
#include "link.h"
#include <recon.h>

char **Argv;                  /* original DWORD45021c */
int MapFile;                  /* original DWORD45015c, initially BSS zero */
char *MapFName;                /* original DWORD450164 */
outfilelist *OutFiles;         /* original DWORD450160 */
path_entry *LibPath;           /* original DWORD4500e8 */
unsigned short LinkState;     /* original WORD450186; confirmed WORD store */

int main( int argc, char **argv )
{
    outfilelist *file;

    argc = argc;
    Argv = argv;
    Spawn( DoLinkFromCommandLine );

    if( MapFile != -1 ) {
        QClose( MapFile, MapFName );
        MapFile = -1;
    }
    while( OutFiles != 0 ) {
        file = OutFiles;
        if( file->handle != -1 ) {
            QClose( file->handle, file->fname );
        }
        if( LinkState & 8 ) {
            QDelete( file->fname );
        }
        _LnkFree( file->fname );
        OutFiles = file->next;
        _LnkFree( file );
    }
    _LnkFree( MapFName );
    BurnSystemList();
    FreeList( LibPath );
    CloseSpillFile();
    CleanTraces();
    FreePaths();
    FreeUndefs();
    CleanLoadFile();
    CleanLinkStruct();
    FreeFormatStuff();
    FreeObjInfo();
    FreeVirtMem();
    FiniMsg();
    LnkMemFini();
    exit( (LinkState & 8) != 0 );
    return 0;
}
