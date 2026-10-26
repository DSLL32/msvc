/* Cold controls complement the actual live-owner DIRECT report. They do not
   claim these constructed shapes occurred in the decoder's public pipeline. */
#include "physical-allocation.h"
#include "scheduler.h"
#include <recon-tool.h>
#include <string.h>
#include <recon.h>
static int conflict_relationships(void)
{
 RegisterVariable v[4],saved[4];RegisterConflict edges[6];unsigned i;
 memset(v,0,sizeof(v));memset(edges,0,sizeof(edges));
 for(i=0;i<4;++i)v[i].number=(unsigned short)(0x4000+i);
 for(i=0;i<3;++i){
  v[i].degree=2;v[i].conflicts=&edges[2*i];
  edges[2*i].variable=&v[(i+1)%3];edges[2*i].next=&edges[2*i+1];
  edges[2*i+1].variable=&v[(i+2)%3];
 }
 memcpy(saved,v,sizeof(v));
 return count_shared_register_conflicts(&v[0],&v[1])==1&&
  count_shared_register_conflicts(&v[0],&v[0])==2&&
  count_shared_register_conflicts(&v[0],&v[3])==0&&!memcmp(v,saved,sizeof(v));
}
static int home_flag_domains(void)
{
 RegisterVariable a,b;memset(&a,0,sizeof(a));memset(&b,0,sizeof(b));
 a.number=0x4000;a.flags=0x100;
 if(register_spill_homes_compatible(&a,&b)||register_spill_homes_compatible(&b,&a))return 0;
 a.flags=0x200;if(register_spill_homes_compatible(&a,&b))return 0;
 a.number=0x8000;if(!register_spill_homes_compatible(&a,&b))return 0;
 a.number=0x300;a.flags=0x4000;
 return register_spill_homes_compatible(&a,&b)!=0;
}
static int flags_and_destinations(void)
{
 MachineInstruction input,skip,operation,jump,label;LabelSymbol symbol;
 memset(&input,0,sizeof(input));memset(&skip,0,sizeof(skip));
 memset(&operation,0,sizeof(operation));memset(&jump,0,sizeof(jump));
 memset(&label,0,sizeof(label));memset(&symbol,0,sizeof(symbol));
 if(pentium_flags_live_after(&input))return 0;
 input.next=&skip;skip.opcode=3;skip.next=&operation;operation.opcode=MC_JCC<<8;
 if(!pentium_flags_live_after(&input))return 0;
 operation.opcode=MC_INT<<8;if(!pentium_flags_live_after(&input))return 0;
 operation.opcode=MC_ADD<<8;if(pentium_flags_live_after(&input))return 0;
 input.next=&jump;jump.opcode=MC_JMP<<8;jump.operands[0].flags=1;
 if(!pentium_flags_live_after(&input))return 0;
 jump.operands[0].flags=8;jump.operands[0].value.symbol=&symbol.symbol;
 if(!pentium_flags_live_after(&input))return 0;
 symbol.instruction=&label;label.opcode=2;
 if(!pentium_flags_live_after(&input))return 0;
 label.opcode=1;label.next=&operation;operation.opcode=MC_JCC<<8;
 if(!pentium_flags_live_after(&input))return 0;
 operation.opcode=MC_ADD<<8;if(pentium_flags_live_after(&input))return 0;
 operation.opcode=MC_JMP<<8;if(!pentium_flags_live_after(&input))return 0;
 label.next=&skip;operation.opcode=MC_ADD<<8;
 return pentium_flags_live_after(&input)==0;
}
const ReconTest allocation_tests[]={
 {"distinct-shared-conflict-and-exclusion",1,conflict_relationships},
 {"spill-home-number-versus-flag-domain",1,home_flag_domains},
 {"flags-reader-priority-and-branch-destination",1,flags_and_destinations}
};
