#include <recon-tool.h>
extern int c2_actual_double_input(void),c2_double_input_bit_controls(void);

#include <recon-native.h>
#include "extended.h"
#include "algebra.h"
#include "float-machine.h"
#include "float-peepholes.h"
#include "machine-analysis.h"
#include <string.h>
#include <recon.h>
extern int main(int,char **,char **);
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);
}
static void __cdecl maximum_entry(const Extended *left,const Extended *right,Extended *result)
{
 static const Extended a={0,0xc0000000U,0x3ffe,8},b={0,0xa0000000U,0x3fff,8};
 if(!left||!right||!result||left==right||result==left||result==right||memcmp(left,&a,12)||memcmp(right,&b,12))recon_native_entry_finish(0);
 recon_native_entry_finish(!maximum_extended(left,right,result)&&!memcmp(result,&b,12)&&!memcmp(left,&a,12)&&!memcmp(right,&b,12));
}
static int actual_maximum(void)
{
 return recon_native_at_entry((const void*)maximum_extended,(const void*)maximum_entry,real_input,0)==1;
}
static int maximum_controls(void)
{
 Extended a={0,0xc0000000U,0x3fff,4},b={0,0x80000000U,0x4000,8},result;
 if(maximum_extended(&a,&b,&result)||result.low||result.high!=0x80000000U||result.exponent!=0x4000||result.format!=8)return 0;
 a.high=0x80000000U;a.exponent=0xbfff;a.format=10;b.exponent=0xc000;b.format=4;
 if(maximum_extended(&a,&b,&result)||result.low||result.high!=0x80000000U||result.exponent!=0xbfff||result.format!=10)return 0;
 a.high=b.high=0;a.exponent=0;b.exponent=0x8000;a.format=b.format=4;
 return !maximum_extended(&a,&b,&result)&&!result.low&&!result.high&&result.exponent==0x8000&&result.format==4;
}

