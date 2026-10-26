#include "case.h"
#include <recon-native.h>
#include "unoptimized.h"
#include "temporaries.h"
#include "exception-scope.h"
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
const ReconTest checksum_tests[]={
 {"actual legacy spill owners and mapping helpers",1,actual_spill},
 {"actual except-only scope-state call",1,actual_unwind}
};
