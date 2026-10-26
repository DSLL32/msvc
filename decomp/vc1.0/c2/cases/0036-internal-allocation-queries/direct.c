/* INTERNAL DIRECT: borrow initialized decoder allocation state at the existing
   eviction-query marker. Query inputs are actual owners/uses, never made-up
   compiler graphs; the interval adapter is allowed only an already-covered use. */
#include "../../src/physical-allocation.h"
#include "../../src/scheduler.h"
#include "../../src/float-machine.h"
#include "../../src/functions.h"
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
typedef int (__fastcall *PairQuery)(RegisterVariable *,RegisterVariable *);
typedef int (__fastcall *FlagsQuery)(MachineInstruction *);
typedef void (__fastcall *IntervalUse)(unsigned short,int);
#define OWNER_LIMIT 256
#define INSTRUCTION_LIMIT 2048
#define WORD_LIMIT 32768
static RegisterVariable *owners[OWNER_LIMIT];
static MachineInstruction *instructions[INSTRUCTION_LIMIT];
static WorkItem *blocks[256];
static unsigned owner_count,instruction_count,block_count;
static unsigned before[WORD_LIMIT],after[WORD_LIMIT],report[4096],report_count;
/* Existing contextual opcode descriptions give only valid operand counts and
   independently check immediate flag-reader/writer witnesses in the probe. */
static const signed char operand_counts[]={
#define OP(name,count,properties) count,
#include "../../src/machine-opcodes.def"
#undef OP
};
static const unsigned char properties[]={
#define OP(name,count,properties) properties,
#include "../../src/machine-opcodes.def"
#undef OP
};
static int put(unsigned value)
{
 if(report_count==4096)return 0;report[report_count++]=value;return 1;
}
static unsigned owner_id(RegisterVariable *variable)
{
 unsigned i;for(i=0;i<owner_count;++i)if(owners[i]==variable)return i;
 return 0xffffffffU;
}
static RegisterVariable *number_owner(unsigned short number)
{
 unsigned i;for(i=0;i<owner_count;++i)if(owners[i]->number==number)return owners[i];
 return 0;
}
static int collect(RegisterVariable *(*buckets)[32],WorkItem *work)
{
 unsigned g,b,i,n;RegisterVariable *v;MachineInstruction *instruction;
 owner_count=instruction_count=block_count=0;
 for(g=0;g<3;++g)for(b=0;b<32;++b)for(v=buckets[g][b];v;v=v->next){
  if(owner_count==OWNER_LIMIT||owner_id(v)!=0xffffffffU||v->number>>14!=g||(v->number&31)!=b)return 0;
  owners[owner_count++]=v;
 }
 for(;work;work=work->next){
  if(block_count==256)return 0;blocks[block_count++]=work;
  instruction=work->body.block->next;
  for(n=0;(instruction->opcode&15)!=7;++n){
   if(n==INSTRUCTION_LIMIT||instruction_count==INSTRUCTION_LIMIT)return 0;
   for(i=0;i<instruction_count;++i)if(instructions[i]==instruction)return 0;
   instructions[instruction_count++]=instruction;instruction=instruction->next;
  }
 }
 return owner_count&&instruction_count;
}
/* Constructor-defined variable fields and conflict endpoints are initialized.
   Machine snapshots omit phase-dependent record payloads and unused operands;
   links/opcode and actually present operands are defined at this anchor. */
