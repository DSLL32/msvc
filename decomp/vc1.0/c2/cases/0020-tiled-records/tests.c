#include <recon-tool.h>
#include <recon-native.h>
#include "region-motion.h"
#include "flow-edges.h"
#include "machine-analysis.h"
#include "instruction-selection.h"
#include "register-variables.h"
#include "bitsets.h"
#include "algebra.h"
#include "tree-rewrite.h"
#include "expression-fields.h"
#include <string.h>
#include <recon.h>
extern int main(int,char **,char **);
static void real_input(void *unused)
{
 char *arguments[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,arguments,0);
}
/* All seven ABI inputs and the meaningful expression/node owners come from
   the genuine first1371-frontier snapshot; raw addresses and padding differ. */
static void __fastcall memory_entry(Expression *anchor,Node *value,Node *address,
 unsigned opcode,Node *counter,unsigned bytes,Node *assignment)
{
 Expression *tail;Node *result,*arguments;
 int valid=anchor&&anchor->tree&&!anchor->source_location&&opcode==0x97&&bytes==4&&
  value!=address&&value!=counter&&assignment->opcode==0x32&&assignment->attributes.value==0x20204&&
  assignment->left.node==address&&assignment->second.node==value&&assignment->right.value&&assignment->detail.value&&
  value->opcode==0x30&&value->attributes.value==0x110204&&value->info.value==8&&value->left.node&&value->right.value&&value->detail.value&&
  address->opcode==0x30&&address->attributes.value==0x10404&&!address->info.value&&address->left.node&&address->right.value&&address->detail.value&&
  counter->opcode==0x30&&counter->attributes.value==0x110204&&counter->info.value==2&&counter->left.node&&counter->right.value&&counter->detail.value;
 if(!valid)recon_native_entry_finish(0);
 insert_region_memory_operation(anchor,value,address,opcode,counter,bytes,assignment);
 for(tail=anchor;tail->next;tail=tail->next);
 result=tail->tree;arguments=result->second.node;
 recon_native_entry_finish(!tail->field08&&!tail->source_location&&result->opcode==0x40&&
  result->attributes.value==0x404&&!result->info.value&&result->detail.value==0x204&&
  result->left.node->opcode==0x33&&result->left.node->attributes.words.type==0x102&&result->left.node->left.value==0x97&&
  arguments->opcode==0x55&&arguments->attributes.value==0x404&&arguments->left.node!=address&&
  arguments->right.value==assignment->right.value&&arguments->detail.value==assignment->detail.value&&
  arguments->second.node->opcode==0x55&&arguments->second.node->attributes.value==0x204&&arguments->second.node->left.node!=value&&
  arguments->second.node->right.value==value->right.value&&arguments->second.node->detail.value==value->detail.value);

}
static int actual_memory_inputs(void)
{
 return recon_native_at_entry((const void*)insert_region_memory_operation,(const void*)memory_entry,real_input,0)==1;
}
static Node *__fastcall copy_entry(Node *node)
{
 recon_native_entry_finish(node->opcode==0x30&&node->attributes.value==0x110204&&node->info.value==8&&
  node->left.node&&node->right.value&&node->detail.value);
}
static int actual_copy_input(void)
{
 return recon_native_at_entry((const void*)copy_region_memory_tree,(const void*)copy_entry,real_input,0)==1;
}
static void __fastcall block_entry(WorkItem *work)
{
 unsigned i;Expression *expression=work->body.expressions;
 int valid=work==hoist_first_work&&work==hoist_last_work&&work->next==hoist_after_work&&
  work->previous==hoist_header_work&&work->number==3&&work->region_depth==1&&work->exception_scope==-1&&
  !work->flags&&work->successors&&work->predecessors&&work->analysis.dominators&&work->region&&
  !work->pass.selection.uses&&!work->pass.selection.definitions&&expression&&expression->next&&
  expression->source_location==16&&!expression->field08&&expression->tree->opcode==0x29&&expression->tree->right.symbol&&
  transformation_cursor.expression&&transformation_cursor.expression->next&&!transformation_cursor.expression->source_location;
 for(i=0;i<6;i++)if(!work->flow_sets[i])valid=0;
 recon_native_entry_finish(valid);
}
static int actual_removed_work(void)
{
 return recon_native_at_entry((const void*)remove_region_block,(const void*)block_entry,real_input,0)==1;
}
static int actual_unlinked_work(void)
{
 return recon_native_at_entry((const void*)unlink_region_work,(const void*)block_entry,real_input,0)==1;
}
static int unlink_controls(void)
{
 WorkItem work,following,predecessor,duplicate,successor,unrelated;FlowLink edges[5];
 memset(&work,0,sizeof work);memset(&following,0,sizeof following);memset(&predecessor,0,sizeof predecessor);
 memset(&duplicate,0,sizeof duplicate);memset(&successor,0,sizeof successor);memset(&unrelated,0,sizeof unrelated);memset(edges,0,sizeof edges);
 work.number=duplicate.number=3;work.next=&following;work.successors=&edges[0];edges[0].work=&successor;
 successor.predecessors=&edges[1];edges[1].work=&work;
 work.predecessors=&edges[2];edges[2].work=&predecessor;
 predecessor.successors=&edges[3];edges[3].work=&duplicate;following.predecessors=&edges[4];edges[4].work=&unrelated;
 unlink_region_work(&work);
 if(successor.predecessors||edges[3].work!=&following||following.predecessors!=&edges[2]||edges[2].next!=&edges[4]||
  work.successors!=&edges[0]||work.predecessors!=&edges[2])return 0;
 memset(&work,0,sizeof work);unlink_region_work(&work);
 return !work.successors&&!work.predecessors;
}
static void __fastcall optional_reference_entry(FlowLink **links,WorkItem *work)
{
 recon_native_entry_finish(work==hoist_first_work&&work->successors&&
  links==&work->successors->work->predecessors);
}
static int actual_optional_reference(void)
{
 return recon_native_at_entry((const void*)remove_optional_flow_reference,(const void*)optional_reference_entry,real_input,0)==1;
}
static int optional_reference_controls(void)
{
 FlowLink first,second,*head;WorkItem a,b,absent;memset(&first,0,sizeof first);memset(&second,0,sizeof second);
 memset(&a,0,sizeof a);memset(&b,0,sizeof b);memset(&absent,0,sizeof absent);
 head=&first;first.next=&second;first.work=&a;second.work=&b;second.flags=1;
 remove_optional_flow_reference(&head,&absent);if(head!=&first||first.next!=&second)return 0;
 remove_optional_flow_reference(&head,&b);if(head!=&first||first.next||second.work!=&b||second.flags!=1)return 0;
 remove_optional_flow_reference(&head,&a);if(head)return 0;
 remove_optional_flow_reference(&head,&absent);return !head;
}
static void __fastcall transform_entry(MachineFlowRegion *region)
{
 MachineFlowRegion before;LabelSymbol *label=(LabelSymbol*)region->label->record.symbol;unsigned i;
 int valid=region->parent&&!region->next&&!region->children&&region->first&&region->last&&region->first!=region->label&&
  !region->exit_label&&region->first_block==8&&region->last_block==11&&!region->pending_references&&region->initial_references==1&&
  !region->field58&&region->flags==0x40&&region->label->opcode==1&&region->label->labels.attachment==region&&
  region->label->labels.flags==1&&label->symbol.kind==3&&label->instruction==region->label&&label->references==1;
 for(i=0;i<3;i++)if(region->references[i])valid=0;
 for(i=0;i<8;i++)if(!region->registers[i])valid=0;
 if(!valid)recon_native_entry_finish(0);before=*region;
 prepare_machine_region_transform(region);
 recon_native_entry_finish(!memcmp(region,&before,sizeof before)&&label->references==1);
}
static int actual_transform_input(void)
{
 return recon_native_at_entry((const void*)prepare_machine_region_transform,(const void*)transform_entry,real_input,0)==1;
}
static void __fastcall insertion_entry(MachineFlowRegion *region)
{
 MachineInstruction *position=region->first->previous,*scan=position;
 int valid=region->first_block==8&&region->last_block==11&&region->flags==0x40&&region->initial_references==1;
 while((scan->opcode&15)!=7)scan=scan->previous;
 if(!valid)recon_native_entry_finish(0);
 prepare_region_insertion(region);
 recon_native_entry_finish(active_machine_region==region&&transformation_cursor.instruction==position&&
  region_insertion_block==(MachineBlockHeader*)scan);
}
static int actual_insertion_input(void)
{
 return recon_native_at_entry((const void*)prepare_region_insertion,(const void*)insertion_entry,real_input,0)==1;
}
static int insertion_controls(void)
{
 MachineInstruction records[4];MachineFlowRegion region;unsigned i;
 memset(records,0,sizeof records);memset(&region,0,sizeof region);
 for(i=0;i<4;i++){if(i)records[i].previous=&records[i-1];if(i<3)records[i].next=&records[i+1];}
 records[0].opcode=7;records[1].opcode=4;records[3].opcode=1;region.first=&records[2];region.label=&records[3];
 prepare_region_insertion(&region);
 return active_machine_region==&region&&transformation_cursor.instruction==&records[1]&&region_insertion_block==(MachineBlockHeader*)&records[0];
}
/* Original real first range is a single MOV immediate, generation4->5.
   Compare defined record content and real deferred/register owners, not pool addresses. */
