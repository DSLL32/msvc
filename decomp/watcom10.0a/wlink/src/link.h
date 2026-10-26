#ifndef WLINK_LINK_H
#define WLINK_LINK_H

/* The old output-file record predates buffered output fields. */
#pragma pack(1)
typedef struct outfilelist {
    struct outfilelist *next;
    char *fname;
    int handle;
    unsigned long file_loc;
    unsigned short ovlfnoff;
} outfilelist;
#pragma pack()
typedef char old_outfile_is_18[(sizeof(outfilelist) == 18) ? 1 : -1];
typedef struct file_list file_list;
typedef struct member_list member_list;
typedef struct infilelist infilelist;
typedef struct omf_dict_entry omf_dict_entry;
/* Target addresses are packed six-byte offset:segment pairs. */
#pragma pack(1)
typedef struct address {
    unsigned long off;
    unsigned short seg;
} address;
#pragma pack()
typedef char target_address_is_6[(sizeof(address) == 6) ? 1 : -1];
extern address CurrLoc;
extern address StackAddr;
extern address OvltabAddr;
extern unsigned OvltabSize;

typedef struct mod_entry mod_entry;
typedef struct debug_info debug_info;
typedef struct dbi_addr_range dbi_addr_range;
typedef struct dbi_section {
    unsigned long init, curr, size, start;
} dbi_section;
#pragma pack(1)
/* The old section debug workspace is154bytes; fields are identified as used. */
struct debug_info {
    dbi_section locallinks, local, typelinks, type, linelinks, line;
    dbi_section mod, global, addr;
    unsigned long DBIWrite;
    unsigned long line_link_start;
    short modnum;
};
#pragma pack()
typedef char old_debug_info_is_154[(sizeof(debug_info) == 154) ? 1 : -1];
extern void DoAddLocal( dbi_section *, unsigned long );
struct group_entry;
typedef struct segdata segdata;
typedef struct edgelist edgelist;
typedef struct segnode segnode;
typedef struct class_entry class_entry;
typedef struct seg_leader seg_leader;
#pragma pack(1)
struct dbi_addr_range {
    dbi_addr_range *next;
    unsigned long write_pos;
    unsigned long start;
    unsigned short seg, num;
    unsigned long curr;
    seg_leader *prev, *last;
    unsigned char flags;
};
#pragma pack()
typedef char old_dbi_addr_range_is_29[(sizeof(dbi_addr_range) == 29) ? 1 : -1];
typedef struct list_of_names list_of_names;
typedef struct nodearray nodearray;
typedef struct odbimodinfo odbimodinfo;
typedef struct dbi_demand dbi_demand;
#pragma pack(1)
typedef struct path_entry {
    struct path_entry *next;
    char name[1];
} path_entry;
#pragma pack()
typedef char old_path_header_is_5[(sizeof(path_entry) == 5) ? 1 : -1];

/* Export/symbol prefixes encountered by the old export parser. */
#pragma pack(1)
typedef struct dll_sym_info {
    unsigned short modnum, entry;
    unsigned char isordinal;
} dll_sym_info;
#pragma pack()
#pragma pack(1)
typedef struct symbol {
    struct symbol *hash;
    struct symbol *link;
    struct symbol *publink;
    struct { unsigned long off; unsigned short seg; } addr;
    unsigned short info;
    union { char *alias; void *seg; seg_leader *leader; edgelist *edges; } p;
    union { dll_sym_info *import; int aliaslen; segdata *comdat; unsigned long value; } u;
    union { void *def; unsigned long value; } e;
    char name[1];
} symbol;
typedef struct entry_export {
    struct entry_export *next;
    char *name;
    symbol *sym;
    unsigned short iopl_words;
    unsigned short ordinal;
    struct { unsigned long off; unsigned short seg; } addr;
    unsigned long flags;
} entry_export;
#pragma pack()
typedef char old_symbol_prefix_is_33[(sizeof(symbol) == 33) ? 1 : -1];
typedef char old_export_is_26[(sizeof(entry_export) == 26) ? 1 : -1];
extern entry_export *AllocExport( char * );
extern symbol *SymXOp( unsigned char, char *, int );
extern symbol *SymOp( unsigned char, char *, int );
extern symbol *DoSymOp( unsigned char, char *, int );
extern unsigned HashFn( char *, int );
extern symbol *SearchSym( char *, unsigned, unsigned, unsigned );
extern symbol *AddSym( char *, unsigned );
typedef struct vecnode {
    struct vecnode *next;
    symbol *entry;
} vecnode;
extern vecnode *OvlVectors;
extern unsigned VecNum;
extern symbol *HeadSym;
extern symbol *LastSym;
extern symbol *RefISymbol( char * );
extern void AddToExportList( entry_export * );
extern entry_export *FreeAnExport( entry_export * );
extern int (*CmpSym)( const void *, const void *, unsigned );

