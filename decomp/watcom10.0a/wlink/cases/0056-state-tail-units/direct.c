/* Source-backed old reset effects and a real pre-opened owned spill file. */
#include <string.h>
#include <stdio.h>
#include <recon-tool.h>
#include "../../src/cmdline.h"
#include <recon.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"state tail %d: %s\n",__LINE__,#x); return 0; } } while(0)
extern unsigned short AreaSize;
extern unsigned char OvlLevel;
extern int TempFile;
extern unsigned long TmpFSize;
int state_tail_unit( void ) { AreaSize=37;ResetOvlSupp();return AreaSize==0xffff; }
static int reset_fields_and_owners( void )
{
    struct { unsigned char before;segnode node;unsigned char after; } node;
    symbol sym,expected;
    unsigned char bytes[8];
    memset(&node,0x57,sizeof(node));memset(&sym,0x69,sizeof(sym));expected=sym;
    memset(bytes,0xa5,sizeof(bytes));CurrNode=&node.node;LastSym=&sym;
    ObjBuff=bytes;EOObjRec=bytes+sizeof(bytes);Extension=12;OvlLevel=17;AreaSize=0;
    SetDosFmt();CHECK(Extension==4 && OvlLevel==17 && AreaSize==0);
    ResetOvlSupp();CHECK(AreaSize==0xffff && OvlLevel==17 && Extension==4);
    ResetObjOMF();CHECK(CurrNode==0 && ObjBuff==bytes && EOObjRec==bytes+sizeof(bytes));
    ResetSym();CHECK(LastSym==0 && memcmp(&sym,&expected,sizeof(sym))==0);
    CHECK(node.before==0x57 && node.after==0x57);
    CHECK(memcmp(&node.node, "WWWWWWWWWWWWWWWWWWWW",sizeof(node.node))==0);
    ResetObjOMF();ResetSym();ResetOvlSupp();SetDosFmt();
    CHECK(CurrNode==0 && LastSym==0 && AreaSize==0xffff && Extension==4);
    ObjBuff=EOObjRec=0;return 1;
}
static int spill_zero_owned_range( void )
{
    unsigned char bytes[80],actual[81];unsigned i;unsigned long token;
    int file;
    memset(bytes,0x73,sizeof(bytes));TempFile=QOpenRW("zero-spill.tmp");CHECK(TempFile>=0);
    file=TempFile;TmpFSize=0;token=SpillAlloc(sizeof(bytes));CHECK(token==1 && TmpFSize==80);
    SpillWrite(token,0,bytes,sizeof(bytes));
    SpillNull(token,7,35);CHECK(QPos(file)==42 && TempFile==file && TmpFSize==80);
    memset(actual,0xa5,sizeof(actual));SpillRead(token,0,actual,sizeof(bytes));
    for(i=0;i<sizeof(bytes);++i)CHECK(actual[i]==(i>=7 && i<42?0:0x73));
    CHECK(actual[80]==0xa5 && QPos(file)==80);
    SpillNull(token,17,0);CHECK(QPos(file)==17 && TmpFSize==80);
    memset(actual,0xa5,sizeof(actual));SpillRead(token,0,actual,sizeof(bytes));
    for(i=0;i<sizeof(bytes);++i)CHECK(actual[i]==(i>=7 && i<42?0:0x73));
    CHECK(actual[80]==0xa5);QClose(file,"zero-spill.tmp");TempFile=-1;QDelete("zero-spill.tmp");return 1;
}
const ReconTest state_tail_tests[]={
 {"exact-reset-fields-and-preserved-borrowed-owners",1,reset_fields_and_owners},
 {"real-spill-subrange-zero-cursor-and-tail",1,spill_zero_owned_range}
};
