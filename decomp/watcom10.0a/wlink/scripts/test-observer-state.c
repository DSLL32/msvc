/* Native, authored debugger-state controls. No Watcom executable is run. */
#include <stdio.h>
#include <string.h>
#include "observer-state.h"
#include <recon.h>
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"failed line %u\n",__LINE__); return 1; } } while (0)
int main(void)
{
    ObStep s, fresh;
    ObRegs before, armed, after, wrong;
    unsigned i;
    memset(&s,0,sizeof(s)); memset(&before,0,sizeof(before));
    before.eip=0x410101; before.esp=0x120000; before.flags=0x10246;
    before.eax=1; before.ebx=2; before.ecx=3; before.edx=4;
    before.esi=5; before.edi=6; before.ebp=7;
    before.cs=0x23; before.ss=0x2b; before.ds=0x2b;
    before.es=0x2b; before.fs=0x53; before.gs=0x63;
    CHECK(ob_prepare(&s,11,0x410100,0x70001000,&before)==OB_OK);
    fresh=s; CHECK(ob_prepare(&s,11,0x410100,0x70001000,&before)==OB_ACTIVE);
    armed=before; armed.eip=0x410100; armed.flags|=0x100;
    CHECK(ob_armed(&s,12,&armed)==OB_OWNER);
    wrong=armed; wrong.flags&=~0x100U;
    CHECK(ob_armed(&s,11,&wrong)==OB_FLAGS);
    CHECK(ob_armed(&s,11,&armed)==OB_OK);
    after=before; after.eip=0x70001000;
    CHECK(ob_finish(&fresh,11,&after)==OB_UNARMED);
    CHECK(ob_finish(&s,11,&after)==OB_OK);
    CHECK(ob_finish(&s,12,&after)==OB_OWNER);
    wrong=after; wrong.eip++; CHECK(ob_finish(&s,11,&wrong)==OB_TARGET);
    wrong=after; wrong.esp-=4; CHECK(ob_finish(&s,11,&wrong)==OB_STACK);
    wrong=after; wrong.flags|=0x100; CHECK(ob_finish(&s,11,&wrong)==OB_FLAGS);
    wrong=after; wrong.flags^=0x10000; CHECK(ob_finish(&s,11,&wrong)==OB_FLAGS);
    for (i=0;i<7;++i) {
        wrong=after;
        switch(i) { case 0:wrong.eax++;break;case 1:wrong.ebx++;break;
        case 2:wrong.ecx++;break;case 3:wrong.edx++;break;case 4:wrong.esi++;break;
        case 5:wrong.edi++;break;default:wrong.ebp++; }
        CHECK(ob_finish(&s,11,&wrong)==OB_CONTEXT);
    }
    for (i=0;i<6;++i) {
        wrong=after;
        switch(i) { case 0:wrong.cs++;break;case 1:wrong.ss++;break;
        case 2:wrong.ds++;break;case 3:wrong.es++;break;case 4:wrong.fs++;break;
        default:wrong.gs++; }
        CHECK(ob_finish(&s,11,&wrong)==OB_SEGMENTS);
    }
    CHECK(ob_clear(&s,11,&after)==OB_OK); CHECK(!s.active && !s.armed);
    CHECK(ob_return_stack(0x120000,5,0x120018)==OB_OK);
    CHECK(ob_return_stack(0x120000,5,0x120014)==OB_STACK);
    CHECK(ob_return_stack(0xfffffff0U,5,8)==OB_OVERFLOW);
    CHECK(ob_return_stack(0x120000,8,0x120024)==OB_CONTEXT);
    printf("%u observer-state controls passed\n",checks);
    return 0;
}
