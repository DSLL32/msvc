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
