/* INTERNAL DIRECT readonly allocation diagnostics at first assign_global_registers.
   Real ledger values, counted loads, numbered work, regions and dominators
   coexist here. No target object/flag/interval is constructed or rewritten.
   Original helper: build-direct direct.c --include ../../../lib/recon/include;
   genuine marker-rva332784. Match abrupt completion via terminal receipt/_exit,
   leaving the actual empty partial scan.obj without application CRT cleanup.
   Native fixtures never invoke Wine; explicit original artifacts stay scratch. */
#include "../../src/global-registers.h"
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
typedef int (__cdecl *Compare)(const void *,const void *);
typedef int (__fastcall *Dominates)(WorkItem *);
#define LIMIT 128
#define WORDS 16384
static AllocationValue *values[LIMIT];static WorkItem *blocks[LIMIT];
static FlowRegion *regions[LIMIT];
static unsigned value_count,block_count,region_count;
static unsigned before[WORDS],after[WORDS],report[2048],report_count;
static int put(unsigned value){if(report_count==2048)return 0;report[report_count++]=value;return 1;}
static unsigned region_id(FlowRegion *region)
{unsigned i;if(!region)return ~0U;for(i=0;i<region_count;++i)if(regions[i]==region)return i;return 0xfffffffeU;}
static unsigned block_id(WorkItem *work)
{unsigned i;if(!work)return ~0U;for(i=0;i<block_count;++i)if(blocks[i]==work)return i;return 0xfffffffeU;}
static int collect(AllocationValue **table,WorkItem *work)
{
 unsigned bucket,i;AllocationValue *value;FlowRegion *region;
 value_count=block_count=region_count=0;
 for(bucket=0;bucket<32;++bucket)for(value=table[bucket];value;value=value->next){
  if(value_count==LIMIT)return 0;
  for(i=0;i<value_count;++i)if(values[i]==value)return 0;values[value_count++]=value;
 }
 for(;work;work=work->next){
  if(block_count==LIMIT)return 0;for(i=0;i<block_count;++i)if(blocks[i]==work)return 0;
  blocks[block_count++]=work;
 }
 for(i=0;i<block_count;++i)for(region=blocks[i]->region;region;region=region->parent){
  if(region_id(region)!=0xfffffffeU)continue;
  if(region_count==LIMIT)return 0;regions[region_count++]=region;
 }
 return value_count>=3&&block_count>3&&region_count;
}
/* Snapshot only initialized owners and active alternatives. Pointer words
   compare within one execution; none are put in the cross-process report. */