#pragma pack(1)
typedef struct snamelist {
    struct snamelist *next;
    unsigned char len;
    char name[1];
} snamelist;
#pragma pack()
extern snamelist *LangAlloc( unsigned char, void * );

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
extern void WriteLibsUsed( void );
extern unsigned TokSize;
extern char *TokBuff;
extern void MapInit( void );
extern int QOpenRW( char * );
extern void StartMapBuffering( void );
extern void StopMapBuffering( void );
extern void BufWrite( char *, int );
extern void WriteMapNL( unsigned );
extern char *PutDec( char *, unsigned );
extern void ProcObjFiles( void );
struct section;
extern void LoadObjFiles( struct section * );
extern void OvlPass1( void );
extern void DoPass1( file_list * );
extern unsigned short ObjFormat;
extern void DoDefaultSystem( void );
extern void FindLibPaths( void );
extern void InitModTable( void );
extern unsigned char CheckVFList( symbol * );
extern unsigned char ResolveVFExtdefs( void );
/* File-local ProcLibFile in libsupp.c, distinct from the command handler. */
typedef unsigned short (*omf_compare)( char *, char *, unsigned short );
extern unsigned char CheckLibraryType( file_list * );
typedef struct omf_hash {
    unsigned short minor_class, minor_inc, major_class, major_inc;
} omf_hash;
extern void HashSymbol( omf_hash *, char * );
extern void SetDict( file_list *, unsigned short );
extern void **AllocDict( unsigned short, unsigned short );
extern unsigned char OMFSearchExtLib( file_list *, char *, omf_compare );
extern unsigned short OMFCompName( char *, char *, unsigned short );
extern unsigned short OMFCompIName( char *, char *, unsigned short );
extern mod_entry *SearchLib( file_list *, char * );
extern void AddModTable( mod_entry *, unsigned );
extern void FinishArcs( mod_entry * );
extern unsigned char FindLibTrace( mod_entry * );
extern unsigned char ModNameCompare( char *, char * );
extern unsigned char LibProcFile( file_list *, char * );
extern void LibFind( char *, unsigned char );
extern void PrintBadTraces( void );
extern void ResolveUndefined( void );
extern void DecideFormat( void );
extern void SetFormat( void );
extern outfilelist *NewOutFile( char * );
extern void FillOutFilePtrs( void );
extern void FillOutPtr( struct section * );
extern void CmdQNXFini( void );
extern void SetRelocSize( void );
extern unsigned FmtRelocSize;
extern unsigned short binary_log( unsigned short );
extern void ConvertLazyRefs( void );
extern void WeldSyms( symbol *, symbol * );
extern void SetSegments( void );
extern void SetSectSegments( struct section * );
extern void SetModSegments( mod_entry * );
extern void FixGroupProblems( void );
extern void FindRedefs( void );
extern unsigned NextSegIndex;
extern void *SavedSegMask;
extern struct section **SectOvlTab;
extern void CheckSymbolPass( int );
/* Provisional roles for the two older communal-symbol passes. */
extern void BSSPass1( symbol * );
extern void BSSPass2( symbol * );
extern void DefBSSSyms( void );
extern void DefABSSSym( char * );
extern void ReportUndefined( void );
extern void PreAddrCalcFormatSpec( void );
extern void ChkPEData( void );
extern void ChkOS2Data( void );
typedef struct seg_flags seg_flags;
extern void SetSegFlags( seg_flags * );
extern void FillTypeFlags( unsigned short, unsigned char );
extern void FillClassFlags( char *, unsigned short );
extern seg_leader *FindASeg( class_entry *, char * );
extern void FindExportedSyms( void );
extern void CheckClassOrder( void );
extern void ReOrderClasses( struct section * );
struct ovl_area;
extern void ReOrderAreas( struct ovl_area * );
extern void CheckClassUninitialized( class_entry * );
extern void SetStkSize( void );
extern seg_leader *StackSegment( void );
extern seg_leader *FindStack( class_entry * );
extern void AutoGroup( void );
extern void AutoGroupSect( struct section * );
extern void PackClass( class_entry *, struct section * );
extern unsigned long SetSegType( seg_leader * );
extern unsigned long PackCodeLimit;
extern unsigned char CanPack( seg_leader *, seg_leader * );
extern void PackSegs( seg_leader *, unsigned, unsigned long, struct section *, unsigned char, unsigned char );
extern struct group_entry *GetAutoGroup( unsigned, unsigned char );
extern void SortGroupList( void );
extern unsigned char SortGroupSect( struct section * );
extern unsigned char SortGroupAreas( struct ovl_area * );
extern int NumGroups;
extern struct group_entry *DataGroup;
extern void FindSplitGroups( void );
extern void CalcAddresses( void );
extern void StartMemMap( void );
extern void AllocFileSegs( void );
extern unsigned short ToQNXSel( unsigned short );
extern void FindUninitDataStart( void );
extern void AllocClasses( class_entry * );
extern void AllocSegs( class_entry * );
extern void NewSegment( seg_leader * );
extern void Align( unsigned char );
extern void AddSize( unsigned long );
extern void NormalizeAddr( void );
extern void DBIDefClass( class_entry *, unsigned long );
extern void ODBIDefClass( class_entry *, unsigned long );
extern void DwarfDefClass( class_entry *, unsigned long );
extern void CalcOvl( void );
extern unsigned long GetPEHeaderSize( void );
extern unsigned FindNumObjects( void );
extern unsigned long GetStubSize( void );
extern unsigned DoExeName( void );
extern void ReallocFileSegs( void );
extern void CalcGrpAddr( struct group_entry * );
extern void DBIAddrStart( void );
extern void DBIAddrSectStart( struct section * );
extern void ODBIAddrSectStart( struct section * );
extern unsigned long DBIAlloc( unsigned long );
extern unsigned long AllocStg( unsigned long );
extern void AllocDBIClasses( class_entry *, debug_info * );
extern void AllocDBISegs( seg_leader *, unsigned long );
extern void DwarfAddrSectStart( struct section * );
extern void NovDBIAddrStart( void );
extern void CVAddrStart( void );
extern void DBIAddrOvlStart( void );
extern void DefinePublics( void );
extern void GetBSSSize( void );
extern void GetStkAddr( void );
extern void PostAddrCalcFormatSpec( void );
extern void AllocGroupBuffers( void );
extern void InitLoadFile( void );
extern void ObjPass2( void );
extern void FiniLoadFile( void );
extern void EndTime( void );
extern void LnkMsg( unsigned, const char *, ... );
extern int InitMsg( char * );
extern void LnkFilesInit( void );
extern void InitNodes( void );
extern nodearray *MakeArray( unsigned );
extern void InitTokBuff( void );
extern void InitSpillFile( void );
extern void SetBreak( void );
extern void InitSym( void );
extern void InitSymStorage( void );
#pragma pack(1)
typedef struct section {
    struct section *next_sect;
    file_list *files;
    void *mods;
    class_entry *classlist;
    struct { unsigned long off; unsigned short seg; } sect_addr;
    unsigned short ovl_num;
    void *areas;
    struct section *parent;
    unsigned short relocs;
    unsigned long size;
    void *reloclist;
    union { unsigned long file_loc; mod_entry *dist_mods; } u;
    debug_info *dbg_info;
    outfilelist *outfile;
} section;
#pragma pack()
typedef char link_section_is_54[(sizeof(section) == 54) ? 1 : -1];
extern section *Root;
extern file_list *ObjLibFiles;
#pragma pack(1)
typedef struct lobject_data {
    unsigned char *data;
    address addr;
    segnode *seg;
    path_entry *fixups;
    unsigned long length, offset;
    unsigned char is32;
    unsigned char buffer[1];
} lobject_data;
#pragma pack()
typedef char old_lobject_header_is_27[(sizeof(lobject_data) - 1 == 27) ? 1 : -1];
extern lobject_data *CurrRec, *CurrIterated;
extern mod_entry *LibModules;
extern void BurnLibs( void );
extern void FreeDictCache( void **, unsigned short );
#pragma pack(1)
/* Old member and input-file prefixes reached by command input handling. */
struct member_list {
    struct member_list *next;
    unsigned short flags;
    char name[1];
};
struct infilelist {
    infilelist *next;
    path_entry *path_list;
    char *prefix;
    void *cache;
    unsigned long len;
    unsigned long currpos;
    int handle;
    unsigned char flags;
    char name[1];
};
struct omf_dict_entry {
    void **cache;
    unsigned long start;
    unsigned short pages, rec_length;
    unsigned char *buffer;
    unsigned long location;
};
struct file_list {
    struct file_list *next_file;
    infilelist *file;
    union { omf_dict_entry *dict; member_list *member; } u;
    unsigned short ovlref;
    unsigned short status;
    unsigned char priority;
};
#pragma pack()
typedef char old_dict_is_20[(sizeof(omf_dict_entry) == 20) ? 1 : -1];
typedef char old_file_list_is_17[(sizeof(file_list) == 17) ? 1 : -1];
#pragma pack(1)
struct mod_entry {
    union { file_list *source; } f;
    char *name;
    unsigned long location;
    symbol *publist;
    void *arcs;                 /* segment-list owner or distributed-module data */
    mod_entry *next_mod;
    union { odbimodinfo *o; void *d; void *cv; } d;
    unsigned short modinfo;
};
#pragma pack()
typedef char old_mod_entry_is_30[(sizeof(mod_entry) == 30) ? 1 : -1];
extern mod_entry *CurrMod;
extern unsigned char CacheOpen( file_list * );
extern unsigned char DoObjOpen( infilelist * );
extern unsigned NumCacheBlocks( unsigned long );
extern unsigned char QHavePath( char * );
extern int PathObjOpen( char *, char *, char *, infilelist * );
extern int TrySearchingLib( char *, char *, infilelist * );
extern void PrintIOError( unsigned, const char *, char * );
extern void *CacheRead( file_list *, unsigned long, unsigned );
extern unsigned char CacheEnd( file_list *, unsigned long );
extern unsigned long CalcAlign( unsigned long, unsigned short );
extern unsigned char EndOfLib( file_list *, unsigned long );
extern void IdentifyObject( file_list *, unsigned long );
extern unsigned char IsOMF( file_list *, unsigned long );
extern unsigned char IsORL( file_list *, unsigned long );
extern member_list *GetObjMember( file_list *, unsigned long * );
extern mod_entry *NewModEntry( void );
extern void InitArcBuffer( mod_entry * );
extern unsigned long ObjPass1( void );
extern void SymModStart( void );
extern void DBIInitModule( mod_entry * );
extern void ODBIInitModule( mod_entry * );
extern void DwarfInitModule( mod_entry * );
extern void CVInitModule( mod_entry * );
extern unsigned long OMFPass1( void );
extern void P1Start( void );
extern unsigned long ProcObj( file_list *, unsigned long, void (*)(unsigned char) );
extern void Pass1Cmd( unsigned char );
extern void ProcTHEADR( void );
extern void Comment( unsigned pass );
extern void MSExportKeyword( void );
extern void ProcExportKeyword( void );
extern void MSImportKeyword( void );
extern unsigned short ReadImpName( unsigned char );
extern unsigned short AddImpName( char *, unsigned, unsigned char );
extern unsigned short AddNameTable( char *, unsigned, unsigned char, snamelist ** );
extern void ProcImportKeyword( void );
extern void CheckLibTrace( file_list * );
extern void AddCommentLib( char *name, unsigned len, unsigned char priority );
extern void DoLazyExtdef( unsigned char isweak );
extern void DBIComment( void );
extern void LinkDirective( unsigned pass );
extern void DBIP1Source( unsigned char *, unsigned char * );
extern void ODBIP1Source( unsigned char, unsigned char, unsigned char *, int );
extern unsigned short FindMatch( unsigned char, void * );
extern void DBIP2Source( unsigned char *, unsigned char * );
extern void ProcVFTableRecord( unsigned char );
extern void ProcVFReference( void );
extern unsigned short GetIdx( void );
extern void *FindNode( nodearray *, unsigned );
extern unsigned long PackDataLimit;
#pragma pack(1)
struct segnode {
    void *entry;
    unsigned long offset;
    struct { unsigned long off; unsigned short seg; } addr;
    unsigned long length;
    unsigned short info;
};
#pragma pack()
typedef char old_segnode_is_20[(sizeof(segnode) == 20) ? 1 : -1];
extern void AddNames( void );
extern list_of_names *AddName( char *, unsigned );
extern void *AllocNode( nodearray * );
#pragma pack(1)
struct list_of_names {
    list_of_names *next_name;
    unsigned char flags;
    char name[1];
};
#pragma pack()
extern void ProcSegDef( void );
extern segdata *AllocSegData( void );
extern void *Pass1Alloc( unsigned );
extern list_of_names *FindName( unsigned short );
extern void SkipIdx( void );
extern void AllocateSegment( segdata * );
extern unsigned char CheckOvlClass( char *, unsigned char * );
extern unsigned char IsCodeClass( char *, unsigned );
extern section *DBIGetSect( char * );
#pragma pack(1)
struct class_entry {
    class_entry *next_class;
    seg_leader *segs;
    seg_leader *old_segs;
    unsigned char flags;
    char name[1];
};
#pragma pack()
typedef char old_class_prefix_is_14[(sizeof(class_entry) == 14) ? 1 : -1];
#pragma pack(1)
struct seg_flags {
    seg_flags *next;
    unsigned short flags;
    char *name;
    unsigned char type;
};
#pragma pack()

