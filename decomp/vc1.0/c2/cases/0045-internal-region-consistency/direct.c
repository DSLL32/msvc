/* INTERNAL DIRECT readonly replacement consistency over genuine global-bank
   machine state. The scaled-index hypothesis missed; preserve that distinction.
   Preparation uses the existing target API, never raw global assignment.
   Original helper: recon-tool original build-direct thisfile --toolchain 1.0-nt
   --include ../../../lib/recon/include; marker RVA242512 (combine entry).
   Native replay takes over the same live owner and completes abruptly, retaining
   the genuine empty partial bank.obj rather than invoking compiler cleanup. */
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
#include "../../src/machine-region-motion.h"
#include "../../src/instruction-rewrite.h"
#include "../../src/transformation-cursor.h"
#include <recon.h>
#define LIMIT 4096
#define WORDS 65536
static MachineInstruction *old[LIMIT],*now[LIMIT];
static MachineFlowRegion *owners[128];
static unsigned report[WORDS],after[WORDS],*destination=report,count,old_count,now_count,owner_count;
static const signed char operand_counts[]={
#define OP(name,n,flags) n,
#include "../../src/machine-opcodes.def"
#undef OP
};
static int put(unsigned value){if(count==WORDS)return 0;destination[count++]=value;return 1;}
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
 if(!put(value->flags))return 0;
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
typedef void (__fastcall *Prepare)(MachineFlowRegion *);
typedef int (__fastcall *Query)(MachineInstruction *,unsigned short,MachineOperand *);
typedef int (__fastcall *Mentions)(MachineOperand *,unsigned short);
static int exercise(MachineFlowRegion *root,MachineInstruction *head,Function *function,
 Prepare prepare,Query query,Mentions mentions,MachineFlowRegion **active,
 MachineBlockHeader **insertion,MachineInstruction **cursor)
{
 MachineFlowRegion *region;MachineInstruction *first,*scan,*saved_cursor;
 MachineBlockHeader *saved_insertion;MachineOperand *value,*use;
 unsigned i,j,op,k,n,first_count,registers,addresses,queries=0,positive=0,negative=0;
 unsigned register_only=0,mixed=0,state_count,result_count;unsigned short number;int answer;
 if(!root||!(region=root->children)||region->next||(region->flags&1)||
  !function||!function->symbol||strcmp(function->symbol->name,"_global_bank_checksum"))return 0;
 prepare(region);saved_insertion=*insertion;saved_cursor=*cursor;
 if(*active!=region||!saved_insertion||(saved_insertion->opcode&15)!=7||!saved_cursor)return 0;
 owner_count=count=0;destination=report;
 if(!collect_owners(root)||!collect_stream(head,old,&old_count)||
  !put(0x31514352)||!put(owner_id(region)))return 0;
 state_count=count;if(!record_state(old,old_count))return 0;state_count=count-state_count;
 if(!put(position((MachineInstruction*)saved_insertion,old,old_count))||!put(position(saved_cursor,old,old_count)))return 0;
 for(first=region->first;first!=region->last->next;first=first->next){
  k=first->opcode&15;if(k&&k!=8&&k!=9&&k!=10)continue;
  first_count=operand_counts[(first->opcode>>8)&255];
  for(i=0;i<first_count&&i<2;++i){
   value=&first->operands[i];k=value->flags&15;
   if(k==OPERAND_REGISTER)number=value->value.reg.words.number;
   else if(k==OPERAND_BASED||k==OPERAND_INDEXED)number=value->value.address->base.words.number;
   else continue;if(!(number&0xc000))continue;
   /* These witnesses establish actual register-only and mixed-role queries;
      they do not implement the general replacement-consistency predicate. */
   registers=addresses=0;
   for(scan=first,n=0;scan!=region->last->next;scan=scan->next){
    if(++n==LIMIT)return 0;k=scan->opcode&15;if(k&&k!=8&&k!=9&&k!=10)continue;
    op=(scan->opcode>>8)&255;
    for(j=0;j<(unsigned)operand_counts[op]&&j<2;++j){
     use=&scan->operands[j];if(!mentions(use,number))continue;
     if((use->flags&15)==OPERAND_REGISTER)++registers;else ++addresses;
    }
   }
   answer=query(first,number,value);if(answer!=0&&answer!=1)return 0;
   if((value->flags&15)==OPERAND_REGISTER&&registers&&!addresses){
    ++register_only;if(!answer)return 0;
   }
   if((value->flags&15)!=OPERAND_REGISTER&&registers&&addresses){
    ++mixed;if(answer)return 0;
   }
   ++queries;if(answer)++positive;else ++negative;
   if(!put(position(first,old,old_count))||!put(i)||!put(value->flags&15)||
    !put(number)||!put(registers)||!put(addresses)||!put(answer))return 0;
  }
 }
 if(queries!=37||register_only!=29||mixed!=3||!positive||!negative)return 0;
 result_count=count;destination=after;count=0;
 if(!collect_stream(head,now,&now_count)||now_count!=old_count||
  memcmp(old,now,old_count*sizeof(*old))||!record_state(now,now_count)||count!=state_count||
  memcmp(report+2,after,state_count*sizeof(unsigned))||*active!=region||
  *insertion!=saved_insertion||*cursor!=saved_cursor)return 0;
 destination=report;count=result_count;
 return put(queries)&&put(positive)&&put(negative)&&put(register_only)&&put(mixed)&&put(1);
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *probe)
{
 int ok=exercise(*(MachineFlowRegion**)RECON_ORIGINAL_ADDRESS(probe,0x730c4),
  *(MachineInstruction**)RECON_ORIGINAL_ADDRESS(probe,0x6b940),
  *(Function**)RECON_ORIGINAL_ADDRESS(probe,0x731b4),
  (Prepare)RECON_ORIGINAL_ADDRESS(probe,0x39230),
  (Query)RECON_ORIGINAL_ADDRESS(probe,0x3a820),
  (Mentions)RECON_ORIGINAL_ADDRESS(probe,0x11fd0),
  (MachineFlowRegion**)RECON_ORIGINAL_ADDRESS(probe,0x730c8),
  (MachineBlockHeader**)RECON_ORIGINAL_ADDRESS(probe,0x730d4),
  (MachineInstruction**)RECON_ORIGINAL_ADDRESS(probe,0x730d0));
 if(ok)ok=recon_original_emit(probe,"observed.region-consistency",report,count*sizeof(unsigned));
 recon_original_finish(probe,ok);
}
#else
extern int main(int,char **,char **);extern void combine_machine_instructions(void);
static int expected(void)
{
 unsigned words[WORDS],n;FILE *file;char path[1024];const char *assets=recon_case_assets();
 if(!assets||strlen(assets)+40>=sizeof(path))return 0;
 strcpy(path,assets);strcat(path,"/region-consistency.expected");file=fopen(path,"rb");if(!file)return 0;
 n=fread(words,sizeof(unsigned),WORDS,file);if(ferror(file)||!feof(file)){fclose(file);return 0;}
 fclose(file);return n==count&&!memcmp(words,report,n*sizeof(unsigned));
}
static void at_combine(void)
{
 int ok=exercise(machine_region_root,machine_instructions,current_function,
  prepare_region_insertion,region_operand_replacement_safe,operand_mentions_register,
  &active_machine_region,&region_insertion_block,&transformation_cursor.instruction);
 if(ok)ok=expected();recon_native_entry_finish(ok);
}
static void invoke(void *unused)
{char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);}
int region_consistency(void)
{
 int result=recon_native_at_entry((const void*)combine_machine_instructions,(const void*)at_combine,invoke,0);
 recon_record_terminal_status(result==1?0:125);_exit(result==1?0:125);
}
#endif
