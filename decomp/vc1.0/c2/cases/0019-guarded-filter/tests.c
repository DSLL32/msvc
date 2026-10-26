#include <recon-tool.h>
#include <recon-native.h>
#include <string.h>
#include "float-peepholes.h"
#include "instruction-selection.h"
#include "scheduler.h"
#include "float-machine.h"
#include "jump-threading.h"
#include <recon.h>
extern int main(int,char **,char **);
extern int backend_stage;
static void real_input(void *unused)
{
 char *args[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};
 (void)unused;main(1,args,0);
}
static MachineInstruction *__fastcall store_entry(MachineInstruction *instruction)
{
 MachineInstruction before,marker_before,next_before,*marker,*next,*result;
 MachineAddress *address=instruction->operands[0].value.address;
 int valid=instruction->opcode==0x16280&&backend_stage==6&&instruction->operands[0].flags==4&&
   address&&address->first.displacement==0&&address->symbol&&address->base.value==5&&
   !address->index.value&&instruction->operands[1].flags==1&&instruction->operands[1].value.reg.value==14;
 if(!valid)recon_native_entry_finish(0);
 marker=instruction->next;next=marker?marker->next:0;
 if(!marker||marker->opcode!=4||marker->record.source_location!=23||!next||
   next->opcode!=0x4680||next->operands[1].flags!=1||next->operands[1].value.reg.value!=14)
  recon_native_entry_finish(0);
 before=*instruction;marker_before=*marker;next_before=*next;
 result=rewrite_pentium_float_store(instruction);
 recon_native_entry_finish(result==marker&&!memcmp(instruction,&before,sizeof before)&&
   !memcmp(marker,&marker_before,sizeof marker_before)&&!memcmp(next,&next_before,sizeof next_before));
}
static int actual_store_inputs(void)
{
 return recon_native_at_entry((const void*)rewrite_pentium_float_store,
   (const void*)store_entry,real_input,0)==1;
}
/* Independent raw ISA/control rows include each six-way relationship and
   the original no-companion exit after FST has already become FSTP. These
   cold streams do not assert that the real first entry transforms anything. */
static int store_controls(void)
{
 static const struct { unsigned first,second,converted,reorder,partial; } rows[]={
  {0x45,0x45,0x45,0,0}, {0x46,0x49,0x46,0,0},
  {0x47,0x45,0x45,1,0}, {0x49,0x46,0x46,1,0},
  {0x48,0x45,0x47,0,0}, {0x4a,0x46,0x49,0,0},
  {0x47,0x47,0,0,1}, {0x49,0x49,0,0,1}
 };
 MachineInstruction stream[4],*result,*added,*first,*last;
 MachineAddress destination,operands[2];SymbolHeader symbol;unsigned i,j;
 for(i=0;i<RECON_COUNT(rows);i++){
  memset(stream,0,sizeof stream);memset(&destination,0,sizeof destination);memset(operands,0,sizeof operands);memset(&symbol,0,sizeof symbol);
  destination.first.displacement=13;destination.symbol=&symbol;destination.base.value=5;
  for(j=0;j<4;j++){if(j)stream[j].previous=&stream[j-1];if(j<3)stream[j].next=&stream[j+1];}
  stream[0].opcode=0x16280;stream[0].operands[0].flags=4;stream[0].operands[0].value.address=&destination;
  stream[0].operands[1]=operand_st0;
  stream[1].opcode=0x20080|(rows[i].first<<8);stream[2].opcode=0x30080|(rows[i].second<<8);
  for(j=0;j<2;j++){stream[j+1].operands[0]=operand_st0;stream[j+1].operands[1].flags=4;stream[j+1].operands[1].value.address=&operands[j];}
  stream[3].opcode=0x800;
  result=rewrite_pentium_float_store(&stream[0]);
  if(stream[0].opcode!=0x16380)return 0;
  if(rows[i].partial){
   if(stream[0].next!=&stream[1]||stream[1].next!=&stream[2]||stream[2].next!=&stream[3]||result!=&stream[2]||
      stream[1].opcode!=(0x20080|(rows[i].first<<8))||stream[2].opcode!=(0x30080|(rows[i].second<<8)))return 0;
   continue;
  }
  first=rows[i].reorder?&stream[2]:&stream[1];last=&stream[2];added=last->next;
  if(stream[0].next!=first||first->previous!=&stream[0]||
     first->opcode!=((rows[i].reorder?0x30080:0x20080)|0x6000)||added==&stream[3]||
     added->opcode!=(0x80|(rows[i].converted<<8))||memcmp(&added->operands[0],&operand_st0,sizeof(MachineOperand))||
     added->operands[1].flags!=4||added->operands[1].value.address==&destination||
     memcmp(added->operands[1].value.address,&destination,sizeof destination))return 0;
  if(rows[i].reorder){
   if(first->next!=added||added->next!=&stream[1]||stream[1].next!=&stream[3]||result!=&stream[1])return 0;
  }else if(first->next!=&stream[2]||added->next!=&stream[3]||result!=added)return 0;
  if(added->previous!=last||added->next->previous!=added||stream[3].previous!=result)return 0;
 }
 return 1;
}
/* Bounded read-only original observation at the genuine newly reached child
   binds these defined fields and aliases, not allocator addresses or padding. */
