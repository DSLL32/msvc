/* INTERNAL DIRECT exit split at the genuine decoder eviction lookup.
   Only the target APIs insert/register a label and transfer real edge/owner
   references; other defined borrowed state is preserved. No synthetic graph
   or private flag/global assignment manufactures public command-line reach. */
#include "../../src/machine-analysis.h"
#include <stdio.h>
#include <string.h>
#ifdef RECON_ORIGINAL_DIRECT
#include <recon-original-direct.h>
#else
#include <unistd.h>
#include <recon-native.h>
#include <recon-fixture.h>
#include "../../src/physical-rewrite.h"
#endif
#include <recon.h>
typedef MachineInstruction *(__fastcall *ExitSplit)(MachineFlowRegion *);
#define QUERY_WORDS 256
#define SNAPSHOT_WORDS 16384
static unsigned report[QUERY_WORDS],report_count;
static unsigned before[SNAPSHOT_WORDS],after[SNAPSHOT_WORDS];
static MachineFlowRegion *owners[128];
static unsigned owner_count;
static MachineInstruction *old_exit,*new_exit;
static unsigned allowed_exit_alias[128];
/* Stable traversal order supplies relationship IDs only. Snapshot addresses
   are compared within one process, never to another process's pointers. */
static int collect(MachineFlowRegion *region)
{
 unsigned i;
 for(;region;region=region->next){
  if(owner_count==128)return 0;
  for(i=0;i<owner_count;++i)if(owners[i]==region)return 0;
  owners[owner_count++]=region;
  if(!collect(region->children))return 0;
 }
 return 1;
}
static unsigned owner_id(MachineFlowRegion *region)
{
 unsigned i;if(!region)return 0xffffffffU;
 for(i=0;i<owner_count;++i)if(owners[i]==region)return i;
 return 0xfffffffeU;
}
/* Only constructor-defined links, counters, and interpreted flag bits are
   read. Skip unused field06/field1c/padding and increment unless flag4 owns it. */
