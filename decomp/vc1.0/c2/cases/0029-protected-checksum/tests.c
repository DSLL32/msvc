#include "case.h"
#include <recon-native.h>
#include "unoptimized.h"
#include "temporaries.h"
#include "exception-scope.h"
#include "physical-rewrite.h"
#include "machine-analysis.h"
#include "instruction-rewrite.h"
#include <string.h>
#include <recon.h>
extern int main(int,char **,char **);
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};
 (void)unused;main(1,argv,0);
}
/* Genuine first spill, independently observed in the original: requested8188,
   candidatesCF, victim8022, physicalEAX0. Borrow the actual machine chain;
   check the two added owners, shared temporary identity and defined operands. */
static unsigned __fastcall spill_entry(MachineInstruction *instruction,unsigned reg)
{
 MachineInstruction *old[8192],*scan,*added[2];unsigned count=0,n=0,i,physical;
 MachineInstruction *store,*reload;TemporaryRegister *temporary;
 static const unsigned mapped[8]={0x8022,0x80da,0x8136,0x8162,0x1a,0x1a,0x8179,0x8184};
 if(reg!=0x8188||instruction->opcode!=0x70140||instruction->operands[0].flags!=1||
  instruction->operands[0].value.reg.words.number!=0x8188||instruction->operands[1].flags!=4)
  recon_native_entry_finish(0);
 for(i=0;i<8;++i)if(unoptimized_registers[i].virtual_register!=mapped[i])recon_native_entry_finish(0);
 if(lowering_spill_candidates(reg)!=0xcf||choose_lowering_spill(instruction,0xcf)!=0x8022||
  require_lowering_register(0x8022)!=0||lookup_lowering_register(0x8fff)!=0x1a||
  lowering_register_is_candidate(REG_EAX,0xcf)||lowering_register_is_candidate(0x8fff,0xcf)||
  !lowering_register_is_candidate(0x8184,0x80)||lowering_register_is_candidate(0x8184,1))
  recon_native_entry_finish(0);
 for(scan=machine_instructions;scan;scan=scan->next){
  if(count==8192)recon_native_entry_finish(0);old[count++]=scan;
 }
 physical=spill_lowering_register(instruction,reg);
 for(scan=machine_instructions;scan;scan=scan->next){
  for(i=0;i<count&&old[i]!=scan;++i){}
  if(i==count){if(n==2)recon_native_entry_finish(0);added[n++]=scan;}
 }
 if(physical||n!=2)recon_native_entry_finish(0);
 store=added[0];reload=added[1];
 if(store->opcode!=0x140||reload->opcode!=0x140||store->operands[0].flags!=4||
  store->operands[1].flags!=0x401||store->operands[1].value.reg.words.number!=REG_EAX||
  reload->operands[0].flags!=1||reload->operands[0].value.reg.words.number!=0x8022||
  reload->operands[1].flags!=4)recon_native_entry_finish(0);
 temporary=(TemporaryRegister*)store->operands[0].value.address->symbol;
 recon_native_entry_finish(temporary&&temporary->kind==19&&temporary->bytes==4&&
  store->operands[0].value.address!=reload->operands[1].value.address&&
  reload->operands[1].value.address->symbol==(SymbolHeader*)temporary&&
  store->operands[0].value.address->base.words.number==REG_EBP&&
  reload->operands[1].value.address->base.words.number==REG_EBP&&
  !store->operands[0].value.address->first.displacement&&
  !reload->operands[1].value.address->first.displacement&&
  store->next->previous==store&&store->previous->next==store&&
  reload->next->previous==reload&&reload->previous->next==reload&&
  unoptimized_registers[0].virtual_register==0x8022);
 return 0;
}
static int actual_spill(void)
{
 return recon_native_at_entry((const void*)spill_lowering_register,
  (const void*)spill_entry,real_input,0)==1;
}
/* Actual except-only constructor target is-1; check the generated one-argument
   scope-state call and list terminator, independently of its later selection. */
static Node *__fastcall unwind_entry(long target)
{
 Node *node;
 if(target!=-1)recon_native_entry_finish(0);
 node=new_scope_unwind(target);
 recon_native_entry_finish(node->opcode==0x40&&node->attributes.words.type==0x104&&
  node->left.node->opcode==0x33&&node->left.node->left.value==0xf4&&
  node->second.node->opcode==0x55&&node->second.node->left.node->opcode==0x33&&
  node->second.node->left.node->left.value==0xffffffffU&&
  node->second.node->second.node->opcode==0x4c);
 return 0;
}
static int actual_unwind(void)
{
 return recon_native_at_entry((const void*)new_scope_unwind,
  (const void*)unwind_entry,real_input,0)==1;
}
/* These dormant flow-maintenance owners are native UNIT-qualified only;
   the cached public protected-checksum oracle/transport is unchanged. */