static int __fastcall inherited_use_entry(FloatStreamUse *use)
{
 MemoryMotionContext *context=memory_motion_context,*successor=context->next;
 MachineOperand *operand=use->operand;MachineAddress *address=operand->value.address;
 int valid=context->available_registers==7&&!context->uses&&!context->last_use&&!context->reverse_uses&&
  context->order==1&&context->maximum_depth==1&&successor&&successor->previous==context&&
  !successor->next&&successor->uses==use&&successor->last_use==successor->reverse_uses&&
  successor->available_registers==-1&&successor->order==2&&successor->maximum_depth==2&&
  !use->origin_symbol&&use->weighted_uses==12&&use->stack_slot==-1&&use->base==5&&use->index==0xffff&&
  use->width==8&&use->operation==0x60&&use->hash_next&&!use->previous_selected&&
  use->stores==2&&!use->multiplies&&use->arithmetic==3&&use->loads==1&&use->cost_adjustment==2&&
  !use->branch&&use->flags==0x74&&operand->flags==4&&address&&!address->first.displacement&&
  address->symbol&&address->base.value==5&&!address->index.value;
 recon_native_entry_finish(valid);
}
static int actual_inherited_use(void)
{
 return recon_native_at_entry((const void*)can_inherit_float_motion_use,
   (const void*)inherited_use_entry,real_input,0)==1;
}
static int no_inherited_capacity(void)
{
 MemoryMotionContext context,successor;FloatStreamUse use;
 memset(&context,0,sizeof context);memset(&successor,0,sizeof successor);memset(&use,0,sizeof use);
 context.next=&successor;context.available_registers=-1;successor.uses=&use;memory_motion_context=&context;
 return inherit_float_motion_uses()==0&&successor.uses==&use&&!context.uses&&!context.last_use&&!context.reverse_uses;
}
static int __fastcall range_entry(MachineInstruction *first,MachineInstruction *last,MachineOperand *operand,unsigned width)
{
 MemoryMotionContext *context=memory_motion_context;FloatStreamUse *use=context->next->uses;
 int valid=first==context->boundary&&last==context->next->boundary&&operand==use->operand&&width==8&&
  first!=last&&first->opcode==1&&last->opcode==1&&first->record.symbol==&context->label->symbol&&
  last->record.symbol==&context->next->label->symbol&&first->next&&last->previous&&
  operand->flags==4&&operand->value.address->base.value==5;
 recon_native_entry_finish(valid);
}
static int actual_range_inputs(void)
{
 return recon_native_at_entry((const void*)float_operand_changed_between,(const void*)range_entry,real_input,0)==1;
}
static int inherit_eligibility_controls(void)
{
 FloatStreamUse use,*registers[8];MachineOperand operand;MachineAddress address;BackendSymbol symbol;
 memset(&use,0,sizeof use);memset(registers,0,sizeof registers);memset(&operand,0,sizeof operand);
 memset(&address,0,sizeof address);memset(&symbol,0,sizeof symbol);
 use.operand=&operand;operand.value.address=&address;address.symbol=&symbol.symbol;operand.flags=3;
 symbol.attributes=0x40;if(!can_inherit_float_motion_use(&use))return 0;
 symbol.attributes=0;operand.flags=4;address.base.value=5;registers[5]=FLOAT_STREAM_FIXED_REGISTER;float_stream_register_uses=registers;
 if(can_inherit_float_motion_use(&use))return 0;
 operand.flags=5;address.index.value=3;registers[3]=FLOAT_STREAM_FIXED_REGISTER;
 if(can_inherit_float_motion_use(&use))return 0;
 registers[3]=0;if(can_inherit_float_motion_use(&use))return 0;
 return 1;
}
static int range_controls(void)
{
 MachineInstruction stream[4];MachineOperand operand;MachineAddress address,other;BackendSymbol symbol;unsigned i;
 memset(stream,0,sizeof stream);memset(&operand,0,sizeof operand);memset(&address,0,sizeof address);
 memset(&other,0,sizeof other);memset(&symbol,0,sizeof symbol);
 address.symbol=other.symbol=&symbol.symbol;address.base.value=other.base.value=5;other.first.displacement=32;
 for(i=0;i<3;i++)stream[i].next=&stream[i+1];
 operand.flags=4;operand.value.address=&address;
 stream[1].opcode=0x1200|0x40;stream[1].operands[0]=operand; /* LEA is ignored. */
 stream[2].opcode=0x100|0x40;stream[2].operands[0].flags=4;stream[2].operands[0].value.address=&other;
 stream[2].operands[1]=operand;
 if(!float_operand_changed_between(stream,&stream[3],&operand,4))return 0;
 stream[2].operands[1]=stream[2].operands[0];
 if(float_operand_changed_between(stream,&stream[3],&operand,4))return 0;
 stream[2].operands[0]=operand;
 if(!float_operand_changed_between(stream,&stream[3],&operand,4))return 0;
 stream[2].opcode|=4; /* Noninstruction records are skipped. */
 if(float_operand_changed_between(stream,&stream[3],&operand,4))return 0;
 operand.flags=2; /* Unsupported operand kinds return without walking. */
 return !float_operand_changed_between(stream,&stream[3],&operand,4);
}
static int inherited_list_controls(void)
{
 MemoryMotionContext context,successor;FloatStreamUse uses[3];MachineOperand operands[3];MachineAddress addresses[3];BackendSymbol symbols[3];
 unsigned i;memset(&context,0,sizeof context);memset(&successor,0,sizeof successor);
 memset(uses,0,sizeof uses);memset(operands,0,sizeof operands);memset(addresses,0,sizeof addresses);memset(symbols,0,sizeof symbols);
 for(i=0;i<3;i++){
  uses[i].operand=&operands[i];operands[i].flags=3;operands[i].value.address=&addresses[i];addresses[i].symbol=&symbols[i].symbol;symbols[i].attributes=0x40;
  if(i<2)uses[i].hash_next=&uses[i+1];if(i)uses[i].previous_selected=&uses[i-1];
 }
 context.next=&successor;context.available_registers=0;successor.uses=uses;successor.last_use=successor.reverse_uses=&uses[2];memory_motion_context=&context;
 if(inherit_float_motion_uses()!=1||context.uses!=uses||context.last_use!=uses||context.reverse_uses!=uses||uses[0].hash_next||uses[0].previous_selected||
  successor.uses!=&uses[1]||uses[1].previous_selected)return 0;
 context.available_registers=2;
 if(inherit_float_motion_uses()!=2||context.uses!=uses||context.last_use!=&uses[2]||context.reverse_uses!=&uses[2]||
  uses[0].hash_next!=&uses[1]||uses[1].hash_next!=&uses[2]||uses[2].hash_next||uses[2].previous_selected!=&uses[1]||
  uses[1].previous_selected!=uses||successor.uses||successor.reverse_uses)return 0;
 return 1;
}
static int __fastcall diamond_entry(unsigned short number,MachineInstruction *first,MachineInstruction *last,unsigned bytes)
{
 MachineInstruction before;int valid=number==7&&bytes==4&&first->opcode==1&&first->next==last&&
  last->previous==first&&last->opcode==0x40140&&last->operands[0].flags==1&&last->operands[0].value.reg.value==7&&
  last->operands[1].flags==4&&last->next&&last->next->opcode==7;
 if(!valid)recon_native_entry_finish(0);before=*last;
 recon_native_entry_finish(diamond_register_constraint(number,first,last,bytes)==0&&!memcmp(last,&before,sizeof before));
}
static int actual_diamond_input(void)
{
 return recon_native_at_entry((const void*)diamond_register_constraint,(const void*)diamond_entry,real_input,0)==1;
}
static void __fastcall protected_entry(MachineInstruction *instruction)
{
 MachineBlockHeader *block=(MachineBlockHeader*)instruction;MemoryMotionContext *context=float_stream_context;
 int valid=instruction->opcode==7&&instruction->next&&instruction->previous&&block->uses&&block->definitions&&
  block->number==2&&block->field18==0&&block->flags==0x201&&context&&!context->previous&&!context->next&&!context->label&&
  !context->first_load&&!context->last_load&&!context->first_store&&!context->rewrite_end&&context->boundary&&context->boundary!=instruction&&
  !context->last&&!context->uses&&!context->branches&&context->maximum_depth==1&&context->flags==0&&context->order==0;
 recon_native_entry_finish(valid);
}
static int actual_protected_input(void)
{
 return recon_native_at_entry((const void*)process_float_stream_protected_block,(const void*)protected_entry,real_input,0)==1;
}
static int diamond_controls(void)
{
 MachineInstruction stream[3];MachineAddress address;memset(stream,0,sizeof stream);memset(&address,0,sizeof address);
 stream[0].next=&stream[1];stream[1].next=&stream[2];stream[1].opcode=0x140;
 stream[1].operands[0].flags=1;stream[1].operands[0].value.reg.value=7;stream[1].operands[1].flags=2;
 if(diamond_register_constraint(7,stream,&stream[1],4))return 0;
 stream[1].operands[0].value.reg.value=6;stream[1].operands[1].flags=1;stream[1].operands[1].value.reg.value=7;
 if(!diamond_register_constraint(7,stream,&stream[1],4))return 0;
 stream[1].operands[1].flags=2;address.base.value=7;stream[1].operands[0].flags=4;stream[1].operands[0].value.address=&address;
 if(!diamond_register_constraint(7,stream,&stream[1],4))return 0;
 stream[1].opcode=0x120;if(!diamond_register_constraint(7,stream,&stream[1],4))return 0;
 return diamond_register_constraint(7,stream,stream,4)==1;
}
static MachineInstruction *__fastcall protected_skip_entry(MachineInstruction *instruction)
{
 MachineBlockHeader *block=(MachineBlockHeader*)instruction;
 /*393a3..393ae forwards the unchanged input in ESI to this exact child. */
 recon_native_entry_finish(instruction->opcode==7&&block->number==2&&block->flags==0x201&&
  block->uses&&block->definitions&&instruction->next&&instruction->previous);
}
static int actual_protected_skip_input(void)
{
 return recon_native_at_entry((const void*)next_float_context_after_protected_block,(const void*)protected_skip_entry,real_input,0)==1;
}
static int protected_skip_controls(void)
{
 MachineBlockHeader blocks[4];unsigned i;memset(blocks,0,sizeof blocks);
 for(i=0;i<3;i++)blocks[i].next=(MachineInstruction*)&blocks[i+1];
 blocks[1].opcode=4;blocks[2].opcode=7;blocks[2].flags=0x201;blocks[3].opcode=7;blocks[3].flags=0x200;
 if(next_float_context_after_protected_block((MachineInstruction*)blocks)!=(MachineInstruction*)&blocks[3]||
  float_stream_next!=(MachineInstruction*)&blocks[3])return 0;
 blocks[3].flags|=1;
 return !next_float_context_after_protected_block((MachineInstruction*)blocks)&&!float_stream_next;
}
const ReconTest guarded_tests[]={
 {"actual-Pentium-FST-input-and-unchanged-exit",1,actual_store_inputs},
 {"Pentium-FST-grouping-and-reordering",1,store_controls},
 {"actual-inherited-floating-use-and-owner",1,actual_inherited_use},
 {"negative-capacity-retains-successor-use",1,no_inherited_capacity},
 {"actual-range-scanner-register-stack-and-owner-inputs",1,actual_range_inputs},
 {"inherited-use-constant-and-address-register-controls",1,inherit_eligibility_controls},
 {"floating-operand-range-endpoints-LEA-and-alias-controls",1,range_controls},
 {"successor-floating-use-list-identities-and-inclusive-capacity",1,inherited_list_controls},
 {"actual-diamond-register-width-and-boundaries",1,actual_diamond_input},
 {"actual-protected-floating-context-and-block",1,actual_protected_input},
 {"diamond-register-read-overwrite-address-and-width",1,diamond_controls},
 {"actual-protected-skip-child-input-identity",1,actual_protected_skip_input},
 {"protected-skip-nested-marker-and-terminal-cursor",1,protected_skip_controls}
};