extern Node *__fastcall push_unary_into_left(Node *);
extern int __fastcall float_target_is_epilogue(MachineInstruction *);
extern int __fastcall resolve_machine_region_suffix(MachineFlowRegion *,MachineFlowRegion *);
static void __fastcall nonnegative_entry(Node *node)
{
 Node before=*node;
 if(node->opcode!=0x30||node->attributes.value!=0x10104||node->detail.node!=node)recon_native_entry_finish(0);
 recon_native_entry_finish(!is_nonnegative_tree(node)&&!memcmp(node,&before,sizeof(before)));
}
static int actual_nonnegative(void)
{
 return recon_native_at_entry((const void*)is_nonnegative_tree,(const void*)nonnegative_entry,real_input,0)==1;
}
static int nonnegative_controls(void)
{
 Node positive,negative,shift,both,cast;
 memset(&positive,0,sizeof(positive));positive.opcode=0x33;positive.attributes.words.type=0x104;positive.left.value=7;
 negative=positive;negative.left.value=0xffffffffU;
 if(!is_nonnegative_tree(&positive)||is_nonnegative_tree(&negative))return 0;
 memset(&shift,0,sizeof(shift));shift.opcode=0xa;shift.attributes.words.type=0x204;shift.left.node=&negative;shift.second.node=&positive;
 if(!is_nonnegative_tree(&shift))return 0;
 shift.attributes.words.type=0x104;if(is_nonnegative_tree(&shift))return 0;
 memset(&both,0,sizeof(both));both.opcode=0xb;both.left.node=&negative;both.second.node=&positive;
 if(!is_nonnegative_tree(&both))return 0;
 both.second.node=&negative;if(is_nonnegative_tree(&both))return 0;
 memset(&cast,0,sizeof(cast));cast.opcode=0x34;cast.attributes.words.type=0x204;cast.left.node=&negative;
 negative.attributes.words.type=0x102;if(!is_nonnegative_tree(&cast))return 0;
 cast.attributes.words.type=0x101;return !is_nonnegative_tree(&cast);
}
static void __fastcall unary_entry(Node *node)
{
 Node *operand=node->left.node,*right=operand->second.node,*left=operand->left.node,*result;
 if(node->opcode!=0x34||node->attributes.words.type!=0x204||operand->opcode!=9||operand->attributes.words.type!=0x104)recon_native_entry_finish(0);
 result=push_unary_into_left(node);
 recon_native_entry_finish(result==operand&&result->opcode==9&&result->attributes.words.type==0x204&&
  result->left.node==node&&node->left.node==left&&result->second.node==right);
}
static int actual_unary(void)
{
 return recon_native_at_entry((const void*)push_unary_into_left,(const void*)unary_entry,real_input,0)==1;
}
static void __fastcall branches_entry(FloatStreamBranch *head)
{
 FloatStreamBranch *duplicate,*third;
 if(!head||!(duplicate=head->next)||!(third=duplicate->next)||third->next||head->label!=duplicate->label||head->label==third->label||
  head->stack_depth||duplicate->stack_depth||third->stack_depth||head->references!=1||duplicate->references!=1||third->references!=1)recon_native_entry_finish(0);
 mark_float_branch_targets(head);
 recon_native_entry_finish(head->references==2&&head->next==third&&third->references==1&&!third->next&&duplicate->next==third);
}
static int actual_branches(void)
{
 return recon_native_at_entry((const void*)mark_float_branch_targets,(const void*)branches_entry,real_input,0)==1;
}
static int branch_duplicate_controls(void)
{
 FloatStreamBranch a,b,c;LabelSymbol label;
 memset(&a,0,sizeof(a));memset(&b,0,sizeof(b));memset(&c,0,sizeof(c));
 a.label=b.label=c.label=&label;a.references=4;b.references=7;c.references=9;
 a.next=&b;b.next=&c;c.stack_depth=1;
 mark_float_branch_targets(&a);mark_float_branch_targets(0);
 return a.references==5&&a.next==&c&&c.references==9&&!c.next&&b.next==&c;
}
static void __fastcall epilogue_entry(MachineInstruction *instruction)
{
 if(instruction->opcode!=0x6080||instruction->operands[0].flags!=1||instruction->operands[0].value.constant!=14||instruction->operands[1].flags!=4)recon_native_entry_finish(0);
 recon_native_entry_finish(!float_target_is_epilogue(instruction));
}
static int actual_epilogue(void)
{
 return recon_native_at_entry((const void*)float_target_is_epilogue,(const void*)epilogue_entry,real_input,0)==1;
}
static int epilogue_controls(void)
{
 MachineInstruction value,pop,leave,label;
 memset(&value,0,sizeof(value));memset(&pop,0,sizeof(pop));memset(&leave,0,sizeof(leave));memset(&label,0,sizeof(label));
 leave.opcode=MC_LEAVE<<8;pop.opcode=MC_EPOP<<8;pop.next=&leave;
 value.opcode=MC_MOV<<8;value.operands[0].flags=1;value.operands[0].value.constant=0x8000;value.next=&pop;
 if(!float_target_is_epilogue(&value))return 0;
 value.operands[0].value.constant=1;if(float_target_is_epilogue(&value))return 0;
 value.opcode=MC_FLD<<8;value.operands[1].flags=4;
 if(!float_target_is_epilogue(&value))return 0;
 label.opcode=1;value.next=&label;if(float_target_is_epilogue(&value))return 0;
 pop.next=&leave;leave.opcode=MC_RET<<8;return !float_target_is_epilogue(&pop);
}
static void __fastcall merge_entry(FloatStreamBranch *branch,MachineInstruction *previous,MachineInstruction *target)
{
 if(!branch||branch->references!=2||branch->stack_depth||branch->flags||!previous||previous->opcode!=0x1040||
  !target||target->opcode!=1||target->record.symbol!=&branch->label->symbol)recon_native_entry_finish(0);
 recon_native_entry_finish(!can_merge_float_branch(branch,previous,target)&&!branch->flags);
}
static int actual_merge(void)
{
 return recon_native_at_entry((const void*)can_merge_float_branch,(const void*)merge_entry,real_input,0)==1;
}
static int merge_epilogue_control(void)
{
 FloatStreamBranch branch;MachineInstruction target,leave;LabelSymbol label;
 memset(&branch,0,sizeof(branch));memset(&target,0,sizeof(target));memset(&leave,0,sizeof(leave));memset(&label,0,sizeof(label));
 branch.label=&label;branch.flags=0x80;target.opcode=1;target.next=&leave;leave.opcode=MC_LEAVE<<8;
 return can_merge_float_branch(&branch,0,&target)==1&&branch.flags==0x81;
}
static void __fastcall reuse_entry(MachineInstruction *target)
{
 if(target->opcode!=1||!float_stream_context||!float_stream_context->last)recon_native_entry_finish(0);
 recon_native_entry_finish(!can_reuse_float_branch_target(target));
}
static int actual_reuse(void)
{
 return recon_native_at_entry((const void*)can_reuse_float_branch_target,(const void*)reuse_entry,real_input,0)==1;
}
static int reuse_controls(void)
{
 MemoryMotionContext context;MachineInstruction before,metadata,ordinary,target;
 memset(&context,0,sizeof(context));memset(&before,0,sizeof(before));memset(&metadata,0,sizeof(metadata));memset(&ordinary,0,sizeof(ordinary));memset(&target,0,sizeof(target));
 context.last=&before;before.next=&metadata;metadata.opcode=7;metadata.next=&ordinary;ordinary.opcode=MC_MOV<<8;ordinary.next=&target;target.opcode=1;
 float_stream_context=&context;
 if(!can_reuse_float_branch_target(&target))return 0;
 ordinary.opcode=MC_JMP<<8;if(can_reuse_float_branch_target(&target))return 0;
 ordinary.opcode=MC_JCC<<8;if(can_reuse_float_branch_target(&target))return 0;
 ordinary.opcode=MC_MOV<<8;ordinary.next=0;return !can_reuse_float_branch_target(&target);
}
static void __fastcall region_entry(MachineFlowRegion *parent,MachineFlowRegion *region)
{
 MachineInstruction *label=region->label;MachineRegionEdge *before=parent->references[0];
 if(region->parent!=parent||parent->children!=region||parent->flags!=4||region->flags!=0x44||
  parent->first_block!=18||region->first_block!=20||!region->children||region->pending_references)recon_native_entry_finish(0);
 recon_native_entry_finish(close_machine_region(parent,region)==1&&!label->labels.attachment&&!parent->children&&
  parent->flags==4&&parent->first_block==18&&parent->references[0]&&parent->references[0]!=before&&!region->references[0]);
}
static int actual_region_close(void)
{
 return recon_native_at_entry((const void*)close_machine_region,(const void*)region_entry,real_input,0)==1;
}
static void __fastcall suffix_entry(MachineFlowRegion *parent,MachineFlowRegion *region)
{
 if(region->parent!=parent||parent->children!=region||region->children||region->last||region->flags!=4||
  region->pending_references!=1||region->initial_references!=1||region->first_block!=21||parent->first_block!=18)recon_native_entry_finish(0);
 recon_native_entry_finish(resolve_machine_region_suffix(parent,region)==1&&!parent->children&&
  !region->pending_references&&!region->initial_references&&!region->references[0]&&parent->references[0]);
}
static int actual_region_suffix(void)
{
 return recon_native_at_entry((const void*)resolve_machine_region_suffix,(const void*)suffix_entry,real_input,0)==1;
}
/* Synthetic controls have no pending edges/children: they cover the flag
   propagation and unmatched-reference branches outside the actual first pair. */
