/* 1994 command-line callback, VA4243c8..424525. The initial zero
 * command pointer and the full final link phases belong to this encountered
 * entry; no independent entry into its interior has been established.
 * Later released wlink.c correlates phase names; target order is authoritative. */
#include <signal.h>
#include "cmdline.h"
#include <recon.h>

/* Only fields read by this function are defined here. */
typedef struct address {
    unsigned long off;
    unsigned short seg;
} address;

void *InputFileList;               /* original4500bc; role remains provisional */
group_entry *Groups;           /* original450120 */
address StartInfo;               /* original450178 */

void DoLinkFromCommandLine( void )
{
    LnkInit();
    signal( SIGINT, TrapBreak );
    StartTime();
    DoCmdFile( 0 );
    CheckErr();
    MapInit();
    ProcObjFiles();
    CheckErr();
    DoDefaultSystem();
    if( LinkState & 4 ) {
        FindLibPaths();
        LinkState |= 2;
        ResolveUndefined();
        LinkState &= 0x7ffd;
        LinkState |= 0x8000;
    }
    DecideFormat();
    SetFormat();
    ConvertLazyRefs();
    PrepareSegments();
    CheckErr();
    CheckSymbolPass( 1 );
    DefBSSSyms();
    ReportUndefined();
    PreAddrCalcFormatSpec();
    CheckClassOrder();
    SetStkSize();
    AutoGroup();
    CalcAddresses();
    GetBSSSize();
    GetStkAddr();
    if( InputFileList == 0 && !(FmtData.type & 0x100) && !(FmtData.flags & 1) ) {
        if( Groups == 0 ) {
            StartInfo.off = 0;
            StartInfo.seg = 0;
        } else {
            StartInfo.off = Groups->grp_addr.off;
            StartInfo.seg = Groups->grp_addr.seg;
        }
        LnkMsg( 0x3217, "a", StartInfo );
    }
    PostAddrCalcFormatSpec();
    ReportLinkMap();
    CheckErr();
    InitLoadFile();
    ObjPass2();
    CheckErr();
    FiniLoadFile();
    EndTime();
    signal( SIGINT, SIG_IGN );
}

unsigned long LinkFlags;         /* original450154 */
file_list *ObjLibFiles;           /* original450140 */
mod_entry *LibModules;            /* original45013c */
void *FreeClassBlocks;           /* original450150; role provisional */
void *OvlClasses;                /* original450144 */
path_entry *ObjInfoList;         /* original450138; role provisional */
path_entry *AuxObjInfoList;      /* original450134; role provisional */
address CurrLoc;                /* original450170 */
mod_entry *FakeModule;            /* original450148 */
void *CurrMod;                  /* original4500f8 */
void *OverlayWork;              /* original450114; role provisional */
unsigned OvlNum;                /* original450108 */
unsigned SectionNum;            /* original45010c */
char *OvlFName;                 /* original450104 */
section *Root;                     /* original4500f0 */
unsigned long StackSize;        /* original4500e0 */

void LnkInit( void )
{
    LinkFlags = 0x40000000UL;
    LinkState = 1;
    /* Both original memory init/fini names resolve to the same no-op entry. */
    LnkMemFini();
    InitMsg( Argv[0] );
    LnkFilesInit();
    MapFile = -1;
    InputFileList = 0;
    MapFName = 0;
    OutFiles = 0;
    ObjLibFiles = 0;
    LibModules = 0;
    FreeClassBlocks = 0;
    Groups = 0;
    OvlClasses = 0;
    ObjInfoList = 0;
    AuxObjInfoList = 0;
    CurrLoc.off = 0;
    FakeModule = 0;
    CurrLoc.seg = 0xffff;
    InitNodes();
    CurrMod = 0;
    OverlayWork = 0;
    OvlNum = 0;
    SectionNum = 0;
    OvlFName = 0;
    InitTokBuff();
    InitSpillFile();
    InitSym();
    Root = NewSection();
    VirtMemInit();
    StackSize = 0x200;
}
