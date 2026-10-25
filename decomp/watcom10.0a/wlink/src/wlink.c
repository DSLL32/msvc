/* 1994 command-line callback, VA4243c8..424525. The initial zero
 * command pointer and the full final link phases belong to this encountered
 * entry; no independent entry into its interior has been established.
 * Later released wlink.c correlates phase names; target order is authoritative. */
#include <signal.h>
#include "link.h"
#include <recon.h>

/* Only fields read by this function are defined here. */
typedef struct address {
    unsigned long off;
    unsigned short seg;
} address;
typedef struct start_symbol {
    void *link;
    char *name;
    address addr;
} start_symbol;

unsigned short FormatType;       /* original4500a2 */
unsigned char FormatDll;         /* original4500b3, lowbit */
void *ObjLibFiles;               /* original4500bc */
start_symbol *StartSym;          /* original450120 */
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
    if( ObjLibFiles == 0 && !(FormatType & 0x100) && !(FormatDll & 1) ) {
        if( StartSym == 0 ) {
            StartInfo.off = 0;
            StartInfo.seg = 0;
        } else {
            StartInfo.off = StartSym->addr.off;
            StartInfo.seg = StartSym->addr.seg;
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
void *LibModules;                /* original450140 */
void *Groups;                    /* original45013c */
void *AbsGroups;                 /* original450150 */
void *OvlClasses;                /* original450144 */
void *OvlVectors;                /* original450138 */
unsigned VecNum;                /* original450134 */
address CurrLoc;                /* original450170 */
void *CurrentModule;            /* original450148 */
void *CurrMod;                  /* original4500f8 */
void *OverlayWork;              /* original450114; role provisional */
unsigned OvlNum;                /* original450108 */
unsigned SectionNum;            /* original45010c */
char *OvlFName;                 /* original450104 */
void *Root;                     /* original4500f0 */
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
    ObjLibFiles = 0;
    MapFName = 0;
    OutFiles = 0;
    LibModules = 0;
    Groups = 0;
    AbsGroups = 0;
    StartSym = 0;
    OvlClasses = 0;
    OvlVectors = 0;
    VecNum = 0;
    CurrLoc.off = 0;
    CurrentModule = 0;
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
    InitObjIO();
    StackSize = 0x200;
}