static int region_controls(void)
{
 MachineFlowRegion parent,region;MachineInstruction label;SymbolHeader symbol;
 memset(&parent,0,sizeof(parent));memset(&region,0,sizeof(region));memset(&label,0,sizeof(label));memset(&symbol,0,sizeof(symbol));
 parent.children=&region;parent.flags=8;region.parent=&parent;region.label=&label;region.flags=1;label.labels.attachment=&region;label.record.symbol=&symbol;
 if(close_machine_region(&parent,&region)!=1||label.labels.attachment||parent.children||parent.flags!=9)return 0;
 region.pending_references=region.initial_references=2;
 return !resolve_machine_region_suffix(&parent,&region)&&region.pending_references==2&&region.initial_references==2;
}

/* Cold pop-transition control borrows genuine initialized memory operands
   and the compiler's live value/arena owners at its first normal transfer.
   Local list nodes reproduce the separately observed reversed role aliases;
   this does not claim that the one-shot hook selected the later reversed call.
   That actual path is bound by the full object oracle and native/original
   six-argument observations retained in scratch. */
static void __fastcall reversed_transfer_entry(MachineInstruction *load,MachineInstruction *store,
    MachineInstruction *first,MachineInstruction *last,int reversed,MachineInstruction *comparison)
{
 MachineInstruction source,target,compare,before,metadata,after;
 MachineOperand operand;
 (void)first;(void)last;(void)comparison;
 if(reversed||((load->opcode>>4)&15)!=8||((store->opcode>>4)&15)!=8)recon_native_entry_finish(0);
 memset(&source,0,sizeof(source));memset(&target,0,sizeof(target));memset(&compare,0,sizeof(compare));
 memset(&before,0,sizeof(before));memset(&metadata,0,sizeof(metadata));memset(&after,0,sizeof(after));
 source.opcode=0x6080;source.operands[0]=load->operands[0];source.operands[1]=load->operands[1];
 target.opcode=0x16380;target.operands[0]=store->operands[0];target.operands[1]=store->operands[1];
 compare.opcode=0x15c80;compare.operands[0]=load->operands[0];compare.operands[1]=load->operands[1];operand=compare.operands[1];
 before.next=&source;source.previous=&before;source.next=&metadata;metadata.previous=&source;metadata.next=&compare;
 compare.previous=&metadata;compare.next=&target;target.previous=&compare;target.next=&after;after.previous=&target;
 split_float_memory_transfer(&source,&target,&target,&target,1,&compare);
 recon_native_entry_finish(compare.opcode==0x15d80&&compare.previous==&metadata&&
  !memcmp(&operand,&compare.operands[1],sizeof(operand))&&((target.opcode>>8)&255)==MC_MOV&&
  (target.opcode&0x400000)&&(target.operands[1].flags&15)==1&&(target.operands[1].flags&0x400));
}
static int reversed_transfer_control(void)
{
 return recon_native_at_entry((const void*)split_float_memory_transfer,(const void*)reversed_transfer_entry,real_input,0)==1;
}
const ReconTest resampler_tests[]={
 {"actual-double-input-register-POD-and-FILE",1,c2_actual_double_input},
 {"cold-double-raw-bits-including-NaN-payload",1,c2_double_input_bit_controls},
 {"actual-maximum-constant-operands-and-result",1,actual_maximum},
 {"cold-maximum-order-tags-and-signed-zero",1,maximum_controls},
 {"actual-signed-load-remains-unknown",1,actual_nonnegative},
 {"cold-sign-shift-AND-and-widening",1,nonnegative_controls},
 {"actual-unary-left-distribution-owners",1,actual_unary},
 {"actual-three-branches-coalesce-to-two",1,actual_branches},
 {"cold-depth-and-unit-reference-counting",1,branch_duplicate_controls},
 {"actual-floating-return-not-epilogue",1,actual_epilogue},
 {"cold-MOV-FLD-EPOP-LEAVE-boundaries",1,epilogue_controls},
 {"actual-branch-merge-rejection",1,actual_merge},
 {"cold-direct-epilogue-merge-flag",1,merge_epilogue_control},
 {"actual-reuse-stops-at-branch",1,actual_reuse},
 {"cold-target-reuse-and-blockers",1,reuse_controls},
 {"actual-closed-region-owners",1,actual_region_close},
 {"actual-pending-region-suffix",1,actual_region_suffix},
 {"cold-region-flag-and-pending-reference",1,region_controls},
 {"cold-FCOM-to-FCOMP-with-live-memory-owners",1,reversed_transfer_control}
};