short __fastcall next_numbered_work(WorkItem *work);
int __fastcall redirect_assembly_label(Node *node,SymbolHeader *old_label,SymbolHeader *new_label);
static int numbered_successor(void)
{
 WorkItem a={0},b={0},c={0};a.next=&b;b.next=&c;a.number=-3;b.number=-1;c.number=12;
 if(next_numbered_work(0)!=0||next_numbered_work(&a)!=12)return 0;
 c.number=0;if(next_numbered_work(&a)!=0)return 0;
 c.number=-2;if(next_numbered_work(&a)!=0)return 0;
 a.number=7;return next_numbered_work(&a)==7&&a.next==&b&&b.next==&c;
}
static int detached_exception_label(void)
{
 Function function={0};ExceptionScope scopes[3]={{0}},expected[3];
 SymbolHeader removed={0},retained={0};unsigned i;
 for(i=0;i<3;++i){scopes[i].identifier=i+1;scopes[i].kind=1;scopes[i].flags=0x12+i;}
 scopes[0].next=&scopes[1];scopes[1].next=&scopes[2];function.exception_scopes=scopes;
 scopes[0].handler_label=scopes[0].filter_label=scopes[0].alternate_label=&removed;
 scopes[1].handler_label=&retained;scopes[1].filter_label=&removed;
 scopes[2].alternate_label=&retained;
 memcpy(expected,scopes,sizeof(scopes));expected[0].handler_label=0;expected[0].filter_label=0;
 expected[0].alternate_label=0;expected[1].filter_label=0;
 forget_exception_label(&function,&removed);
 return function.exception_scopes==scopes&&!memcmp(scopes,expected,sizeof(scopes));
}
static int split_region_descendants(void)
{
 MachineFlowRegion root={0},left={0},right={0},grandchild={0};
 root.children=&left;left.next=&right;left.parent=right.parent=&root;
 left.children=&grandchild;grandchild.parent=&left;
 root.flags=4;left.flags=1;right.flags=8;grandchild.flags=0x40;
 root.saved_split_register=17;left.saved_split_register=18;right.saved_split_register=19;
 mark_machine_region_split(&root,6,0x8022,0,0);
 return root.flags==0x24&&left.flags==0x21&&right.flags==0x28&&grandchild.flags==0x60&&
  root.split_color==6&&left.split_color==6&&right.split_color==6&&grandchild.split_color==6&&
  root.saved_split_register==17&&left.saved_split_register==18&&right.saved_split_register==19&&
  root.children==&left&&left.next==&right&&left.children==&grandchild&&grandchild.parent==&left;
}
static int assembly_destination_identity(void)
{
 static const unsigned operations[]={MC_MOV,MC_LEA,MC_JCC,MC_JMP};
 SymbolHeader old_label={0},new_label={0},other={0};
 MachineOperand reg,operand;MachineInstruction *instruction;Node node={0};
 unsigned i,match,index,kind;
 old_label.kind=new_label.kind=other.kind=3;
 initialize_operand(&reg,OPERAND_REGISTER,REG_EAX,0,0);
 for(i=0;i<4;++i)for(match=0;match<2;++match){
  kind=i<2?OPERAND_ABSOLUTE:OPERAND_SYMBOL;index=i<2?1:0;
  initialize_operand(&operand,kind,0,12,match?&old_label:&other);
  instruction=i<2?new_machine_record(0,operations[i],4,&reg,&operand):
   new_machine_record(0,operations[i],4,&operand,0);
  node.left.pointer=instruction;
  if(redirect_assembly_label(&node,&old_label,&new_label)!=(int)match)return 0;
  if(index){if(instruction->operands[1].value.address->symbol!=(match?&new_label:&other)||
    instruction->operands[1].value.address->first.displacement!=12||
    instruction->operands[0].value.reg.words.number!=REG_EAX)return 0;}
  else if(instruction->operands[0].value.symbol!=(match?&new_label:&other))return 0;
 }
 initialize_operand(&operand,OPERAND_BASED,REG_EAX,0,0);
 instruction=new_machine_record(0,MC_JMP,4,&operand);node.left.pointer=instruction;
 if(!redirect_assembly_label(&node,&old_label,&new_label)||
  instruction->operands[0].value.address!=operand.value.address)return 0;
 instruction=new_machine_record(0,MC_ADD,4,&reg,&reg);node.left.pointer=instruction;
 return !redirect_assembly_label(&node,&old_label,&new_label);
}
const ReconTest checksum_tests[]={
 {"actual legacy spill owners and mapping helpers",1,actual_spill},
 {"actual except-only scope-state call",1,actual_unwind},
 {"unit-qualified numbered flow successor skips inserted owners",1,numbered_successor},
 {"unit-qualified detached SEH label clears all matching roles",1,detached_exception_label},
 {"unit-qualified split color propagates without repeating range updates",1,split_region_descendants},
 {"unit-qualified inline assembly label retargeting preserves identity",1,assembly_destination_identity}
};
