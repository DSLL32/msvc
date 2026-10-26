/* Target-owned structured loop region, summarized by the real machine pass.
   Compare normalized stream/owner/register state after one internal motion
   request; source/IR/arguments are unchanged from case0002, with owned
   standalone cwd S:vc1.0/c2/region-motion-live replacing the occupied legacy
   reference directory. No synthetic instructions or private state manufacture eligibility. */
#include "../../src/machine-analysis.h"
#include "../../src/instruction-selection.h"
#include "../../src/bitsets.h"
#include <stdio.h>
#include <string.h>
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <unistd.h>
#include <recon-native.h>
#include <recon-fixture.h>
#endif
#include <recon.h>
#define LIMIT 4096
#define WORDS 65536
static MachineInstruction *old[LIMIT],*now[LIMIT];
static MachineFlowRegion *owners[128];
static unsigned report[WORDS],count,old_count,now_count,owner_count;
static MachineOperand *created_source, *rewritten_source;
static unsigned old_source_flags[LIMIT];
static const signed char operand_counts[]={
#define OP(name,n,flags) n,
#include "../../src/machine-opcodes.def"
#undef OP
};
static int put(unsigned value){if(count==WORDS)return 0;report[count++]=value;return 1;}
static unsigned position(MachineInstruction *instruction,MachineInstruction **list,unsigned n)
{
 unsigned i;if(!instruction)return ~0U;
 for(i=0;i<n;++i)if(list[i]==instruction)return i;
 return 0xfffffffeU;
}
static unsigned owner_id(MachineFlowRegion *region)
{
 unsigned i;if(!region)return ~0U;
 for(i=0;i<owner_count;++i)if(owners[i]==region)return i;
 return 0xfffffffeU;
}
static int collect_owners(MachineFlowRegion *region)
{
 unsigned i;
 for(;region;region=region->next){
  if(owner_count==128)return 0;
  for(i=0;i<owner_count;++i)if(owners[i]==region)return 0;
  owners[owner_count++]=region;if(!collect_owners(region->children))return 0;
 }return 1;
}
static int collect_stream(MachineInstruction *head,MachineInstruction **list,unsigned *n)
{
 MachineInstruction *previous=0;unsigned i;*n=0;
 for(;head;head=head->next){
  if(*n==LIMIT||head->previous!=previous)return 0;
  for(i=0;i<*n;++i)if(list[i]==head)return 0;
  list[(*n)++]=head;previous=head;
 }return *n!=0;
}
static int sets(BitSet *set)
{
 unsigned n=0;
 for(;set;set=set->next){if(++n==512||!put(set->base)||!put(set->bits))return 0;}
 return put(0xeeeeeeeeU);
}
static int operand(MachineOperand *value)
{
 MachineAddress *address;unsigned kind=value->flags&15;
 /* Only these two actual products inherit partially initialized stack bits.
    Both have defined kind/width/scale. Real block analysis defines0x7c00;
    the replacement helper additionally preserves the old operand's0x8000.
    Keep every other pre-existing flag word intact. This observer projection
    neither changes internal state nor proves later consumers ignore the bits. */
 if(!put(value==created_source ? value->flags&0x7fffU :
         value==rewritten_source ? value->flags&0xffffU : value->flags))return 0;
 switch(kind){
 case 1:case 2:case 9:return put(value->value.constant);
 case 3:case 4:case 5:case 6:case 7:
  address=value->value.address;if(!address)return 0;
  if(!put(address->first.displacement)||!put(address->symbol?address->symbol->identifier:0))return 0;
  if(kind==4||kind==5)if(!put(address->base.value))return 0;
  return kind!=5||put(address->index.value);
 case 8:return put(value->value.symbol?value->value.symbol->identifier:0);
 default:return 0;
 }
}
static int record_state(MachineInstruction **list,unsigned n)
{
 unsigned i,j,kind,op,k;MachineInstruction *node;MachineBlockHeader *block;
 MachineFlowRegion *region;MachineRegionRegister *reg;
 if(!put(n))return 0;
 for(i=0;i<n;++i){
  node=list[i];kind=node->opcode&15;if(!put(node->opcode))return 0;
  if(!kind||kind==8||kind==9||kind==10){op=(node->opcode>>8)&255;if(op>=MC_OPCODE_COUNT)return 0;
   for(j=0;j<(unsigned)operand_counts[op];++j)if(!operand(&node->operands[j]))return 0;
   if((op==MC_JCC||op==MC_SCC)&&!put(node->operands[1].flags&15))return 0;
   if(op==MC_CALL){MachineCallInfo *call=node->operands[1].value.call;
    if(!call||!put(call->argument_bytes)||!put(call->register_mask))return 0;}

  }else if(kind==1||kind==2){
   if(!node->record.symbol||!put(node->record.symbol->identifier)||
    (kind==1&&!put(((LabelSymbol*)node->record.symbol)->references))||
    !put(position(node->labels.next,list,n))||!put(owner_id(node->labels.attachment))||!put(node->labels.flags&3))return 0;
  }else if(kind==7){
   block=(MachineBlockHeader*)node;
   if(!put(block->number)||!put(block->flags)||!sets(block->uses)||!sets(block->definitions))return 0;
  }else if(kind==4){if(!put(node->record.source_location))return 0;}
  else if(kind==5){
   char *name=node->record.filename;if(!name)return 0;
   for(j=0;name[j]&&j<512;++j)if(!put((unsigned char)name[j]))return 0;
   if(j==512||!put(0))return 0;
  }else if(kind==11){if(!put((unsigned short)node->record.directive.code)||!put((unsigned short)node->record.directive.value))return 0;}
  else if(kind==3){}
  else return 0;
 }
 if(!put(owner_count))return 0;
 for(i=0;i<owner_count;++i){
  region=owners[i];if(!put(region->flags)||!put(owner_id(region->parent))||
   !put(position(region->first,list,n))||!put(position(region->last,list,n))||
   !put(position(region->label,list,n))||!put(position(region->exit_label,list,n))||
   !put(region->first_block)||!put(region->last_block)||!put(region->initial_references))return 0;
  for(j=0;j<8;++j){k=0;for(reg=region->registers[j];reg;reg=reg->next){
   if(++k==512||!put(reg->number)||!put(reg->flags&31)||!put(reg->uses)||!put(reg->definitions))return 0;
   if(reg->definitions==1&&!put(position(reg->definition,list,n)))return 0;
   if((reg->flags&4)&&!put(reg->increment))return 0;
  }if(!put(0xddddddddU))return 0;}
 }return 1;
}
/* The retained outer-region request is a real no-op: its only eligible
   instruction family lies in the nested child skipped by that request. Use
   that genuine child, with its existing preheader and induction summary. */
