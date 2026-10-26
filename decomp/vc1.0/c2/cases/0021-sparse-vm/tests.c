#include <recon-tool.h>
#include <recon-native.h>
#include "tree-rewrite.h"
#include "temporaries.h"
#include "work-prepare.h"
#include "post-allocation.h"
#include <string.h>
#include <recon.h>
extern int main(int,char **,char **);
static void real_input(void *unused)
{
 char *arguments[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,arguments,0);
}
static void __fastcall materialize_entry(Node *node)
{
 Node *result;TemporaryRegister *old=temporary_registers;
 if(node->opcode!=0x44||node->attributes.value!=0x104||!node->left.node||!node->second.node||
  node->info.value||node->right.value||node->detail.value)recon_native_entry_finish(0);
 result=materialize_expression(node);
 recon_native_entry_finish(result!=node&&result->opcode==0x32&&result->attributes.value==0x104&&
  result->second.node==node&&result->left.node->opcode==0x26&&result->left.node->attributes.value==0x404&&
  (TemporaryRegister*)result->left.node->right.symbol==temporary_registers&&temporary_registers!=old&&
  temporary_registers->next==old&&temporary_registers->bytes==4&&temporary_registers->kind==19);
}
static int actual_materialization(void)
{
 return recon_native_at_entry((const void*)materialize_expression,(const void*)materialize_entry,real_input,0)==1;
}
static void __fastcall split_entry(Node *ranges,unsigned cases,int count,unsigned span,Node **pivot,int *left_count)
{
 Node *node=ranges;unsigned n=0,last=0;
 int valid=cases==22&&count==22&&span==0x1ffffffc&&pivot&&left_count&&(void*)pivot!=(void*)left_count;
 while(node->opcode!=0x4c){if(node->opcode!=0x3d||node->left.value<=last||node->left.value!=node->detail.value)valid=0;last=node->left.value;node=node->second.node;++n;}
 if(!valid||n!=22||ranges->left.value!=1||last!=0x1ffffffd)recon_native_entry_finish(0);
 split_switch_ranges(ranges,cases,count,span,pivot,left_count);
 node=ranges;for(n=0;n<11;++n)node=node->second.node;
 recon_native_entry_finish(*left_count==11&&*pivot==node&&node->left.value==0x7ffff);
}
static int actual_split_inputs(void)
{
 return recon_native_at_entry((const void*)split_switch_ranges,(const void*)split_entry,real_input,0)==1;
}
static void __fastcall prefix_entry(Node *ranges,int count,unsigned cases)
{
 Node before=*ranges;
 if(count!=22||cases!=22||ranges->opcode!=0x3d||ranges->left.value!=1||ranges->second.node->left.value!=17)recon_native_entry_finish(0);
 recon_native_entry_finish(choose_switch_prefix_count(ranges,count,cases)==23&&!memcmp(ranges,&before,sizeof before));
}
static int actual_prefix_inputs(void)
{
 return recon_native_at_entry((const void*)choose_switch_prefix_count,(const void*)prefix_entry,real_input,0)==1;
}
static int prefix_controls(void)
{
 Node nodes[11],*pivot;int left;unsigned i;
 memset(nodes,0,sizeof nodes);
 for(i=0;i<10;i++){nodes[i].opcode=0x3d;nodes[i].left.value=nodes[i].detail.value=i;nodes[i].second.node=&nodes[i+1];}
 nodes[10].opcode=0x4c;
 if(choose_switch_prefix_count(nodes,1,1)!=2||choose_switch_prefix_count(nodes,10,10)!=11)return 0;
 nodes[8].left.value=nodes[8].detail.value=100;nodes[9].left.value=nodes[9].detail.value=1000;
 if(choose_switch_prefix_count(nodes,10,10)!=9)return 0;
 split_switch_ranges(nodes,10,10,1000,&pivot,&left);if(left!=8||pivot!=&nodes[8])return 0;
 nodes[6].left.value=nodes[6].detail.value=50;nodes[7].left.value=nodes[7].detail.value=75;
 if(choose_switch_prefix_count(nodes,10,10)!=11)return 0;
 split_switch_ranges(nodes,10,10,1000,&pivot,&left);return left==5&&pivot==&nodes[5];
}
static void __fastcall pivot_entry(Node *range)
{
 Node *label,*first,*second;Expression *expression=switch_dispatch_expression;
 if(range->opcode!=0x3d||range->left.value!=0x7ffff||range->detail.value!=0x7ffff||
  !range->right.symbol||switch_selector_type!=0x204||switch_selector->opcode!=0x30||
  switch_selector->attributes.value!=0x10204||expression->tree!=switch_dispatch_node||expression->next)recon_native_entry_finish(0);
 label=make_switch_pivot(range);first=expression->tree;second=expression->next->tree;
 recon_native_entry_finish(label->opcode==0x29&&label->right.symbol&&label->right.symbol->kind==3&&
  first->opcode==0x38&&first->right.symbol==label->right.symbol&&first->left.node->opcode==0x21&&
  first->left.node->second.node->left.value==0x7ffff&&second->opcode==0x38&&
  second->right.symbol==range->right.symbol&&second->left.node->opcode==0x20&&
  second->left.node->second.node->left.value==0x7ffff&&expression->next->next->tree==switch_dispatch_node);
}
static int actual_pivot(void)
{
 return recon_native_at_entry((const void*)make_switch_pivot,(const void*)pivot_entry,real_input,0)==1;
}
static void __fastcall stack_copy_entry(MachineInstruction *first,MachineInstruction *last,MachineOperand *operand,unsigned bytes)
{
 MachineInstruction *instruction;unsigned n=0;
 int valid=bytes==4&&first->opcode==0x70140&&last->opcode==0x460140&&operand==&first->operands[1]&&
  operand->flags==4&&operand->value.address->first.displacement==12&&operand->value.address->symbol&&
  operand->value.address->base.value==4&&!operand->value.address->index.value;
 for(instruction=first->next;instruction!=last;instruction=instruction->next)++n;
 if(!valid||n!=4||first->next->opcode!=0x140||first->next->operands[0].flags!=0x8003)recon_native_entry_finish(0);
 recon_native_entry_finish(!stack_copy_operand_changed(first,last,operand,bytes)&&operand->value.address->first.displacement==12);
}
static int actual_stack_copy_inputs(void)
{
 return recon_native_at_entry((const void*)stack_copy_operand_changed,(const void*)stack_copy_entry,real_input,0)==1;
}
static void __fastcall stack_child_entry(MachineInstruction *instruction,MachineOperand *operand,unsigned bytes)
{
 MachineAddress before=*operand->value.address;
 if(bytes!=4||instruction->opcode!=0x140||instruction->operands[0].flags!=0x8003||
  instruction->operands[1].flags!=0x401||instruction->operands[1].value.constant||operand->flags!=4||
  before.first.displacement!=12||before.base.value!=4||before.index.value||!before.symbol)recon_native_entry_finish(0);
 recon_native_entry_finish(!stack_copy_instruction_changed(instruction,operand,bytes)&&!memcmp(operand->value.address,&before,sizeof before));
}
static int actual_stack_child(void)
{
 return recon_native_at_entry((const void*)stack_copy_instruction_changed,(const void*)stack_child_entry,real_input,0)==1;
}
static int stack_copy_controls(void)
{
 BackendSymbol symbol,other;MachineAddress address,destination;MachineOperand operand;
 MachineInstruction instruction,records[3];MachineCallInfo call;
 memset(&symbol,0,sizeof symbol);memset(&other,0,sizeof other);memset(&address,0,sizeof address);
 memset(&destination,0,sizeof destination);memset(&instruction,0,sizeof instruction);memset(&call,0,sizeof call);
 symbol.symbol.kind=other.symbol.kind=1;address.symbol=&symbol.symbol;address.base.words.number=REG_ESP;address.first.displacement=12;
 operand.flags=OPERAND_BASED;operand.value.address=&address;
 instruction.opcode=(MC_PUSH<<8)|0x40;instruction.operands[0].flags=OPERAND_REGISTER;
 if(stack_copy_instruction_changed(&instruction,&operand,4)||address.first.displacement!=16)return 0;
 instruction.opcode=(MC_POP<<8)|0x40;if(stack_copy_instruction_changed(&instruction,&operand,4)||address.first.displacement!=12)return 0;
 instruction.opcode=(MC_SUB<<8)|0x40;instruction.operands[0].value.constant=REG_ESP;instruction.operands[1].value.constant=8;
 if(stack_copy_instruction_changed(&instruction,&operand,4)||address.first.displacement!=20)return 0;
 instruction.opcode=(MC_ADD<<8)|0x40;if(stack_copy_instruction_changed(&instruction,&operand,4)||address.first.displacement!=12)return 0;
 call.argument_bytes=8;instruction.opcode=(MC_CALL<<8)|0x40;instruction.operands[1].value.call=&call;
 if(stack_copy_instruction_changed(&instruction,&operand,4)||address.first.displacement!=4)return 0;
 address.first.displacement=12;symbol.attributes=0x2000;
 if(!stack_copy_instruction_changed(&instruction,&operand,4)||address.first.displacement!=12)return 0;
 symbol.symbol.kind=19;if(stack_copy_instruction_changed(&instruction,&operand,4)||address.first.displacement!=4)return 0;
 symbol.symbol.kind=1;symbol.attributes=0;address.first.displacement=12;
 instruction.opcode=(MC_STOS<<8)|0x40;if(!stack_copy_instruction_changed(&instruction,&operand,4))return 0;
 instruction.opcode=(MC_MOV<<8)|0x40;instruction.operands[0].flags=OPERAND_BASED;instruction.operands[0].value.address=&destination;
 destination.symbol=&symbol.symbol;destination.base.words.number=REG_ESP;destination.first.displacement=16;
 if(stack_copy_instruction_changed(&instruction,&operand,4))return 0;
 destination.first.displacement=15;if(!stack_copy_instruction_changed(&instruction,&operand,4))return 0;
 destination.symbol=&other.symbol;if(stack_copy_instruction_changed(&instruction,&operand,4))return 0;
 destination.symbol=0;if(stack_copy_instruction_changed(&instruction,&operand,4))return 0;
 symbol.attributes=0x2000;if(!stack_copy_instruction_changed(&instruction,&operand,4))return 0;
 symbol.attributes=0;destination.symbol=&symbol.symbol;destination.first.displacement=16;
 instruction.operands[0].flags=OPERAND_INDEXED;if(!stack_copy_instruction_changed(&instruction,&operand,4))return 0;
 operand.flags=OPERAND_INDEXED;address.index.words.number=destination.index.words.number=REG_ESI;
 if(stack_copy_instruction_changed(&instruction,&operand,4))return 0;
 destination.index.words.number=REG_EDI;if(!stack_copy_instruction_changed(&instruction,&operand,4))return 0;
 destination.index.words.number=REG_ESI;instruction.operands[0].flags|=0x100;
 if(!stack_copy_instruction_changed(&instruction,&operand,4))return 0;
 operand.flags=OPERAND_BASED;memset(records,0,sizeof records);records[0].next=&records[1];records[1].next=&records[2];records[1].opcode=1;
 if(!stack_copy_operand_changed(records,&records[2],&operand,4))return 0;
 records[1].opcode=4;if(stack_copy_operand_changed(records,&records[2],&operand,4)||address.first.displacement!=12)return 0;
 address.base.words.number=REG_EBP;if(!stack_copy_operand_changed(records,&records[2],&operand,4))return 0;
 address.base.words.number=REG_ESP;address.symbol=0;return stack_copy_operand_changed(records,&records[2],&operand,4)==1;
}
const ReconTest sparse_tests[]={
 {"actual-sequenced-expression-private-temporary-and-result-owners",1,actual_materialization},
 {"actual-sparse-splitter-six-ABI-inputs",1,actual_split_inputs},
 {"actual-prefix-child-three-ABI-inputs-and-unchanged-range",1,actual_prefix_inputs},
 {"sparse-fallback-dense-prefix-and-six-range-threshold",1,prefix_controls},
 {"actual-switch-pivot-input-label-targets-and-expression-owners",1,actual_pivot},
 {"actual-stack-copy-open-interval-four-ABI-inputs",1,actual_stack_copy_inputs},
 {"actual-stack-copy-child-three-ABI-inputs-and-private-address",1,actual_stack_child},
 {"stack-copy-motion-exposure-overlap-index-and-label-controls",1,stack_copy_controls}
};
