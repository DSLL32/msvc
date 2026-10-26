/* Named application callees remain trapped until reached and reconstructed. */
#include "cmdline.h"
#include <recon.h>

#if defined(__WATCOMC__)
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#define TRAP() reconstruction_trap()
#else
#define TRAP() __asm__ __volatile__( "int3" )
#endif

void QClose( int handle, char *name ) { handle = handle; name = name; TRAP(); }
void QDelete( char *name ) { name = name; TRAP(); }

void TrapBreak( int value ) { value = value; TRAP(); }
void CheckErr( void ) { TRAP(); }
void MapInit( void ) { TRAP(); }
void ProcObjFiles( void ) { TRAP(); }
void DoDefaultSystem( void ) { TRAP(); }
void FindLibPaths( void ) { TRAP(); }
void ResolveUndefined( void ) { TRAP(); }
void DecideFormat( void ) { TRAP(); }
void SetFormat( void ) { TRAP(); }
void ConvertLazyRefs( void ) { TRAP(); }
void PrepareSegments( void ) { TRAP(); }
void CheckSymbolPass( int value ) { value = value; TRAP(); }
void DefBSSSyms( void ) { TRAP(); }
void ReportUndefined( void ) { TRAP(); }
void PreAddrCalcFormatSpec( void ) { TRAP(); }
void CheckClassOrder( void ) { TRAP(); }
void SetStkSize( void ) { TRAP(); }
void AutoGroup( void ) { TRAP(); }
void CalcAddresses( void ) { TRAP(); }
void GetBSSSize( void ) { TRAP(); }
void GetStkAddr( void ) { TRAP(); }
void PostAddrCalcFormatSpec( void ) { TRAP(); }
void ReportLinkMap( void ) { TRAP(); }
void InitLoadFile( void ) { TRAP(); }
void ObjPass2( void ) { TRAP(); }
void FiniLoadFile( void ) { TRAP(); }
void EndTime( void ) { TRAP(); }
void LnkMsg( unsigned message, const char *types, ... ) { message = message; types = types; TRAP(); }

unsigned char FreeUpMemory( void ) { TRAP(); return 0; }
long QPos( int file ) { file = file; TRAP(); return 0; }
int WResReadExtraWResID( WResID *id, int file ) { id = id; file = file; TRAP(); return 0; }
int WResReadFixedResRecord( WResResInfo *info, int file )
{ info = info; file = file; TRAP(); return 0; }
int readLangInfoList( int file, WResResNode *node )
{ file = file; node = node; TRAP(); return 0; }
int readMResDir( int file, WResDir dir, int *duplicates )
{ file = file; dir = dir; duplicates = duplicates; TRAP(); return 0; }
int WResIDNameCmp( const WResIDName *first, const WResIDName *second )
{ first = first; second = second; TRAP(); return 0; }



void WresRecordError( int status ) { status = status; TRAP(); }



unsigned QRead( int file, void *buffer, unsigned length, char *name )
{ file = file; buffer = buffer; length = length; name = name; TRAP(); return 0; }
unsigned char ProcOne( parse_entry *table, unsigned separator, unsigned char required )
{ table = table; separator = separator; required = required; TRAP(); return 0; }
unsigned char ProcDosHelp( void ) { TRAP(); return 0; }
unsigned char ProcOS2Help( void ) { TRAP(); return 0; }
unsigned char ProcWindowsHelp( void ) { TRAP(); return 0; }
unsigned char ProcNTHelp( void ) { TRAP(); return 0; }
unsigned char ProcPharHelp( void ) { TRAP(); return 0; }
unsigned char ProcNovellHelp( void ) { TRAP(); return 0; }
unsigned char ProcQNXHelp( void ) { TRAP(); return 0; }
unsigned char ProcELFHelp( void ) { TRAP(); return 0; }

unsigned char CleanCachedHandles( void ) { TRAP(); return 0; }

void SetCommandFile( int handle, char *name ) { handle = handle; name = name; TRAP(); }

void DoCmdParse( void ) { TRAP(); }



void HintFormat( unsigned short format ) { format = format; TRAP(); }

void CmdNovFini( void ) { TRAP(); }

void CmdOvlFini( void ) { TRAP(); }

void AddObjLib( char *name, unsigned priority ) { name = name; priority = priority; TRAP(); }

char * FileName( char *name, unsigned len, unsigned extension, int force ) { name = name; len = len; extension = extension; force = force; TRAP(); return 0; }

void CheckTraces( void ) { TRAP(); }


void PruneSystemList( void ) { TRAP(); }

void NumberSections( void ) { TRAP(); }

void DBIInit( void ) { TRAP(); }


void FreeDictCache( void **cache, unsigned short buckets ) { cache=cache; buckets=buckets; TRAP(); }

void CacheClose( file_list *list, unsigned pass ) { list=list; pass=pass; TRAP(); }

void FreeAMod( mod_entry *mod ) { mod=mod; TRAP(); }

void SetQNXFmt( void ) { TRAP(); }
void FreeQNXFmt( void ) { TRAP(); }
void SetELFFmt( void ) { TRAP(); }
void FreeELFFmt( void ) { TRAP(); }
void SetOS2Fmt( void ) { TRAP(); }
void FreeOS2Fmt( void ) { TRAP(); }
void SetPharFmt( void ) { TRAP(); }
void FreePharFmt( void ) { TRAP(); }
void SetNovFmt( void ) { TRAP(); }
void FreeNovFmt( void ) { TRAP(); }


void FreeSavedSeg( saved_seg *seg ) { seg=seg; TRAP(); }


unsigned char TraverseOS2RelocList( group_entry *grp, unsigned char (*fn)(reloc_info *) ) { grp=grp; fn=fn; TRAP(); return 0; }
void ProcAllOvl( void (*fn)(section *) ) { fn=fn; TRAP(); }

void FreeAGroup( group_entry *group ) { group=group; TRAP(); }


void ODBISectCleanup( section *sec ) { sec=sec; TRAP(); }