static unsigned snapshot(unsigned *out,AllocationValue **table)
{
 unsigned n=0,i,k,links;BitSet *set;AllocationValue *value;AllocationRange *range;
 WorkItem *work;FlowRegion *region;FlowLink *edge;Expression *expression;
#define SAVE(x) do{if(n==WORDS)return 0;out[n++]=(unsigned)(x);}while(0)
#define SET(s) do{set=(s);for(links=0;set;set=set->next){if(++links>512)return 0;SAVE(set);SAVE(set->next);SAVE(set->base);SAVE(set->bits);}SAVE(0xeeeeeeeeU);}while(0)
 for(i=0;i<32;++i)SAVE(table[i]);
 for(i=0;i<value_count;++i){value=values[i];
  SAVE(value);SAVE(value->next);SAVE(value->node);SAVE(value->ranges);
  SAVE(value->global_load_count);SAVE(value->last_block);SAVE(value->diagnostic_flags&3);
  SET(value->definitions);SET(value->registers);
  for(range=value->ranges,k=0;range;range=range->next){if(++k>LIMIT)return 0;
   SAVE(range);SAVE(range->next);SAVE(range->number);SAVE(range->first_block);
   SAVE(range->last_block);SAVE(range->flags&3);SET(range->definitions);
  }SAVE(0xddddddddU);
 }
 for(i=0;i<block_count;++i){work=blocks[i];
  SAVE(work);SAVE(work->next);SAVE(work->previous);SAVE(work->body.expressions);
  SAVE(work->region);SAVE(work->number);SAVE(work->region_depth);SAVE(work->exception_scope);SAVE(work->flags);
  SET(work->analysis.dominators);SET(work->pass.selection.uses);SET(work->pass.selection.definitions);
  for(k=0;k<6;++k){SET(work->flow_sets[k]);}
  for(edge=work->predecessors,k=0;edge;edge=edge->next){if(++k>LIMIT)return 0;SAVE(edge);SAVE(edge->next);SAVE(edge->work);SAVE(edge->flags&1);}SAVE(0xccccccccU);
  for(edge=work->successors,k=0;edge;edge=edge->next){if(++k>LIMIT)return 0;SAVE(edge);SAVE(edge->next);SAVE(edge->work);SAVE(edge->flags&1);}SAVE(0xbbbbbbbbU);
  for(expression=work->body.expressions,k=0;expression;expression=expression->next){if(++k>2048)return 0;
   SAVE(expression);SAVE(expression->next);SAVE(expression->tree);SAVE(expression->source_location);
   SAVE(expression->tree->opcode);SAVE(expression->tree->attributes.value);
  }SAVE(0xaaaaaaaaU);
 }
 for(i=0;i<region_count;++i){region=regions[i];
  SAVE(region);SAVE(region->next);SAVE(region->children);SAVE(region->parent);
  SAVE(region->entry);SAVE(region->header);SAVE(region->last);SAVE(region->flags);
 }
#undef SET
#undef SAVE
 return n;
}
static int exercise(AllocationValue **table,WorkItem *work,Function *function,
 Compare compare,Dominates dominates)
{
 unsigned i,j,n,m,positive=0,negative=0,less=0,equal=0,greater=0;int answer,expected;
 if(!function||!function->symbol||strcmp(function->symbol->name,"_ledger_scan")||!collect(table,work))return 0;
 n=snapshot(before,table);if(!n)return 0;report_count=0;
 if(!put(0x31444152)||!put(value_count)||!put(block_count)||!put(region_count))return 0;
 for(i=0;i<value_count;++i){
  if(!values[i]->node||!values[i]->node->right.symbol||
   !put(values[i]->node->right.symbol->identifier)||!put(values[i]->global_load_count))return 0;
  for(j=0;j<value_count;++j){
   expected=(int)values[j]->global_load_count<(int)values[i]->global_load_count?-1:
    (int)values[j]->global_load_count==(int)values[i]->global_load_count?0:1;
   answer=compare(&values[i],&values[j]);if(answer!=expected||!put(answer))return 0;
   if(answer<0)++less;else if(answer>0)++greater;else ++equal;
  }
 }
 for(i=0;i<block_count;++i){
  answer=dominates(blocks[i]);if(answer!=0&&answer!=1)return 0;
  if(answer)++positive;else ++negative;
  if(!put(blocks[i]->number)||!put(region_id(blocks[i]->region))||!put(answer))return 0;
 }
 if(!less||!equal||!greater||!positive||!negative||!put(less)||!put(equal)||
  !put(greater)||!put(positive)||!put(negative))return 0;
 /* Bind the actual loop-header/predecessor identities and numbers too. */
 for(i=0;i<region_count;++i){FlowLink *edge;unsigned count=0;
  if(!put(regions[i]->flags)||!put(block_id(regions[i]->header)))return 0;
  if(regions[i]->header)for(edge=regions[i]->header->predecessors;edge;edge=edge->next){
   if(++count>LIMIT||!put(block_id(edge->work))||!put(edge->work->number))return 0;
  }if(!put(~0U))return 0;
 }
 m=snapshot(after,table);return m==n&&!memcmp(before,after,n*sizeof(unsigned));
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *probe)
{
 int ok=exercise((AllocationValue**)RECON_ORIGINAL_ADDRESS(probe,0x6b9f0),
  *(WorkItem**)RECON_ORIGINAL_ADDRESS(probe,0x6b948),
  *(Function**)RECON_ORIGINAL_ADDRESS(probe,0x731b4),
  (Compare)RECON_ORIGINAL_ADDRESS(probe,0x53610),
  (Dominates)RECON_ORIGINAL_ADDRESS(probe,0x53410));
 if(ok)ok=recon_original_emit(probe,"observed.allocation-diagnostics",report,report_count*sizeof(unsigned));
 recon_original_finish(probe,ok);
}
#else
extern int main(int,char **,char **);
static int expected(void)
{
 FILE *file;char path[1024];unsigned words[2048],count;const char *assets=recon_case_assets();
 if(!assets||strlen(assets)+40>=sizeof(path))return 0;
 strcpy(path,assets);strcat(path,"/allocation-diagnostics.expected");file=fopen(path,"rb");if(!file)return 0;
 count=fread(words,sizeof(unsigned),2048,file);if(ferror(file)||!feof(file)){fclose(file);return 0;}
 fclose(file);return count==report_count&&!memcmp(report,words,count*sizeof(unsigned));
}
static void at_global(void)
{
 int ok=exercise(allocation_values,current_work,current_function,
  compare_allocation_load_counts,allocation_use_dominates_backedges);
 if(ok)ok=expected();recon_native_entry_finish(ok);
}
static void real_input(void *unused)
{char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);}
int allocation_diagnostics(void)
{
 int result=recon_native_at_entry((const void*)assign_global_registers,(const void*)at_global,real_input,0);
 recon_record_terminal_status(result==1?0:125);_exit(result==1?0:125);
}
#endif