extern void DBIColClass( class_entry * );
extern unsigned char IsStackClass( char *, unsigned );
extern class_entry *FindClass( section *, char *, unsigned char );
#pragma pack(1)
struct seg_leader {
    seg_leader *next_seg;
    section *section;
    struct { unsigned long off; unsigned short seg; } seg_addr;
    unsigned long data;             /* virtual-memory backing at14 */
    struct group_entry *group;
    unsigned long data_start;
    unsigned long size;
    unsigned long addr_adjustment;
    unsigned short info;
    unsigned short num;
    list_of_names *segname;
    mod_entry *mod;
    unsigned short recnum;
    unsigned long saved_size;  /* reused as old DBI address-info position */
    union {
        struct { unsigned short segflags; unsigned char reserved[4]; } p1;
        struct { unsigned long prev_offset; unsigned short prev_mod; } p2;
    } u;
};
#pragma pack()
typedef char old_leader_is_58[(sizeof(seg_leader) == 58) ? 1 : -1];
extern unsigned short DBIColSeg( class_entry * );
extern void Set32BitMode( void );
extern unsigned char DBISkip( unsigned short );
extern seg_leader *MakeNewLeader( class_entry *, list_of_names *, unsigned long, unsigned short );
extern seg_leader *FindALeader( class_entry *, list_of_names *, unsigned short );
extern seg_leader *InitLeader( list_of_names *, unsigned short );
extern void CheckForLast( seg_leader *, class_entry * );
extern unsigned long AddToSegment( segdata *, unsigned long * );
extern seg_leader *CurrentSeg;
extern unsigned long CAlign( unsigned long, unsigned char );
extern unsigned long BumpUp( unsigned long, unsigned long );
extern void ResizeSegment( seg_leader *, unsigned long *, unsigned long, unsigned char, unsigned char );
extern void ODBIAddAddrInfo( seg_leader *, unsigned long );
extern void DwarfAddAddrInfo( seg_leader *, unsigned long );
extern void CVAddAddrInfo( seg_leader *, unsigned long );
extern void DBIAddAddrInfo( seg_leader *, unsigned long );
extern void DBIGenAddrInfo( seg_leader *, unsigned long, unsigned long, section * );
extern void DBIGenLocal( seg_leader *, unsigned long );
extern void ODBIAddLocal( unsigned short, unsigned long );
extern void CVAddLocal( unsigned short, unsigned long );
extern void DBIAddLocal( unsigned short, unsigned long );
extern seg_leader *AddSegment( segdata *, segnode *, section *, unsigned long * );
extern void FreeSegData( segdata * );
extern section *NonSect;
extern unsigned char TestBit( void *, unsigned );
#pragma pack(1)
struct segdata {
    segdata *next;
    unsigned long length;
    union { list_of_names *name; seg_leader *leader; } u;
    union { list_of_names *name; class_entry *entry; } cl;
    union {
        struct { unsigned short frame; unsigned char reserved[2]; } absolute;
        unsigned long saved_addr;
        edgelist *refs;
    } a;
    unsigned long line_size;
    unsigned index;
    union {
        struct { unsigned short sect_id, reserved; } normal;
        symbol *sym;
    } owner;
    unsigned char acbp;
    unsigned char flags;
};
#pragma pack()
typedef char old_segdata_is_34[(sizeof(segdata) == 34) ? 1 : -1];
extern void ReportMultiple( symbol *, char *, int );
extern void ClearSymUnion( symbol * );
extern void DefStripSym( symbol *, segdata * );
extern void CleanStripInfo( symbol * );
extern void ODBIAddGlobal( symbol *, unsigned );
extern void DwarfAddGlobal( symbol *, unsigned );
extern void CVAddGlobal( symbol *, unsigned );
extern void NovDBIAddGlobal( unsigned );
extern void DBIAddGlobal( symbol *, unsigned );
extern void TryDefVector( symbol * );
extern void OvlDefVector( symbol * );
extern void DefDistribSym( symbol * );
extern void XDefSymAddr( symbol *, unsigned long, unsigned short );
extern void DefineSymbol( symbol *, segnode *, unsigned long, unsigned short );
extern void ProcPubdef( unsigned char isstatic );
#pragma pack(1)
typedef struct extnode {
    symbol *entry;
    unsigned short flags;
} extnode;
#pragma pack()
extern void TryRefVector( symbol * );
extern void TryUseVector( symbol *, extnode * );
extern void UndefinedSym( symbol * );
extern void DefineReference( symbol *, extnode * );
extern void UseSymbols( unsigned char isstatic, unsigned char comdat );
extern void DefineGroup( unsigned char pass );
extern void ODBIAddLines( seg_leader *, unsigned char, unsigned char );
extern void DwarfAddLines( seg_leader *, unsigned char, unsigned char );
extern void CVAddLines( seg_leader *, unsigned char, unsigned char );
extern void DBIAddLines( seg_leader *, unsigned char, unsigned char );
extern void DBIGenLines( seg_leader *, seg_leader *, unsigned long, unsigned char );
extern void ProcLinnum( unsigned char pass );
extern void ProcLinsym( unsigned char pass );
extern void ProcComdef( unsigned char pass, unsigned char isstatic );
extern void ProcComdat( unsigned char pass );
extern void ProcLxdata( void );
extern segnode *CurrNode;
extern void DoRelocs( void );
extern mod_entry *StartMod;
extern void DataRef( symbol * );
extern void RefGroup( struct group_entry * );
extern void ProcModuleEnd( void );
/* The OMF file-local ProcAlias is distinct from the command handler. */
extern void OMFProcAlias( void );
extern unsigned char *ObjBuff, *EOObjRec;
extern unsigned RecNum;
extern void EarlyEOF( void );
extern void BadObject( void );
extern void ResolveComdats( void );
extern void IterateNodeArray( void *, void (*)(void *), unsigned short, int );
extern void IterateNodelist( nodearray *, void (*)(void *) );
extern void CollapseLazy( void * );
extern void CollapseLazyExtdefs( void );
extern unsigned short TargThreads[4];
extern void RefSeg( segdata * );
extern void DoStripReference( unsigned, unsigned char );
extern void VectorReference( unsigned );
extern unsigned long ORLPass1( void );
extern void SymModEnd( void );
extern void ResetAddrInfo( void );
extern void ODBIP1ModuleScanned( void );
extern void DwarfP1ModuleScanned( void );
extern void CVP1ModuleScanned( void );
extern void DBIP1ModuleScanned( void );
extern char *DBIModName;
extern unsigned DBIModNameLen;
extern void ODBIGenModule( mod_entry * );
extern void DwarfGenModule( void );
extern void CVP1ModuleFinished( mod_entry * );
extern void DBIGenModule( mod_entry * );
extern void ReleaseNamelist( void * );
extern void ReleaseNames( void );
extern void TraceSymList( symbol * );
extern void FreeFiles( file_list * );
extern unsigned char FreeCacheBlocks( infilelist *, unsigned char );
extern void FreeObjCache( file_list * );
extern void CacheClose( file_list *, unsigned );
extern void FreeMods( mod_entry * );
extern void FreeAMod( mod_entry * );
typedef segdata saved_seg;
extern saved_seg *SavedSegs;
extern void ResetSegAddr( void * );
extern void ResetSavedSegAddr( saved_seg * );
extern void FinishComdat( saved_seg * );
extern void FreeSavedSegs( void );
extern void WalkSavedSegs( void (*)(saved_seg *) );
extern void FreeSavedSeg( saved_seg * );
extern void FreeNodes( nodearray * );
extern nodearray *GrpNodes, *SegNodes, *ExtNodes, *NameNodes;
extern path_entry *ObjInfoList, *AuxObjInfoList;
typedef struct group_entry group_entry;
#pragma pack(1)
struct group_entry {
    struct group_entry *next_group;
    section *section;
    struct { unsigned long off; unsigned short seg; } grp_addr;
    short num, free_slots;
    seg_leader **leaders;
    void *grp_relocs;
    unsigned long size;
    unsigned long totalsize;
    unsigned long data;
    char *name;
    unsigned short segflags;
    unsigned long linear;
    short autogroup;
    union { unsigned short info, qnxflags; } u;
};
#pragma pack()
typedef char old_group_is_52[(sizeof(group_entry) == 52) ? 1 : -1];
extern group_entry *Groups, *AbsGroups;
extern group_entry *OvlGroup;
extern seg_leader *OvlRefs;
extern group_entry *FindGroup( list_of_names * );
extern void AddToGroup( group_entry *, seg_leader *, unsigned char );
typedef struct reloc_info reloc_info;
extern void FreeRelocInfo( void );
extern unsigned char FreeRelocList( reloc_info * );
extern void FreeRelocSect( section * );
extern unsigned char TraverseOS2RelocList( group_entry *, unsigned char (*)(reloc_info *) );
extern void ProcAllOvl( void (*)(section *) );
extern void CheckFileTrace( section * );
extern void NumASect( section * );
extern void FreeGroups( group_entry * );
extern void FreeAGroup( group_entry * );
typedef struct ovl_area ovl_area;
struct ovl_area {
    ovl_area *next;
    section *sections;
};
extern void WalkAreas( ovl_area *, void (*)(section *) );
extern void FreeSections( section * );
extern void FreeAreas( ovl_area * );
extern void DBISectCleanup( section * );
extern void ODBISectCleanup( section * );
extern void DBICleanup( void );
extern void ODBIInit( section * );
extern void CVInit( void );
extern void FreeOvlStruct( void );
extern void FreeDistStuff( void );
extern void ReleasePass1( void );
extern void RelSymBlock( void );
extern void BurnNodes( void );
extern void BurnNodeArray( nodearray * );
extern void FreeTokBuffs( void );
extern list_of_names *SavedNames;
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
extern long QSeek( int, long, char * );
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
typedef struct block_data block_data;
extern void *PermAlloc( unsigned );
extern void *AllocBlock( unsigned, block_data * );
extern infilelist *AllocEntry( char *, path_entry * );
extern void LinkList( void *, void * );
extern void GetNewBlock( block_data *, unsigned );
extern unsigned char ShrinkBlock( block_data * );
extern void *LnkExpand( void *, unsigned );
extern void *LnkReAlloc( void *, unsigned );
#include <recon.h>
#endif
