#include <recon-tool.h>
#include <recon-native.h>
#include "records.h"
#include "inline-calls.h"
#include "machine-transfers.h"
#include <string.h>
#include <recon.h>
extern int main(int,char **,char **);
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);
}
/* Actual first import from retained original alias-observed.bin. The44B
   kind1 record is initialized; compare its live string by content and preserve
   its pointer identity in the cloned suffix. No adjacent name-buffer bytes
   or kind1 trailing allocator bytes beyond descriptor_size(1) are asserted. */
static void __fastcall alias_entry(SymbolHeader *source)
{
 ImportedSymbolAlias *before=imported_symbol_aliases,*alias;
 SymbolHeader *target;unsigned fields[11];
 if(source->kind!=1||source->value!=4||source->field0d||source->field0e!=0x100||
  source->identifier!=0x99||strcmp(source->name,"__imp__telemetry_nonce")||
  ((FunctionSymbol*)source)->field20!=0x1012||((FunctionSymbol*)source)->field24!=0x1122)
  recon_native_entry_finish(0);
 memcpy(fields,source,44);clone_alias_symbol(source);alias=imported_symbol_aliases;
 if(!alias||alias==before||alias->next!=before||alias->imported!=source)recon_native_entry_finish(0);
 target=alias->target;
 recon_native_entry_finish(target&&target!=source&&target->name==source->name+6&&
  !strcmp(target->name,"_telemetry_nonce")&&target->identifier!=source->identifier&&
  ((FunctionSymbol*)target)->field24==0x122&&lookup_symbol(&global_scope,target->identifier)==target&&
  !memcmp(fields,source,44));
}
static int actual_import_alias(void)
{
 return recon_native_at_entry((const void*)clone_alias_symbol,(const void*)alias_entry,real_input,0)==1;
}
static void __fastcall clone_entry(SymbolHeader *source,unsigned arena)
{
 unsigned bytes=descriptor_size(source->kind);SymbolHeader *copy;
 if(arena||source->kind!=1||bytes!=44||strcmp(source->name,"__imp__telemetry_nonce"))recon_native_entry_finish(0);
 copy=clone_descriptor(source,arena);
 recon_native_entry_finish(copy&&copy!=source&&!memcmp(copy,source,bytes)&&copy->name==source->name);
}
static int actual_descriptor_clone(void)
{
 return recon_native_at_entry((const void*)clone_descriptor,(const void*)clone_entry,real_input,0)==1;
}

extern unsigned __fastcall mark_inline_assembly_parameter(SymbolHeader *,void *);
extern unsigned __fastcall substitute_inline_assembly_symbol(SymbolHeader *,void *);
static void __fastcall mark_entry(SymbolHeader *symbol,void *context)
{
 InlineBinding *binding=find_inline_binding(context,symbol);unsigned before=binding->flags;
 if(symbol->kind!=1||symbol->identifier!=0xa6||((FunctionSymbol*)symbol)->field24!=0x2232||
  !binding->parameter||binding->parameter->right.symbol!=symbol||binding->storage||before)
  recon_native_entry_finish(0);
 recon_native_entry_finish(!mark_inline_assembly_parameter(symbol,context)&&binding->flags==before);
}
static int actual_assembly_pin(void)
{
 return recon_native_at_entry((const void*)mark_inline_assembly_parameter,(const void*)mark_entry,real_input,0)==1;
}
static void __fastcall substitute_entry(SymbolHeader *symbol,void *context)
{
 InlineBinding *binding=find_inline_binding(context,symbol);
 if(symbol->kind!=1||symbol->identifier!=0xa6||((FunctionSymbol*)symbol)->field24!=0x2232||
  !binding->storage||binding->storage->right.symbol->identifier!=0xf3)recon_native_entry_finish(0);
 recon_native_entry_finish(substitute_inline_assembly_symbol(symbol,context)==0xf3);
}
static int actual_assembly_substitution(void)
{
 return recon_native_at_entry((const void*)substitute_inline_assembly_symbol,(const void*)substitute_entry,real_input,0)==1;
}
/* Cold controls cover the state-changing flag path; actual first binding
   already has direct-substitution bit0 cleared. */
static int assembly_binding_controls(void)
{
 FunctionSymbol source,target;Node parameter,storage;InlineBinding binding;
 memset(&source,0,sizeof(source));memset(&target,0,sizeof(target));
 memset(&parameter,0,sizeof(parameter));memset(&storage,0,sizeof(storage));memset(&binding,0,sizeof(binding));
 source.symbol.kind=1;source.field24=0x30;target.symbol.identifier=0x12345678;
 parameter.right.symbol=&source.symbol;storage.right.symbol=&target.symbol;
 binding.parameter=&parameter;binding.storage=&storage;binding.flags=0x55;
 if(mark_inline_assembly_parameter(&source.symbol,&binding)||binding.flags!=0x54||
  substitute_inline_assembly_symbol(&source.symbol,&binding)!=0x12345678)return 0;
 source.field24=0x20;binding.flags=0x55;
 if(mark_inline_assembly_parameter(&source.symbol,0)||binding.flags!=0x55||substitute_inline_assembly_symbol(&source.symbol,0))return 0;
 source.symbol.kind=4;source.field24=0x30;
 return !mark_inline_assembly_parameter(&source.symbol,0)&&!substitute_inline_assembly_symbol(&source.symbol,0);
}

