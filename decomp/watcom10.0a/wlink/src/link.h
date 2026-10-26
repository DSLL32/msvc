#ifndef WLINK_LINK_H
#define WLINK_LINK_H

/* Only the encountered outfilelist prefix is known at this frontier. */
typedef struct outfilelist {
    struct outfilelist *next;
    char *fname;
    int handle;
} outfilelist;
typedef struct file_list file_list;
typedef struct omf_dict_entry omf_dict_entry;
typedef struct mod_entry mod_entry;
typedef struct path_entry {
    struct path_entry *next;
    char name[1];
} path_entry;

typedef char link_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char link_int_is_32[(sizeof(int) == 4) ? 1 : -1];
typedef char link_state_is_16[(sizeof(unsigned short) == 2) ? 1 : -1];

extern char **Argv;
extern int MapFile;
extern char *MapFName;
extern outfilelist *OutFiles;
extern path_entry *LibPath;
extern unsigned short LinkState;

extern int Spawn( void (*)(void) );
extern void DoLinkFromCommandLine( void );
extern void QClose( int, char * );
extern void QDelete( char * );
extern void _LnkFree( void * );
extern void BurnSystemList( void );
extern void FreeList( path_entry * );
extern void CloseSpillFile( void );
extern void FreeUndefs( void );
extern void FreePaths( void );
extern void CleanLinkStruct( void );
extern void FreeFormatStuff( void );
extern void FreeObjInfo( void );
extern void FiniLinkStruct( void );
extern void FreeVirtMem( void );
extern void FiniSym( void );
extern unsigned char FiniMsg( void );
extern void LnkMemFini( void );
extern void exit( int );
#if defined(__WATCOMC__)
/* Genuine CRT exit does not return. Watcom10 supports this declaration;
 * its shipped SETJMP.H uses the same annotation for longjmp. */
#pragma aux exit aborts;
#endif