static void __fastcall definition_range_entry(unsigned short number,MachineInstruction *first,MachineInstruction *last)
{
 MachineInstruction *old_tail=deferred_tail,*copy;MachineBlockHeader *block;
 int valid=number==0x402f&&first==last&&first->opcode==0x140&&!first->record.symbol&&
  first->operands[0].flags==1&&first->operands[0].value.constant==0x402f&&
  first->operands[1].flags==2&&first->operands[1].value.constant==0x2468ace0&&
  deferred_instructions&&old_tail==selection_global_header->previous;
 if(!valid)recon_native_entry_finish(0);
 record_machine_definition_range(number,first,last);
 copy=deferred_tail;block=register_rematerialization(number);
 recon_native_entry_finish(block&&block->opcode==7&&!block->uses&&bitset_contains(block->definitions,47)&&
  block->previous==old_tail&&block->next==copy&&copy->previous==(MachineInstruction*)block&&
  copy->next==(MachineInstruction*)selection_global_header&&selection_global_header->previous==copy&&
  first->opcode==0x5000140&&copy!=first&&!memcmp((char*)copy+8,(char*)first+8,24));
}
static int actual_definition_range(void)
{
 return recon_native_at_entry((const void*)record_machine_definition_range,(const void*)definition_range_entry,real_input,0)==1;
}
static void __fastcall region_register_entry(MachineFlowRegion *region,unsigned short number,int add,MachineBlockHeader *preserved)
{
 WorkItem *work;MachineBlockHeader *before,*last,*block;int valid;
 before=(MachineBlockHeader*)region->first->previous;
 while((before->opcode&15)!=7)before=(MachineBlockHeader*)before->previous;
 last=(MachineBlockHeader*)region->last;while((last->opcode&15)!=7)last=(MachineBlockHeader*)last->previous;
 for(work=current_work;work->body.block!=before;work=work->next);
 valid=number==0x402f&&add==1&&!preserved&&before->number==7&&last->number==11&&
  work->number==7&&work->pass.selection.uses==before->uses&&work->pass.selection.definitions==before->definitions&&
  (MachineInstruction*)work->next->body.block==region->first&&work->next->number==8;
 if(!valid)recon_native_entry_finish(0);
 record_region_register(region,number,add,preserved);
 if(!bitset_contains(work->pass.selection.definitions,47)||!bitset_contains(before->definitions,47))recon_native_entry_finish(0);
 do{work=work->next;block=work->body.block;
  if(!bitset_contains(work->pass.selection.uses,47)||!bitset_contains(block->uses,47)||
   !bitset_contains(work->pass.selection.definitions,47)||!bitset_contains(block->definitions,47))recon_native_entry_finish(0);
 }while(block!=last);
 recon_native_entry_finish(1);
}
static int actual_region_register(void)
{
 return recon_native_at_entry((const void*)record_region_register,(const void*)region_register_entry,real_input,0)==1;
}
static int region_register_controls(void)
{
 MachineBlockHeader blocks[3];WorkItem work[3];MachineFlowRegion region;unsigned i;
 memset(blocks,0,sizeof blocks);memset(work,0,sizeof work);memset(&region,0,sizeof region);
 for(i=0;i<3;i++){blocks[i].opcode=7;work[i].body.block=&blocks[i];if(i){blocks[i].previous=(MachineInstruction*)&blocks[i-1];work[i-1].next=&work[i];}}
 region.first=(MachineInstruction*)&blocks[1];region.last=(MachineInstruction*)&blocks[2];current_work=work;
 record_region_register(&region,0x402f,1,0);
 for(i=0;i<3;i++)if(!bitset_contains(work[i].pass.selection.definitions,47)||!bitset_contains(blocks[i].definitions,47))return 0;
 record_region_register(&region,0x402f,0,&blocks[2]);
 if(bitset_contains(work[0].pass.selection.definitions,47)||bitset_contains(blocks[0].definitions,47))return 0;
 for(i=1;i<3;i++)if(bitset_contains(work[i].pass.selection.uses,47)||bitset_contains(blocks[i].uses,47)||
  bitset_contains(work[i].pass.selection.definitions,47)!=(i==2)||bitset_contains(blocks[i].definitions,47)!=(i==2))return 0;
 return 1;
}
static void __fastcall replacement_entry(unsigned identifier,MachineInstruction *previous,MachineInstruction *replacement)
{
 MachineInstruction *match=deferred_instructions,*before,*next;
 while((match->opcode>>24)!=identifier)match=match->next;
 if(identifier!=5||previous==replacement||previous->opcode!=0x5000140||replacement->opcode!=0x5000140||
  previous->operands[0].flags!=1||previous->operands[0].value.constant!=0x402f||previous->operands[1].flags!=2||
  previous->operands[1].value.constant!=0x2468ace0||replacement->operands[1].flags!=1||
  replacement->operands[1].value.constant!=0x14032||!machine_records_equal(previous,match))recon_native_entry_finish(0);
 before=match->previous;next=match->next;
 replace_machine_definition(identifier,previous,replacement);
 recon_native_entry_finish(before->next!=match&&before->next!=replacement&&before->next->next==next&&
  next->previous==before->next&&machine_records_equal(before->next,replacement));
}
static int actual_replacement(void)
{
 return recon_native_at_entry((const void*)replace_machine_definition,(const void*)replacement_entry,real_input,0)==1;
}
static void __fastcall complement_entry(Node **slot)
{
 Node *node=*slot,*left=node->left.node,*constant=node->second.node;
 if(node->opcode!=0xd||node->attributes.value!=0x204||tree_changed||left->opcode!=0x30||
  left->attributes.value!=0x10204||constant->opcode!=0x33||constant->attributes.value!=0x204||
  constant->left.value!=0x1234567)recon_native_entry_finish(0);
 recon_native_entry_finish(extract_complement(slot)==1&&*slot==node&&node->left.node==left&&
  node->second.node!=constant&&node->second.node->opcode==0x33&&
  node->second.node->left.value==0xfedcba98&&tree_changed==1);
}
static int actual_complement(void)
{
 return recon_native_at_entry((const void*)extract_complement,(const void*)complement_entry,real_input,0)==1;
}
static int complement_controls(void)
{
 Node *leaf,*wrapper,*slot,*xor_node,*constant;Node model;
 memset(&model,0,sizeof model);model.attributes.words.type=0x204;
 leaf=new_node(0x30);leaf->attributes.words.type=0x204;slot=leaf;tree_changed=0;
 if(extract_complement(&slot)||slot!=leaf||tree_changed)return 0;
 wrapper=new_node(0xe);wrapper->left.node=leaf;wrapper->attributes.words.type=0x204;slot=wrapper;
 if(!extract_complement(&slot)||slot!=leaf||tree_changed!=1)return 0;
 constant=typed_integer_node(&model,0x1234567);slot=constant;tree_changed=0;
 if(!extract_complement(&slot)||slot->opcode!=0x33||slot->left.value!=0xfedcba98||tree_changed!=1)return 0;
 xor_node=new_node(0xd);xor_node->attributes.words.type=0x204;xor_node->left.node=leaf;
 wrapper=new_node(0xe);wrapper->left.node=leaf;xor_node->second.node=wrapper;slot=xor_node;tree_changed=0;
 if(!extract_complement(&slot)||slot!=xor_node||xor_node->second.node!=leaf||tree_changed!=1)return 0;
 wrapper=new_node(0xe);wrapper->left.node=leaf;xor_node->left.node=wrapper;tree_changed=0;
 if(!extract_complement(&slot)||xor_node->left.node!=leaf||tree_changed!=1)return 0;
 tree_changed=0;return !extract_complement(&slot)&&slot==xor_node&&!tree_changed;
}
const ReconTest tiled_tests[]={
 {"actual-bulk-memory-seven-ABI-inputs-and-node-owners",1,actual_memory_inputs},
 {"actual-copy-child-input-from-unchanged-value",1,actual_copy_input},
 {"actual-loop-removal-work-lifetime-and-flow-set-owners",1,actual_removed_work},
 {"actual-flow-unlink-child-retains-work-input",1,actual_unlinked_work},
 {"flow-unlink-numbered-target-release-and-list-splice",1,unlink_controls},
 {"actual-optional-reference-address-and-work-inputs",1,actual_optional_reference},
 {"optional-reference-absent-head-tail-and-flags",1,optional_reference_controls},
 {"actual-machine-region-transform-unchanged-guard-and-owners",1,actual_transform_input},
 {"actual-insertion-region-retains-unchanged-owner-state",1,actual_insertion_input},
 {"region-insertion-marker-walk-and-block-owner",1,insertion_controls},
 {"actual-definition-range-register-generation-and-clone-owners",1,actual_definition_range},
 {"actual-region-register-four-ABI-inputs-and-shared-sets",1,actual_region_register},
 {"region-register-add-remove-and-preserved-definition",1,region_register_controls},
 {"actual-deferred-generation-replacement-inputs-and-list-owners",1,actual_replacement},
 {"actual-complement-slot-both-arms-and-folded-constant",1,actual_complement},
 {"complement-failure-removal-constant-and-XOR-search-order",1,complement_controls}
};
