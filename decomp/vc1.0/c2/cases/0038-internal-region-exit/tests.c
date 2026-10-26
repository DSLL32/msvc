/* Cold, fully initialized controls supplement actual DIRECT edge/alias reach.
   These do not claim public producer reach or a real decoder jump-table path. */
#include "machine-analysis.h"
#include <recon-tool.h>
#include <string.h>
#include <recon.h>
static void symbol(LabelSymbol *symbol,MachineInstruction *instruction)
{
 memset(symbol,0,sizeof(*symbol));memset(instruction,0,sizeof(*instruction));
 symbol->symbol.kind=3;symbol->instruction=instruction;instruction->opcode=1;
 instruction->record.symbol=&symbol->symbol;
}
static int direct_address_and_alias(void)
{
 LabelSymbol old,replacement;MachineInstruction old_record,new_record,list[5];
 MachineAddress address;MachineFlowRegion attached;unsigned i;
 symbol(&old,&old_record);symbol(&replacement,&new_record);
 memset(list,0,sizeof(list));memset(&address,0,sizeof(address));memset(&attached,0,sizeof(attached));
 for(i=0;i<4;++i)list[i].next=&list[i+1];
 list[0].opcode=MC_JCC<<8;list[0].operands[0].flags=8;list[0].operands[0].value.symbol=&old.symbol;
 list[1].opcode=MC_JMP<<8;list[1].operands[0].flags=3;list[1].operands[0].value.address=&address;
 address.symbol=&old.symbol;
 list[2].opcode=MC_MOV<<8;list[2].operands[0].flags=8;list[2].operands[0].value.symbol=&old.symbol;
 list[3].opcode=1;list[3].labels.attachment=&attached;attached.flags=0x10;attached.exit_label=&old_record;
 list[4].opcode=MC_JMP<<8;list[4].operands[0].flags=8;list[4].operands[0].value.symbol=&old.symbol;
 if(retarget_machine_region_entry(&old,&replacement,list,&list[4])!=2||
  list[0].operands[0].value.symbol!=&replacement.symbol||address.symbol!=&replacement.symbol||
  list[2].operands[0].value.symbol!=&old.symbol||list[4].operands[0].value.symbol!=&old.symbol||
  attached.exit_label!=&new_record||old.references||replacement.references)return 0;
 return retarget_machine_region_entry(&old,&replacement,list,&list[4])==0&&
  retarget_machine_region_entry(&old,&replacement,list,list)==0;
}
static int table_destinations(void)
{
 LabelSymbol old,replacement,other,table;MachineInstruction a,b,c,table_head;
 MachineInstruction branch,end,data[3];MachineAddress first,second;
 symbol(&old,&a);symbol(&replacement,&b);symbol(&other,&c);symbol(&table,&table_head);
 memset(&branch,0,sizeof(branch));memset(&end,0,sizeof(end));memset(data,0,sizeof(data));
 memset(&first,0,sizeof(first));memset(&second,0,sizeof(second));
 table.symbol.field0d=1;table_head.next=data;
 branch.opcode=MC_JMP<<8;branch.operands[0].flags=8;branch.operands[0].value.symbol=&table.symbol;branch.next=&end;
 data[0].opcode=data[1].opcode=MC_DATA<<8;data[0].next=&data[1];data[1].next=&data[2];data[2].opcode=1;
 data[0].operands[0].flags=data[1].operands[0].flags=3;
 data[0].operands[0].value.address=&first;data[1].operands[0].value.address=&second;
 first.symbol=&old.symbol;second.symbol=&other.symbol;
 return retarget_machine_region_entry(&old,&replacement,&branch,&end)==1&&
  first.symbol==&replacement.symbol&&second.symbol==&other.symbol&&
  branch.operands[0].value.symbol==&table.symbol&&!old.references&&!replacement.references;
}
const ReconTest region_exit_tests[]={
 {"cold direct/address edges, ignored opcode, owner alias and half-open bounds",1,direct_address_and_alias},
 {"cold consecutive DATA table references preserve unrelated labels",1,table_destinations}
};
