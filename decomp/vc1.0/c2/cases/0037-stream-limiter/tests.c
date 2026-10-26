#include <recon-tool.h>
#include <recon-native.h>
#include "float-machine.h"
#include "functions.h"
#include <recon.h>
extern int main(int,char **,char **);
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);
}
/* Actual first split entry: conditional branch, old target reference count2,
   context depth0 and five resident width8 owners. Check the newly owned edge,
   label and five exit stores, preserving original target/owner identities.
   This unit borrows real input, not a manufactured compiler graph. */
static void __fastcall split_entry(MachineInstruction *instruction,MemoryMotionContext *context)
{
 LabelSymbol *old_symbol=(LabelSymbol *)instruction->operands[0].value.symbol,*new_symbol;
 MachineInstruction *target=old_symbol->instruction,*previous,*jump,*label,*scan;
 FloatStreamUse *use,*owners[5];int slots[5];unsigned flags[5],i,count=0,opcode=instruction->opcode;
 if(opcode!=0xf40||(instruction->operands[0].flags&15)!=8||!target||
  old_symbol->references!=2||context->stack_depth||context->maximum_depth!=3)
  recon_native_entry_finish(0);
 previous=target->previous;
 for(use=float_stream_uses;use;use=use->hash_next){
  if(count==5||use->width!=8)recon_native_entry_finish(0);
  owners[count]=use;slots[count]=use->stack_slot;flags[count]=use->flags;++count;
 }
 if(count!=5)recon_native_entry_finish(0);
 split_float_motion_branch(instruction,context);
 jump=previous->next;label=jump->next;new_symbol=(LabelSymbol *)instruction->operands[0].value.symbol;
 if(instruction->opcode!=opcode||new_symbol==old_symbol||new_symbol->references!=1||
  jump->opcode!=((opcode&~0xff00U)|(MC_JMP<<8))||
  jump->operands[0].value.symbol!=&old_symbol->symbol||
  (label->opcode&15)!=1||label->record.symbol!=&new_symbol->symbol||
  old_symbol->references!=2||old_symbol->instruction!=target||
  jump->previous!=previous||label->previous!=jump)recon_native_entry_finish(0);
 scan=label->next;count=0;
 for(use=float_stream_uses;use;use=use->hash_next){
  if(count==5||owners[count]!=use||slots[count]!=use->stack_slot||flags[count]!=use->flags||
   use->width!=8||scan==target||(scan->opcode&15)!=0||
   ((scan->opcode>>8)&255)!=MC_FSTP||((scan->opcode>>4)&15)!=8||
   scan->previous->next!=scan||scan->next->previous!=scan)recon_native_entry_finish(0);
  scan=scan->next;++count;
 }
 i=count==5&&scan==target&&context->stack_depth==0;
 recon_native_entry_finish(i!=0);
}
static int actual_split(void)
{return recon_native_at_entry((const void*)split_float_motion_branch,(const void*)split_entry,real_input,0)==1;}
const ReconTest limiter_tests[]={
 {"actual epilogue branch clone label and five resident stores",1,actual_split}
};