extern int __fastcall inline_argument_has_global_load(Node *);
static void __fastcall argument_entry(Node *node)
{
 if(node->opcode!=0x55||node->attributes.value!=0x20204||node->detail.node!=node||
  !node->left.node||!node->second.node)recon_native_entry_finish(0);
 recon_native_entry_finish(inline_argument_has_global_load(node)==1);
}
static int actual_global_argument(void)
{
 return recon_native_at_entry((const void*)inline_argument_has_global_load,(const void*)argument_entry,real_input,0)==1;
}
static int global_argument_controls(void)
{
 Function owner;FunctionSymbol symbol;Node leaf,memory,unary,binary;
 memset(&owner,0,sizeof(owner));memset(&symbol,0,sizeof(symbol));
 memset(&leaf,0,sizeof(leaf));memset(&memory,0,sizeof(memory));
 memset(&unary,0,sizeof(unary));memset(&binary,0,sizeof(binary));
 current_function=&owner;symbol.symbol.kind=1;symbol.field24=0x20;
 leaf.opcode=0x33;memory.opcode=0x30;memory.left.node=&leaf;
 memory.attributes.words.flags=1;memory.right.symbol=&symbol.symbol;
 if(inline_argument_has_global_load(&leaf)||!inline_argument_has_global_load(&memory))return 0;
 unary.opcode=8;unary.left.node=&memory;
 binary.opcode=2;binary.left.node=&leaf;binary.second.node=&memory;
 if(!inline_argument_has_global_load(&unary)||!inline_argument_has_global_load(&binary))return 0;
 binary.left.node=&memory;binary.second.node=0;
 if(!inline_argument_has_global_load(&binary))return 0; /* short circuit */
 symbol.field24=0x30;
 if(inline_argument_has_global_load(&memory))return 0;
 memory.attributes.words.flags=0;
 return !inline_argument_has_global_load(&memory);
}
static void __fastcall boundary_entry(MachineInstruction *transfer)
{
 static const unsigned opcodes[]={0x400148,0x800148,0x140,0x140,0x140,0x140,0x140,0x140,0x2d40,0xe40};
 MachineInstruction *cursor=transfer,*expected;unsigned i;
 for(i=0;i<RECON_COUNT(opcodes);i++){
  if(!cursor||cursor->opcode!=opcodes[i])recon_native_entry_finish(0);
  expected=cursor;cursor=cursor->previous;
 }
 recon_native_entry_finish(find_transfer_call_boundary(transfer)==expected);
}
static int actual_call_boundary(void)
{
 return recon_native_at_entry((const void*)find_transfer_call_boundary,(const void*)boundary_entry,real_input,0)==1;
}
/* Cold controls: forward stores, direct100-class skips, indirect calls,
   and the specific kind14/2d/register4 marker. The actual first load walks
   nine preceding records and returns an indirect call, without marker14. */
static int call_boundary_controls(void)
{
 MachineInstruction transfer,ordinary,ignored,marker;BackendSymbol symbol;
 memset(&transfer,0,sizeof(transfer));memset(&ordinary,0,sizeof(ordinary));
 memset(&ignored,0,sizeof(ignored));memset(&marker,0,sizeof(marker));memset(&symbol,0,sizeof(symbol));
 transfer.opcode=9;transfer.next=&ignored;ignored.opcode=MC_CALL<<8;
 ignored.operands[0].flags=8;ignored.operands[0].value.symbol=&symbol.symbol;symbol.attributes=0x100;
 ignored.next=&ordinary;ordinary.opcode=MC_CALL<<8;ordinary.operands[0].flags=1;
 if(find_transfer_call_boundary(&transfer)!=&ordinary)return 0;
 symbol.attributes=0x200;
 if(find_transfer_call_boundary(&transfer)!=&ignored)return 0;
 transfer.opcode=8;transfer.previous=&ordinary;ordinary.next=&marker;
 marker.opcode=(0x2d<<8)|14;marker.operands[0].flags=1;marker.operands[0].value.constant=4;
 if(find_transfer_call_boundary(&transfer)!=&marker)return 0;
 marker.operands[0].value.constant=3;
 if(find_transfer_call_boundary(&transfer)!=&ordinary)return 0;
 marker.operands[0].value.constant=4;marker.opcode=0x2d40;
 return find_transfer_call_boundary(&transfer)==&ordinary;
}
const ReconTest dll_adapter_tests[]={
 {"actual-import-alias-owner-and-suffix",1,actual_import_alias},
 {"actual-kind1-clone-extent-and-identity",1,actual_descriptor_clone},
 {"actual-assembly-parameter-binding",1,actual_assembly_pin},
 {"actual-assembly-storage-identifier",1,actual_assembly_substitution},
 {"cold-assembly-binding-flags-and-rejections",1,assembly_binding_controls},
 {"actual-global-inline-argument",1,actual_global_argument},
 {"cold-argument-arity-shortcircuit-and-storage",1,global_argument_controls},
 {"actual-nine-record-call-boundary",1,actual_call_boundary},
 {"cold-forward-backward-call-boundaries",1,call_boundary_controls}
};