extern void LnkInit( void );
extern void TrapBreak( int );
extern void StartTime( void );
extern void DoCmdFile( char * );
extern void CheckErr( void );
extern void MapInit( void );
extern void ProcObjFiles( void );
extern void DoDefaultSystem( void );
extern void FindLibPaths( void );
extern void ResolveUndefined( void );
extern void DecideFormat( void );
extern void SetFormat( void );
extern void ConvertLazyRefs( void );
extern void PrepareSegments( void );
extern void CheckSymbolPass( int );
extern void DefBSSSyms( void );
extern void ReportUndefined( void );
extern void PreAddrCalcFormatSpec( void );
extern void CheckClassOrder( void );
extern void SetStkSize( void );
extern void AutoGroup( void );
extern void CalcAddresses( void );
extern void GetBSSSize( void );
extern void GetStkAddr( void );
extern void PostAddrCalcFormatSpec( void );
extern void ReportLinkMap( void );
extern void InitLoadFile( void );
extern void ObjPass2( void );
extern void FiniLoadFile( void );
extern void EndTime( void );
extern void LnkMsg( unsigned, const char *, ... );
extern int InitMsg( char * );
extern void LnkFilesInit( void );
extern void InitNodes( void );
typedef struct nodearray nodearray;
extern nodearray *MakeArray( unsigned );
extern void InitTokBuff( void );
extern void InitSpillFile( void );
extern void SetBreak( void );
extern void InitSym( void );
extern void InitSymStorage( void );
#pragma pack(1)
typedef struct section {
    struct section *next_sect;
    void *files;
    void *mods;
    void *classlist;
    struct { unsigned long off; unsigned short seg; } sect_addr;
    unsigned short ovl_num;
    void *areas;
    struct section *parent;
    unsigned short relocs;
    unsigned long size;
    void *reloclist;
    union { unsigned long file_loc; void *dist_mods; } u;
    void *dbg_info;
    outfilelist *outfile;
} section;
#pragma pack()
typedef char link_section_is_54[(sizeof(section) == 54) ? 1 : -1];
extern section *Root;
extern file_list *ObjLibFiles;
extern mod_entry *LibModules, *FakeModule;
extern void BurnLibs( void );
extern void FreeDictCache( void **, unsigned short );
#pragma pack(1)
struct file_list {
    struct file_list *next_file;
    void *file;
    union { omf_dict_entry *dict; path_entry *member; } u;
    unsigned long status;
};
#pragma pack()
extern void FreeFiles( file_list * );
extern void CacheClose( file_list *, unsigned );
extern void FreeMods( mod_entry * );
extern void FreeAMod( mod_entry * );
typedef struct saved_seg saved_seg;
extern void FreeSavedSegs( void );
extern void WalkSavedSegs( void (*)(saved_seg *) );
extern void FreeSavedSeg( saved_seg * );
extern void FreeNodes( nodearray * );
extern nodearray *GrpNodes, *SegNodes, *ExtNodes;
extern path_entry *ObjInfoList, *AuxObjInfoList;
typedef struct group_entry group_entry;
#pragma pack(1)
struct group_entry {
    struct group_entry *next_group;
    void *pieces;
    struct { unsigned long off; unsigned short seg; } grp_addr;
    unsigned char unknown[4];
    void *addrinfo;
    void *grp_relocs;
    unsigned long unknown26;
    unsigned long totalsize;
};
#pragma pack()
extern group_entry *Groups, *AbsGroups;
typedef struct reloc_info reloc_info;
extern void FreeRelocInfo( void );
extern unsigned char FreeRelocList( reloc_info * );
extern void FreeRelocSect( section * );
extern unsigned char TraverseOS2RelocList( group_entry *, unsigned char (*)(reloc_info *) );
extern void ProcAllOvl( void (*)(section *) );
extern void FreeGroups( group_entry * );
extern void FreeAGroup( group_entry * );
typedef struct ovl_area ovl_area;
extern void FreeSections( section * );
extern void FreeAreas( ovl_area * );
extern void DBISectCleanup( section * );
extern void ODBISectCleanup( section * );
extern void DBICleanup( void );
extern void FreeOvlStruct( void );
extern void FreeDistStuff( void );
extern void ReleasePass1( void );
extern void RelSymBlock( void );
extern void BurnNodes( void );
extern void BurnNodeArray( nodearray * );
extern void FreeTokBuffs( void );
extern void *FreeClassBlocks;
extern section *NewSection( void );
extern void VirtMemInit( void );
typedef struct handle_info {
    int handle;
    char *filename;
} HANDLE_INFO;
extern int OpenResFile( HANDLE_INFO * );
extern int CloseResFile( HANDLE_INFO * );
extern int ResCloseFile( int );
extern int ResOpenFileRO( const char * );
extern void WresRecordError( int );
extern int FindResources( HANDLE_INFO * );
extern long ResSeek( int, long, int );
extern long QLSeek( int, long, int, char * );
extern void CheckBreak( void );
typedef struct WResDirHead *WResDir;
extern void WResFreeDir( WResDir );
extern int InitResources( HANDLE_INFO * );
extern WResDir WResInitDir( void );
extern void *ChkLAlloc( unsigned );
extern void *LAlloc( unsigned );
extern unsigned char FreeUpMemory( void );
extern long QPos( int );
extern int WResReadDir( int, WResDir, int * );
extern void __FreeTypeList( WResDir );
extern int WResIsWResFile( int );
extern int ResReadUint32( unsigned long *, int );
extern int readWResDir( int, WResDir );
typedef struct WResHeader WResHeader;
typedef struct WResExtHeader WResExtHeader;
extern int WResReadHeaderRecord( WResHeader *, int );
extern int WResReadExtHeader( WResExtHeader *, int );
extern int readTypeList( int, WResDir, unsigned short );
typedef struct WResID WResID;
typedef struct WResIDName WResIDName;
typedef struct WResTypeInfo WResTypeInfo;
typedef struct WResTypeNode WResTypeNode;
extern void __FreeResList( WResTypeNode * );
extern int WResReadFixedTypeRecord( WResTypeInfo *, int );
extern int WResIDExtraBytes( const WResID * );
extern int WResReadExtraWResID( WResID *, int );
extern void ResAddLLItemAtEnd( void **, void **, void * );
extern int readResList( int, WResTypeNode *, unsigned short );
typedef struct WResResNode WResResNode;
extern void __FreeLangList( WResResNode * );
typedef struct WResLangNode WResLangNode;
#pragma pack(1)
typedef struct WResLangType {
    unsigned short lang;
    unsigned char sublang;
} WResLangType;
typedef struct WResLangInfo {
    WResLangType lang;
    unsigned short MemoryFlags;
    unsigned long Offset, Length;
} WResLangInfo;
typedef struct WResDirWindow {
    WResResNode *CurrRes;
    WResTypeNode *CurrType;
    WResLangNode *CurrLang;
} WResDirWindow;
#pragma pack()
extern void WResInitIDFromNum( long, WResID * );
extern WResDirWindow WResFindResource( const WResID *, const WResID *, WResDir, WResLangType * );
extern WResTypeNode *__FindType( const WResID *, WResDir );
extern WResResNode *__FindRes( const WResID *, WResTypeNode * );
extern WResLangNode *__FindLang( WResLangType *, WResResNode * );
extern int WResIDCmp( const WResID *, const WResID * );
extern int WResIDNameCmp( const WResIDName *, const WResIDName * );
extern WResLangInfo *WResGetLangInfo( WResDirWindow );
typedef struct WResResInfo WResResInfo;
typedef struct WResResInfo1 WResResInfo1;
extern int WResReadFixedResRecord1( WResResInfo1 *, int );
extern int WResReadFixedResRecord( WResResInfo *, int );
extern int readLangInfoList( int, WResResNode * );
extern int readMResDir( int, WResDir, int * );
extern unsigned QWrite( int, void *, unsigned, char * );
#include <recon.h>
#endif