static unsigned snapshot(unsigned *words)
{
 unsigned n=0,i,j,chain;
 MachineFlowRegion *region;MachineRegionRegister *record;
#define SAVE(value) do{if(n==SNAPSHOT_WORDS)return 0;words[n++]=(unsigned)(value);}while(0)
 for(i=0;i<owner_count;++i){
  region=owners[i];SAVE(region);SAVE(region->parent);SAVE(region->next);SAVE(region->children);
  SAVE(region->first);SAVE(region->last);SAVE(region->label);SAVE(new_exit&&allowed_exit_alias[i]&&region->exit_label==new_exit?old_exit:region->exit_label);
  SAVE(region->flags&0x7f);SAVE(region->first_block);SAVE(region->last_block);
  SAVE(region->pending_references);SAVE(region->initial_references);
  for(j=0;j<3;++j)SAVE(region->references[j]);
  for(j=0;j<8;++j){
   SAVE(region->registers[j]);chain=0;
   for(record=region->registers[j];record;record=record->next){
    if(++chain>256)return 0;
    SAVE(record);SAVE(record->next);SAVE(record->number);SAVE(record->flags&0x1f);
    SAVE(record->uses);SAVE(record->definitions);SAVE(record->definition);
    if(record->flags&4)SAVE(record->increment);
   }
  }
 }
#undef SAVE
 return n;
}
static int put(unsigned value)
{
 if(report_count==QUERY_WORDS)return 0;report[report_count++]=value;return 1;
}
#define RECORD_LIMIT 2048
typedef struct BorrowedRecord {
 MachineInstruction *instruction;
 unsigned opcode,flags[2],values[2];
 LabelSymbol *symbol;
 MachineInstruction *label_next,*label_instruction;
 MachineFlowRegion *attachment;
 long references;
 unsigned label_flags;
} BorrowedRecord;
static BorrowedRecord records[RECORD_LIMIT];
static unsigned record_count;
static const signed char operand_counts[]={
#define OP(name,count,properties) count,
#include "../../src/machine-opcodes.def"
#undef OP
};
static int in_range(MachineInstruction *instruction,MachineFlowRegion *region)
{
 MachineInstruction *scan;unsigned n=0;
 for(scan=region->first;scan!=region->last->next;scan=scan->next){
  if(++n==RECORD_LIMIT)return 0;if(scan==instruction)return 1;
 }
 return 0;
}
static int save_records(MachineInstruction *first)
{
 BorrowedRecord *record;unsigned i,operation;MachineInstruction *instruction;
 record_count=0;
 for(instruction=first;instruction;instruction=instruction->next){
  if(record_count==RECORD_LIMIT)return 0;record=&records[record_count++];
  record->instruction=instruction;record->opcode=instruction->opcode;record->symbol=0;
  if((instruction->opcode&15)==0){
   operation=(instruction->opcode>>8)&255;if(operation>=MC_OPCODE_COUNT)return 0;
   for(i=0;i<(unsigned)operand_counts[operation];++i){
    if(i==2)return 0;record->flags[i]=instruction->operands[i].flags;
    record->values[i]=instruction->operands[i].value.constant;
   }
  }else if((instruction->opcode&15)==1){
   record->symbol=(LabelSymbol *)instruction->record.symbol;
   record->references=record->symbol->references;record->label_instruction=record->symbol->instruction;
   record->label_next=instruction->labels.next;record->attachment=instruction->labels.attachment;
   record->label_flags=instruction->labels.flags&3;
  }
 }
 return record_count!=0;
}
static int query(unsigned short number,MachineFlowRegion *region,MachineContext *context,
 int *cleanup,ExitSplit split)
{
 MachineFlowRegion *root=region;MachineInstruction *head,*scan,*label_head;
 LabelSymbol *old_symbol,*new_symbol;BorrowedRecord *record;
 unsigned i,j,n,m,depth=0,aliases=0,changes=0,new_records=0,new_position=0;
 long references;unsigned cleanup_before=*cleanup;
 if(number!=0x402f||!region||(region->flags&0x7f)!=0x50||!region->exit_label||
  !context||(context->opcode&15)!=2)return 0;
 while(root->parent){if(++depth>64)return 0;root=root->parent;}
 owner_count=0;report_count=0;new_exit=0;old_exit=region->exit_label;
 old_symbol=(LabelSymbol *)old_exit->record.symbol;references=old_symbol->references;
 if(references!=1||old_symbol->instruction!=old_exit||!collect(root))return 0;
 for(i=0;i<owner_count;++i){
  allowed_exit_alias[i]=(owners[i]->flags&0x10)&&owners[i]->exit_label==old_exit&&in_range(owners[i]->label,region);
  if(allowed_exit_alias[i])++aliases;
 }
 if(!aliases)return 0;n=snapshot(before);if(!n)return 0;
 head=context;depth=0;while(head->previous){if(++depth==RECORD_LIMIT)return 0;head=head->previous;}
 if(!save_records(head))return 0;label_head=context->labels.next;
 new_exit=split(region);if(!new_exit||new_exit==old_exit||region->exit_label!=new_exit)return 0;
 new_symbol=(LabelSymbol *)new_exit->record.symbol;
 if((new_exit->opcode&15)!=1||(new_exit->labels.flags&2)==0||new_exit->labels.attachment||
  new_symbol==old_symbol||new_symbol->instruction!=new_exit||context->labels.next!=new_exit||
  new_exit->labels.next!=label_head||old_symbol->references!=0||new_symbol->references!=1||*cleanup!=1)return 0;
 scan=head;
 for(i=0;i<record_count;++i){
  record=&records[i];
  if(scan==new_exit){new_position=i;++new_records;scan=scan->next;}
  if(scan!=record->instruction||scan->opcode!=record->opcode||
   (scan->previous&&scan->previous->next!=scan)||(scan->next&&scan->next->previous!=scan))return 0;
  if((scan->opcode&15)==0){
   for(j=0;j<(unsigned)operand_counts[(scan->opcode>>8)&255];++j){
    if(scan->operands[j].flags!=record->flags[j])return 0;
    if(scan->operands[j].value.constant!=record->values[j]){
     if(j||record->values[j]!=(unsigned)&old_symbol->symbol||
      scan->operands[j].value.symbol!=&new_symbol->symbol||
      (scan->operands[j].flags&15)!=8||!in_range(scan,region))return 0;
     ++changes;
    }
   }
  }else if((scan->opcode&15)==1){
   if(scan->record.symbol!=&record->symbol->symbol||scan->labels.next!=record->label_next||
    scan->labels.attachment!=record->attachment||(scan->labels.flags&3)!=record->label_flags||
    record->symbol->instruction!=record->label_instruction||
    record->symbol->references!=(record->symbol==old_symbol?0:record->references))return 0;
  }
  scan=scan->next;
 }
 if(scan==new_exit){new_position=record_count;++new_records;scan=scan->next;}
 m=snapshot(after);
 if(scan||new_records!=1||changes!=1||m!=n||memcmp(before,after,n*sizeof(unsigned)))return 0;
 return put(0x31584552)&&put(owner_count)&&put(owner_id(region))&&put(record_count)&&
  put(aliases)&&put(changes)&&put(new_position)&&put(references)&&put(old_symbol->references)&&
  put(new_symbol->references)&&put(cleanup_before)&&put(*cleanup);
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *probe)
{
 int ok=query((unsigned short)probe->original_ecx,(MachineFlowRegion *)probe->original_edx,
  *(MachineContext **)RECON_ORIGINAL_ADDRESS(probe,0x6b944),
  (int *)RECON_ORIGINAL_ADDRESS(probe,0x6c29c),
  (ExitSplit)RECON_ORIGINAL_ADDRESS(probe,0x38ee0));
 if(ok)ok=recon_original_emit(probe,"observed.region-exit",report,report_count*sizeof(unsigned));
 recon_original_finish(probe,ok);
}
#else
extern int main(int,char **,char **);
static int expected(void)
{
 FILE *file;char path[1024];unsigned words[QUERY_WORDS],n;const char *assets=recon_case_assets();
 if(!assets||strlen(assets)+28>=sizeof(path))return 0;
 strcpy(path,assets);strcat(path,"/region-exit.expected");
 file=fopen(path,"rb");if(!file)return 0;n=fread(words,sizeof(unsigned),QUERY_WORDS,file);
 if(ferror(file)||!feof(file)){fclose(file);return 0;}fclose(file);
 return n==report_count&&!memcmp(words,report,n*sizeof(unsigned));
}
static MachineFlowRegion *__fastcall at_region(unsigned short number,MachineFlowRegion *region)
{
 int ok=query(number,region,machine_context,&machine_analysis_needs_label_cleanup,split_machine_region_exit);
 if(ok)ok=expected();recon_native_entry_finish(ok);return 0;
}
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);
}
int c2_region_exit(void)
{
 int result=recon_native_at_entry((const void*)find_evictable_machine_region,(const void*)at_region,real_input,0);
 recon_record_terminal_status(result==1?0:125);_exit(result==1?0:125);
}
#endif
