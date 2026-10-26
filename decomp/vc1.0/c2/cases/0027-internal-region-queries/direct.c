/* INTERNAL DIRECT queries at the first genuine decoder eviction lookup.
   Borrowed compiler objects remain untouched; no synthetic graph or private
   flag/global change is used to manufacture ordinary command-line reach. */
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
typedef int (__fastcall *UsesQuery)(unsigned short,MachineFlowRegion *);
typedef int (__fastcall *AncestorQuery)(MachineFlowRegion *,MachineFlowRegion *);
typedef MachineFlowRegion *(__fastcall *PreservationQuery)(unsigned short,MachineFlowRegion *);
#define QUERY_WORDS 256
#define SNAPSHOT_WORDS 16384
static unsigned report[QUERY_WORDS],report_count;
static unsigned before[SNAPSHOT_WORDS],after[SNAPSHOT_WORDS];
static MachineFlowRegion *owners[128];
static unsigned owner_count;
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
  SAVE(region->first);SAVE(region->last);SAVE(region->label);SAVE(region->exit_label);
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
static int query(unsigned short number,MachineFlowRegion *region,
 UsesQuery uses,AncestorQuery ancestor,PreservationQuery preservation)
{
 MachineFlowRegion *root=region,*found;MachineRegionRegister *record;
 unsigned i,count=0,n,m,depth=0;
 if(number!=0x402f||!region||!region->parent||(region->flags&0x7f)!=0x50)return 0;
 while(root->parent){if(++depth>64)return 0;root=root->parent;}
 owner_count=0;report_count=0;
 if(!collect(root))return 0;n=snapshot(before);if(!n)return 0;
 if(!put(0x31514752)||!put(owner_count)||!put(owner_id(region))||!put(number))return 0;
 if(!put(uses(number,region))||!put(uses(number,region->children)))return 0;
 if(!put(ancestor(root,region))||!put(ancestor(region,root))||!put(ancestor(region,region)))return 0;
 found=preservation(number,region);if(owner_id(found)==0xfffffffeU||!put(owner_id(found)))return 0;
 /* Existing summaries supply the additional real identifiers. The16-entry
    bound defines this probe's scope, not a modified compiler limit. */
 for(i=0;i<8&&count<16;++i){
  for(record=region->registers[i];record&&count<16;record=record->next){
   if(!put(record->number)||!put(uses(record->number,root)))return 0;
   found=preservation(record->number,region);
   if(owner_id(found)==0xfffffffeU||!put(owner_id(found)))return 0;
   ++count;
  }
 }
 if(!put(count))return 0;
 m=snapshot(after);
 return count&&m==n&&!memcmp(before,after,n*sizeof(unsigned));
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *probe)
{
 int ok=query((unsigned short)probe->original_ecx,(MachineFlowRegion *)probe->original_edx,
  (UsesQuery)RECON_ORIGINAL_ADDRESS(probe,0x3aea0),
  (AncestorQuery)RECON_ORIGINAL_ADDRESS(probe,0x3b1e0),
  (PreservationQuery)RECON_ORIGINAL_ADDRESS(probe,0x3b280));
 if(ok)ok=recon_original_emit(probe,"observed.region-queries",report,report_count*sizeof(unsigned));
 recon_original_finish(probe,ok);
}
#else
extern int main(int,char **,char **);
static int expected(void)
{
 FILE *file;char path[1024];unsigned words[QUERY_WORDS],n;const char *assets=recon_case_assets();
 if(!assets||strlen(assets)+32>=sizeof(path))return 0;
 strcpy(path,assets);strcat(path,"/region-queries.expected");
 file=fopen(path,"rb");if(!file)return 0;
 n=fread(words,sizeof(unsigned),QUERY_WORDS,file);
 if(ferror(file)||!feof(file)){fclose(file);return 0;}fclose(file);
 return n==report_count&&!memcmp(words,report,n*sizeof(unsigned));
}
static MachineFlowRegion *__fastcall at_region(unsigned short number,MachineFlowRegion *region)
{
 int ok=query(number,region,register_has_region_uses,machine_region_is_ancestor,find_register_preservation_region);
 if(ok)ok=expected();recon_native_entry_finish(ok);return 0;
}
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);
}
int c2_region_queries(void)
{
 int result=recon_native_at_entry((const void*)find_evictable_machine_region,
  (const void*)at_region,real_input,0);
 /* The original typed probe terminates without App CRT cleanup. Match that
    boundary; a normal return would delete the still-incomplete object. */
 recon_record_terminal_status(result==1?0:125);
 _exit(result==1?0:125);
}
#endif