#ifndef RECON_ORIGINAL_DIRECT
static int expected(int prefix);
#endif
static int exercise(MachineFlowRegion *root,MachineInstruction *head,void (__fastcall *move)(MachineFlowRegion*))
{
 MachineFlowRegion *region;MachineInstruction *preheader,*created=0;unsigned i,j,number,shifted=0,new_nodes=0;
 if(!root||!root->children||!(region=root->children->children)||(region->flags&1)||!region->first||!region->last||
  (region->first->opcode&15)!=7||!region->first->previous||!region->label)return 0;
 preheader=region->first->previous;
 for(i=0;preheader&&(preheader->opcode&15)!=7&&i<LIMIT;++i)preheader=preheader->previous;
 if(!preheader||((MachineBlockHeader*)preheader)->flags&3||
  ((LabelSymbol*)region->label->record.symbol)->references!=region->initial_references)return 0;
 owner_count=count=0;created_source=rewritten_source=0;
 if(!collect_owners(root)||!collect_stream(head,old,&old_count)||!put(0x314f4d52)||
  !put(owner_id(region))||!record_state(old,old_count))return 0;
#ifndef RECON_ORIGINAL_DIRECT
 /* Validate the complete defined live input before any new owner executes. */
 if(!expected(1))return 0;
#endif
 for(i=0;i<old_count;++i){
  old_source_flags[i]=0;
  if(!(old[i]->opcode&15)&&((old[i]->opcode>>8)&255)==MC_MOV)
   old_source_flags[i]=old[i]->operands[1].flags;
 }
 move(region);
 if(!collect_stream(head,now,&now_count))return 0;
 /* Identify the genuine newly inserted preheader LEA by ownership, then
    the existing MOV source actually rewritten to its result. No fixed
    node addresses, allocated numbers or report offsets select the roles. */
 for(i=0;i<now_count;++i)if(position(now[i],old,old_count)==0xfffffffeU){
  if(created||(now[i]->opcode&15)||((now[i]->opcode>>8)&255)!=MC_LEA||
   (now[i]->operands[0].flags&15)!=OPERAND_REGISTER||
   (now[i]->operands[1].flags&15)!=OPERAND_INDEXED)return 0;
  created=now[i];
 }
 if(!created)return 0;
 created_source=&created->operands[1];number=created->operands[0].value.reg.words.number;
 for(i=0;i<now_count;++i){
  MachineOperand *source=&now[i]->operands[1];
  j=position(now[i],old,old_count);
  if(j!=0xfffffffeU&&!(now[i]->opcode&15)&&((now[i]->opcode>>8)&255)==MC_MOV&&
   (source->flags&15)==OPERAND_INDEXED&&source->value.address->base.words.number==number&&
   source->value.address->base.words.flags==1){
   if(rewritten_source||((source->flags^old_source_flags[j])&0x8000))return 0;
   rewritten_source=source;
  }
 }
 if(!rewritten_source||!record_state(now,now_count))return 0;
 for(i=0;i<now_count;++i){j=position(now[i],old,old_count);if(j==0xfffffffeU)++new_nodes;else if(j!=i)++shifted;}
 return put(shifted)&&put(new_nodes);
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *probe)
{
 int ok=exercise(*(MachineFlowRegion**)RECON_ORIGINAL_ADDRESS(probe,0x730c4),
  *(MachineInstruction**)RECON_ORIGINAL_ADDRESS(probe,0x6b940),
  (void (__fastcall *)(MachineFlowRegion*))RECON_ORIGINAL_ADDRESS(probe,0x38b20));
 if(ok)ok=recon_original_emit(probe,"observed.region-motion",report,count*sizeof(unsigned));
 recon_original_finish(probe,ok);
}
#else
extern void combine_machine_instructions(void);
extern int main(int,char **,char **);
static int expected(int prefix)
{
 unsigned words[WORDS],n;FILE *file;char path[1024];const char *assets=recon_case_assets();
 if(!assets||strlen(assets)+30>=sizeof(path))return 0;strcpy(path,assets);strcat(path,"/region-motion.expected");
 file=fopen(path,"rb");if(!file)return 0;n=fread(words,sizeof(unsigned),WORDS,file);
 if(ferror(file)||!feof(file)){fclose(file);return 0;}fclose(file);
 return (prefix?n>=count:n==count)&&!memcmp(report,words,count*sizeof(unsigned));
}
static void at_region(void)
{
 int ok=exercise(machine_region_root,machine_instructions,transform_machine_region_blocks);
 if(ok)ok=expected(0);recon_native_entry_finish(ok);
}
static void real_input(void *unused)
{char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);}
int c2_region_motion(void)
{
 int result=recon_native_at_entry((const void*)combine_machine_instructions,(const void*)at_region,real_input,0);
 recon_record_terminal_status(result==1?0:125);_exit(result==1?0:125);
}
#endif
