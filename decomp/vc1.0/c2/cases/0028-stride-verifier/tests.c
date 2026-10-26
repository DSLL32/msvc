#include "case.h"
#include <recon-native.h>
#include "tree-rewrite.h"
#include "comparison-branches.h"
#include "instruction-rewrite.h"
#include <recon.h>
extern int main(int,char **,char **);
Node *__fastcall push_unary_below_conversion(Node *node);
int __cdecl multiplication_overflows(unsigned,unsigned,int,unsigned);
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};
 (void)unused;main(1,argv,0);
}
/* Independent values from original80370..80384, including unused slots. */
static int sign_bits(void)
{
 return integer_sign_bit(0)==0&&integer_sign_bit(1)==0x80&&
  integer_sign_bit(2)==0x8000&&integer_sign_bit(3)==0&&integer_sign_bit(4)==0x80000000U;
}
/* Cold edge controls for the original byte/word OF check. Dword arithmetic
   deliberately returns0 even when a mathematical product exceeds32bits. */
static int product_limits(void)
{
 return !multiplication_overflows(15,17,1,1)&&multiplication_overflows(16,16,1,1)&&
  !multiplication_overflows(0x80,1,0,1)&&multiplication_overflows(0x80,0xff,0,1)&&
  !multiplication_overflows(255,257,1,2)&&multiplication_overflows(256,256,1,2)&&
  !multiplication_overflows(0x8000,1,0,2)&&multiplication_overflows(0x8000,0xffff,0,2)&&
  !multiplication_overflows(0xffffffffU,2,1,4)&&!multiplication_overflows(0x7fffffffU,2,0,4)&&
  multiplication_overflows(16,16,1,0)&&multiplication_overflows(16,16,1,0xffffffffU);
}
/* Genuine first conversion:34/type102 around E/type104 around34/type104.
   The return retains the former child owner and its left becomes the input. */
static Node *__fastcall conversion_entry(Node *node)
{
 Node *child=node->left.node,*result;
 if(node->opcode!=0x34||node->attributes.words.type!=0x102||child->opcode!=0xe||
  child->attributes.words.type!=0x104||child->left.node->opcode!=0x34||
  child->left.node->attributes.words.type!=0x104)recon_native_entry_finish(0);
 result=push_unary_below_conversion(node);
 recon_native_entry_finish(result==child&&result->opcode==0xe&&
  result->attributes.words.type==0x102&&result->left.node==node);return 0;
}
static int conversion_owners(void)
{
 return recon_native_at_entry((const void*)push_unary_below_conversion,
  (const void*)conversion_entry,real_input,0)==1;
}
static int retarget_mode;
/* Borrow the actual branch/target/label owners. Mode0 checks the encountered
   reversed-condition insertion; mode1 is explicitly a cold compatible-branch
   control that changes only the condition nibble before invoking the routine. */
static void __fastcall retarget_entry(MachineInstruction *target,MachineInstruction *branch,
 MachineInstruction *load,int mode)
{
 MachineInstruction *scan=target->next,*head=machine_context->labels.next;
 LabelSymbol *old=(LabelSymbol*)branch->operands[0].value.symbol;
 LabelSymbol *destination=(LabelSymbol*)scan->operands[0].value.symbol,*created;
 long old_count=old->references,destination_count=destination->references;
 if(target->opcode!=0x3040||target->operands[0].value.constant!=3||
  target->operands[1].value.constant!=3||branch->opcode!=0xf40||scan->opcode!=0xf40||
  (branch->operands[1].flags&15)!=4||(scan->operands[1].flags&15)!=5||
  load||mode||old==destination)recon_native_entry_finish(0);
 if(retarget_mode)branch->operands[1].flags=(branch->operands[1].flags&~15U)|5;
 retarget_comparison_branch(target,branch,load,mode);
 if(retarget_mode){
  recon_native_entry_finish(branch->operands[0].value.symbol==&destination->symbol&&
   old->references==old_count-1&&destination->references==destination_count+1&&
   target->next==scan&&machine_context->labels.next==head);return;
 }
 created=(LabelSymbol*)branch->operands[0].value.symbol;
 recon_native_entry_finish(created!=old&&created!=destination&&created->references==1&&
  old->references==old_count-1&&(scan->next->opcode&15)==1&&
  scan->next->record.symbol==&created->symbol&&created->instruction==scan->next&&
  machine_context->labels.next==scan->next&&scan->next->labels.next==head);
}
static int actual_retarget(void)
{
 retarget_mode=0;
 return recon_native_at_entry((const void*)retarget_comparison_branch,
  (const void*)retarget_entry,real_input,0)==1;
}
static int compatible_retarget(void)
{
 retarget_mode=1;
 return recon_native_at_entry((const void*)retarget_comparison_branch,
  (const void*)retarget_entry,real_input,0)==1;
}
const ReconTest stride_tests[]={
 {"integer sign-bit table",1,sign_bits},
 {"byte/word overflow and original dword limit",1,product_limits},
 {"actual conversion owners",1,conversion_owners},
 {"actual reversed branch label insertion",1,actual_retarget},
 {"cold compatible branch target reuse",1,compatible_retarget}
};
