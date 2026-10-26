#include "case.h"
#include <recon-native.h>
#include "tail-recursion.h"
#include "work-optimizer.h"
#include "intrinsics.h"
#include <string.h>
#include <recon.h>
extern int main(int,char **,char **);
extern const IntrinsicDescription rotate_left_intrinsic,rotate_right_intrinsic;
extern const IntrinsicDescription absolute_integer_intrinsic;
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};
 (void)unused;main(1,argv,0);
}
/* First genuine tail query:31 live expressions, with the requested label at25.
   The earlier jumps at6/9/24 reference that same symbol but are not labels.
   Snapshot only defined owner/opcode/symbol fields, not allocation padding. */
static Expression *__fastcall tail_entry(Expression *list,SymbolHeader *label)
{
 Expression *owners[31],*next[31],*e=list,*result;
 Node *trees[31];unsigned op[31],i;
 for(i=0;i<31&&e;++i,e=e->next){owners[i]=e;next[i]=e->next;trees[i]=e->tree;op[i]=e->tree->opcode;}
 if(i!=31||e||op[25]!=0x29||owners[25]->tree->right.symbol!=label||
  op[6]!=0x3a||owners[6]->tree->right.symbol!=label||
  op[24]!=0x3a||owners[24]->tree->right.symbol!=label)recon_native_entry_finish(0);
 result=find_tail_label(list,label);
 for(i=0;i<31;++i)if(owners[i]->next!=next[i]||owners[i]->tree!=trees[i]||
  trees[i]->opcode!=op[i])recon_native_entry_finish(0);
 recon_native_entry_finish(result==owners[25]);return 0;
}
static int actual_tail(void)
{return recon_native_at_entry((const void*)find_tail_label,(const void*)tail_entry,real_input,0)==1;}
/* First genuine pure sqrt call pair: separate7a owners, one55 argument each,
   canonical value ID2a, ending at4c. Preserve both owners and their children. */
static int __fastcall chain_entry(Node *left,Node *right)
{
 Node *la=left->second.node,*ra=right->second.node,*lv=la->left.node,*rv=ra->left.node;
 int result;
 if(left==right||left->opcode!=0x7a||right->opcode!=0x7a||la->opcode!=0x55||
  ra->opcode!=0x55||lv->info.words.identifier!=0x2a||rv->info.words.identifier!=0x2a||
  la->second.node->opcode!=0x4c||ra->second.node->opcode!=0x4c)recon_native_entry_finish(0);
 result=equal_chain_value_ids(left,right);
 recon_native_entry_finish(result==1&&left->second.node==la&&right->second.node==ra&&
  la->left.node==lv&&ra->left.node==rv&&lv->info.words.identifier==0x2a&&
  rv->info.words.identifier==0x2a);return 0;
}
static int actual_chain(void)
{return recon_native_at_entry((const void*)equal_chain_value_ids,(const void*)chain_entry,real_input,0)==1;}
/* Cold argument-list controls: identity differs, IDs match while high info
   flags differ; then mismatched IDs and both unequal-length directions. */
static int chain_edges(void)
{
 Node l={0},r={0},la={0},ra={0},lv={0},rv={0},end={0};
 end.opcode=0x4c;l.second.node=&la;r.second.node=&ra;
 la.opcode=ra.opcode=0x55;la.left.node=&lv;ra.left.node=&rv;
 la.second.node=ra.second.node=&end;lv.info.value=0x12348001;rv.info.value=0x98768001;
 if(!equal_chain_value_ids(&l,&r))return 0;
 rv.info.words.identifier=2;if(equal_chain_value_ids(&l,&r))return 0;
 r.second.node=&end;if(equal_chain_value_ids(&l,&r))return 0;
 l.second.node=&end;if(!equal_chain_value_ids(&l,&r))return 0;
 r.second.node=&ra;return !equal_chain_value_ids(&l,&r);
}
/* Exact encountered original descriptor fields, independently decoded from
  83ac8/843c8/843d8. This contextual data adds no App/text extent credit. */
static int descriptors(void)
{
 return absolute_integer_intrinsic.result_type==0x104&&!absolute_integer_intrinsic.reserved&&
  absolute_integer_intrinsic.flags==0x20&&!strcmp(absolute_integer_intrinsic.commands,"u")&&
  !*absolute_integer_intrinsic.runtime_name&&!rotate_left_intrinsic.result_type&&
  !rotate_left_intrinsic.reserved&&!rotate_left_intrinsic.flags&&
  !strcmp(rotate_left_intrinsic.commands,"vb")&&!*rotate_left_intrinsic.runtime_name&&
  !rotate_right_intrinsic.result_type&&!rotate_right_intrinsic.reserved&&
  !rotate_right_intrinsic.flags&&!strcmp(rotate_right_intrinsic.commands,"vb")&&
  !*rotate_right_intrinsic.runtime_name;
}
/* Native unit only: borrow the real interpreter root after it publishes
   current_intrinsic, before argument preparation. No descriptor A recipe or
   private global is forced merely to run the dormant sequencing helper. */
static void __fastcall prefix_entry(Node *node,unsigned intrinsic,Node **arguments)
{
 Node *old=node,*left=node->left.node,*second=node->second.node,*prefix,*result;
 unsigned opcode=node->opcode;
 (void)intrinsic;(void)arguments;
 if(current_intrinsic!=old||!left||!second)recon_native_entry_finish(0);
 prefix=integer_node(0x1234,0x204);
 append_intrinsic_expression(prefix);result=current_intrinsic;
 recon_native_entry_finish(result&&result!=old&&result->opcode==0x44&&
  result->left.node==prefix&&result->second.node==old&&
  old->opcode==opcode&&old->left.node==left&&old->second.node==second&&
  result->attributes.words.type==old->attributes.words.type);
}
static int actual_intrinsic_prefix(void)
{return recon_native_at_entry((const void*)prepare_intrinsic_arguments,
 (const void*)prefix_entry,real_input,0)==1;}
const ReconTest dictionary_tests[]={
 {"actual tail label identity and unchanged chain",1,actual_tail},
 {"actual pure call argument value IDs",1,actual_chain},
 {"cold argument list equality edges",1,chain_edges},
 {"encountered rotate and integer absolute descriptors",1,descriptors},
 {"unit-qualified sequence precedes a genuine active intrinsic root",1,actual_intrinsic_prefix}
};
