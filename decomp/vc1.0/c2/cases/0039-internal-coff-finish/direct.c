/* INTERNAL writer API scenario borrowing actual DLL-adapter symbol owners.
   No private flags, globals or imported descriptors are rewritten. The new
   section and weak fallback are explicit internal requests, not claimed C1 IR. */
#include "../../src/coff-writer.h"
#include "../../src/records.h"
#include "../../src/output.h"
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
typedef SymbolHeader *(__fastcall *LookupAlias)(SymbolHeader *);
typedef void (__fastcall *Reference)(unsigned,SymbolHeader *);
typedef struct WriterOps {
 LookupAlias lookup;
 unsigned (__fastcall *weak)(char *,unsigned short,unsigned char,unsigned,unsigned short,unsigned,unsigned char);
 unsigned short (__fastcall *create)(char *,unsigned,int);
 int (__fastcall *open)(unsigned short,unsigned);
 unsigned short (__fastcall *select)(unsigned short);
 void *(__fastcall *get)(CoffVector *,unsigned);
 Reference segment,address;
 void (__fastcall *data)(void *,unsigned);
 void (*debug)(void),(*finish)(void);
 int (__cdecl *flush)(FILE *);
 CoffVector **symbols,**sections;
 int *object_started;
} WriterOps;
static unsigned report[64],count;
static int put(unsigned value){if(count==64)return 0;report[count++]=value;return 1;}
static unsigned header_fields(SymbolHeader *s,unsigned *p)
{
 p[0]=(unsigned)s->name;p[1]=s->kind;p[2]=s->identifier;
 p[3]=s->field14;p[4]=s->field04;p[5]=s->field0e;
 return 6;
}
static int finish(ImportedSymbolAlias *aliases,WriterOps *op)
{
 ImportedSymbolAlias *a,*saved[16],*picked=0;
 unsigned before[16][12],after[12],i,n=0,fallback,weak,j;
 unsigned short section;unsigned char bytes[6]={0,0,0,0,0,0};
 CoffSymbolEntry *symbol,*aux;CoffSection *owner;CoffContribution *contribution;
 CoffRelocationBlock *relocations;
 if(!aliases||!*op->object_started)return 0;count=0;
 for(a=aliases;a;a=a->next){
  if(n==16||!a->imported||!a->target||a->imported==a->target||
   strncmp(a->imported->name,"__imp_",6)||a->target->name!=a->imported->name+6)return 0;
  saved[n]=a;header_fields(a->imported,before[n]);header_fields(a->target,before[n]+6);
  if(op->lookup(a->imported)!=a->target)return 0;
  if(!picked&&a->imported->kind==4)picked=a;
  ++n;
 }
 if(n!=5||!picked||!put(0x31464643)||!put(n))return 0;
 for(i=0;i<n;++i){
  a=saved[i];header_fields(a->imported,after);header_fields(a->target,after+6);
  if(memcmp(before[i],after,sizeof(after))||a->next!=(i+1<n?saved[i+1]:0))return 0;
  if(!put(a->imported->kind)||!put(a->imported->identifier)||!put(a->target->identifier))return 0;
 }
 /* Finish normal debug records before adding the writer-owned metadata. */
 op->debug();fallback=picked->imported->field14;
 symbol=op->get(*op->symbols,fallback+1);
 if(!fallback||!symbol||symbol->kind!=3)return 0;
 weak=op->weak("_telemetry_optional_iat",0,105,0,0,fallback,2);
 symbol=op->get(*op->symbols,weak+1);aux=op->get(*op->symbols,weak+2);
 if(!symbol||symbol->kind!=3||!aux||aux->kind!=4||
  symbol->data.symbol->section||symbol->data.symbol->value||
  symbol->data.symbol->type||symbol->data.symbol->storage!=105||symbol->data.symbol->auxiliary_count!=1||
  ((unsigned*)aux->data.auxiliary)[0]!=fallback||((unsigned*)aux->data.auxiliary)[1]!=2)return 0;
 /* The real arena-seven allocator zeroes all new storage. Reserved aux bytes
    are defined zero, not allocator-dependent output or a new normalization. */
 for(j=8;j<18;++j)if(((unsigned char*)aux->data.auxiliary)[j])return 0;
 section=op->create(".segref",0x40300040,1);
 if(!section||!op->open(section,0))return 0;op->select(section);
 owner=op->get(*op->sections,section);
 if(!owner||!owner->last||owner->base_offset)return 0;
 contribution=owner->last;
 /* As in the real six-byte encoder contract: segment16 at0, offset32 at2. */
 op->segment(0,picked->imported);op->address(2,picked->imported);op->data(bytes,6);
 relocations=contribution->relocations;
 if(contribution->relocation_count!=2||!relocations||relocations->count!=2||
  relocations->entries[0].address||relocations->entries[0].type!=9||relocations->entries[0].symbol!=fallback||
  relocations->entries[1].address!=2||relocations->entries[1].type!=6||relocations->entries[1].symbol!=fallback||
  contribution->scope->buffered_bytes!=6||contribution->scope->completed_bytes)return 0;
 if(!put(fallback)||!put(weak)||!put(section)||!put((*op->symbols)->count)||!put(2)||!put(6))return 0;
 op->finish();
 if(*op->object_started||op->flush(0))return 0;
 return put(0);
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *probe)
{
 WriterOps op;int ok;
#define FN(field,rva) op.field=(void*)RECON_ORIGINAL_ADDRESS(probe,rva)
 FN(lookup,0x3e2f0);FN(weak,0x5eb20);FN(create,0x5e4f0);FN(open,0x5e5d0);
 FN(select,0x5e830);FN(get,0x5e780);FN(segment,0x5d600);FN(address,0x5d4f0);
 FN(data,0x5f030);FN(debug,0x59180);FN(finish,0x5c9a0);FN(flush,0x62160);
#undef FN
 op.symbols=(CoffVector**)RECON_ORIGINAL_ADDRESS(probe,0x751a8);
 op.sections=(CoffVector**)RECON_ORIGINAL_ADDRESS(probe,0x751a0);
 op.object_started=(int*)RECON_ORIGINAL_ADDRESS(probe,0x73158);
 ok=finish(*(ImportedSymbolAlias**)RECON_ORIGINAL_ADDRESS(probe,0x6e6b4),&op);
 if(ok)ok=recon_original_emit(probe,"observed.coff-finish",report,count*sizeof(unsigned));
 recon_original_finish(probe,ok);
}
#else
extern SymbolHeader *__fastcall lookup_alias_target(SymbolHeader *);
extern void finish_debugging(void);
extern int main(int,char **,char **);
extern int object_generation_started;
static int expected(void)
{
 FILE *file;char path[1024];unsigned words[64],n;const char *assets=recon_case_assets();
 if(!assets||strlen(assets)+28>=sizeof(path))return 0;
 strcpy(path,assets);strcat(path,"/coff-finish.expected");file=fopen(path,"rb");if(!file)return 0;
 n=fread(words,sizeof(unsigned),64,file);if(ferror(file)||!feof(file)){fclose(file);return 0;}fclose(file);
 return n==count&&!memcmp(words,report,n*sizeof(unsigned));
}
static void at_finish(void)
{
 WriterOps op;int ok;
 op.lookup=lookup_alias_target;op.weak=coff_define_weak_symbol;op.create=coff_create_section;
 op.open=coff_open_contribution;op.select=coff_select_section;op.get=coff_vector_get;
 op.segment=(Reference)coff_output_value;op.address=coff_output_bytes;op.data=coff_write_data;
 op.debug=finish_debug_module;op.finish=coff_finish_module;op.symbols=&coff_symbol_entries;
 op.sections=&coff_sections;op.object_started=&object_generation_started;op.flush=fflush;
 ok=finish(imported_symbol_aliases,&op);if(ok)ok=expected();recon_native_entry_finish(ok);
}
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);
}
int c2_coff_finish(void)
{
 int result=recon_native_at_entry((const void*)finish_debugging,(const void*)at_finish,real_input,0);
 recon_record_terminal_status(result==1?0:125);_exit(result==1?0:125);
}
#endif