static unsigned snapshot(unsigned *words)
{
 unsigned n=0,i,j,chain,operation;RegisterVariable *v;RegisterConflict *edge;
 MachineInstruction *instruction;WorkItem *work;
#define SAVE(value) do{if(n==WORD_LIMIT)return 0;words[n++]=(unsigned)(value);}while(0)
 for(i=0;i<owner_count;++i){
  v=owners[i];SAVE(v);SAVE(v->next);SAVE(v->work_next);SAVE(v->conflicts);
  SAVE(v->spill_priority);SAVE(v->spill_weight);SAVE(v->degree);SAVE(v->spill_cost);
  SAVE(v->number);SAVE(v->representative);SAVE(v->flags);SAVE(v->rematerialization);
  SAVE(v->first_block);SAVE(v->last_block);SAVE(v->storage_bytes);
  SAVE(v->home_symbol);SAVE(v->home_offset);chain=0;
  for(edge=v->conflicts;edge;edge=edge->next){
   if(++chain>OWNER_LIMIT||owner_id(edge->variable)==0xffffffffU)return 0;
   SAVE(edge);SAVE(edge->next);SAVE(edge->variable);
  }
  if(chain!=(unsigned)v->degree)return 0;
 }
 for(i=0;i<block_count;++i){
  work=blocks[i];SAVE(work);SAVE(work->next);SAVE(work->previous);SAVE(work->number);SAVE(work->body.block);
 }
 for(i=0;i<instruction_count;++i){
  instruction=instructions[i];SAVE(instruction);SAVE(instruction->next);SAVE(instruction->previous);SAVE(instruction->opcode);
  if((instruction->opcode&15)==0){
   operation=(instruction->opcode>>8)&255;if(operation>=MC_OPCODE_COUNT)return 0;
   for(j=0;j<(unsigned)operand_counts[operation];++j){
    if(j==2)return 0;SAVE(instruction->operands[j].flags);SAVE(instruction->operands[j].value.constant);
   }
   if(operation==MC_JMP&&(instruction->operands[0].flags&15)==8){
    LabelSymbol *label=(LabelSymbol *)instruction->operands[0].value.symbol;
    SAVE(label->instruction);SAVE(label->references);
   }
  }
 }
#undef SAVE
 return n;
}
static int query(unsigned short number,MachineFlowRegion *region,
 RegisterVariable *(*buckets)[32],WorkItem *work,PairQuery common,PairQuery compatible,
 FlagsQuery flags_live,IntervalUse include)
{
 unsigned i,j,n,m,edges=0,protected_count=0,positive=0,negative=0,checked=0,operation;
 RegisterVariable *zero=0,*v,*covered=0,*left=0,*right=0,*witness=0;
 RegisterConflict *a,*b;MachineInstruction *instruction,*next;
 WorkItem *covered_work=0;int result,self,empty;
 if(number!=0x402f||!region||(region->flags&0x7f)!=0x50||!collect(buckets,work))return 0;
 report_count=0;n=snapshot(before);if(!n)return 0;
 for(i=0;i<owner_count;++i){
  v=owners[i];edges+=v->degree;if(!v->degree&&!zero)zero=v;
  if((v->number&0x4000)&&(v->flags&0x300))++protected_count;
 }
 if(!zero||!edges||!protected_count||!put(0x31514152)||!put(owner_count)||!put(edges)||!put(protected_count))return 0;
 /* Prefer a distinct real pair sharing a third conflict owner. Selection
    proves a positive witness, without independently calculating the result. */
 for(i=0;i<owner_count&&!witness;++i)for(j=i+1;j<owner_count&&!witness;++j){
  for(a=owners[i]->conflicts;a&&!witness;a=a->next){
   if(a->variable==owners[j])continue;
   for(b=owners[j]->conflicts;b;b=b->next)if(a->variable->number==b->variable->number){left=owners[i];right=owners[j];witness=a->variable;break;}
  }
 }
 if(!witness)return 0;result=common(left,right);
 if(result<=0||!put(owner_id(left))||!put(owner_id(right))||!put(owner_id(witness))||!put(result))return 0;
 for(i=0;i<owner_count;++i){
  v=owners[i];self=common(v,v);empty=common(v,zero);
  /* Actual conflict graph has no self edges; this positive/self and zero
     degree witness checks independent relationships without a mirror model. */
  if(self!=v->degree||empty!=0)return 0;
  if(!put(v->number)||!put(self)||!put(empty))return 0;
 }
 for(i=0;i<owner_count;++i){
  v=owners[i];result=compatible(v,zero);
  if(result!=!((v->number&0x4000)&&(v->flags&0x300))||compatible(zero,v)!=result)return 0;
  if(!put(result))return 0;
 }
 if(!put(block_count)||!put(instruction_count))return 0;
 for(i=0;i<instruction_count;++i){
  instruction=instructions[i];result=flags_live(instruction);
  if(result!=0&&result!=1)return 0;if(result)++positive;else ++negative;
  next=instruction->next;
  if(next&&(next->opcode&15)==0){
   operation=(next->opcode>>8)&255;
   if(properties[operation]&MC_READS_FLAGS){if(!result)return 0;++checked;}
   else if(properties[operation]&MC_WRITES_FLAGS){if(result)return 0;++checked;}
  }
  if(!put(i)||!put(result))return 0;
 }
 if(!positive||!negative||!checked||!put(positive)||!put(negative)||!put(checked))return 0;
 /* The first actual direct virtual-register operand use already contained by
    its real owner interval is the only mutating API call. No new owner/range
    is fabricated and the full defined-state snapshot must remain identical. */
 for(i=0;i<block_count&&!covered;++i){
  instruction=blocks[i]->body.block->next;
  while((instruction->opcode&15)!=7&&!covered){
   if((instruction->opcode&15)==0){
    operation=(instruction->opcode>>8)&255;
    for(j=0;j<(unsigned)operand_counts[operation]&&!covered;++j){
     if((instruction->operands[j].flags&15)==1){
      number=instruction->operands[j].value.reg.words.number;
      v=number_owner(number);
      if((number&0xc000)&&v&&v->first_block<=blocks[i]->number&&v->last_block>=blocks[i]->number){covered=v;covered_work=blocks[i];}
     }
    }
   }
   instruction=instruction->next;
  }
 }
 if(!covered||!put(covered->number)||!put(covered_work->number)||!put(covered->first_block)||!put(covered->last_block))return 0;
 include(covered->number,covered_work->number);
 m=snapshot(after);return m==n&&!memcmp(before,after,n*sizeof(unsigned));
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *probe)
{
 int ok=query((unsigned short)probe->original_ecx,(MachineFlowRegion *)probe->original_edx,
  (RegisterVariable *(*)[32])RECON_ORIGINAL_ADDRESS(probe,0x6c730),
  *(WorkItem **)RECON_ORIGINAL_ADDRESS(probe,0x6b948),
  (PairQuery)RECON_ORIGINAL_ADDRESS(probe,0x1e450),
  (PairQuery)RECON_ORIGINAL_ADDRESS(probe,0x1d000),
  (FlagsQuery)RECON_ORIGINAL_ADDRESS(probe,0x37f70),
  (IntervalUse)RECON_ORIGINAL_ADDRESS(probe,0x1d040));
 if(ok)ok=recon_original_emit(probe,"observed.allocation-queries",report,report_count*sizeof(unsigned));
 recon_original_finish(probe,ok);
}
#else
extern int main(int,char **,char **);
static int expected(void)
{
 FILE *file;char path[1024];unsigned words[4096],n;const char *assets=recon_case_assets();
 if(!assets||strlen(assets)+36>=sizeof(path))return 0;
 strcpy(path,assets);strcat(path,"/allocation-queries.expected");
 file=fopen(path,"rb");if(!file)return 0;n=fread(words,sizeof(unsigned),4096,file);
 if(ferror(file)||!feof(file)){fclose(file);return 0;}fclose(file);
 return n==report_count&&!memcmp(words,report,n*sizeof(unsigned));
}
static MachineFlowRegion *__fastcall at_region(unsigned short number,MachineFlowRegion *region)
{
 int ok=query(number,region,register_variable_buckets,current_work,
  count_shared_register_conflicts,register_spill_homes_compatible,pentium_flags_live_after,include_register_block_use);
 if(ok)ok=expected();recon_native_entry_finish(ok);return 0;
}
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);
}
int c2_allocation_queries(void)
{
 int result=recon_native_at_entry((const void*)find_evictable_machine_region,(const void*)at_region,real_input,0);
 recon_record_terminal_status(result==1?0:125);_exit(result==1?0:125);
}
#endif
