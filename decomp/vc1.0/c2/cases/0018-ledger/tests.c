#include <recon-tool.h>
#include <recon-native.h>
#include <string.h>
#include "tree-rewrite.h"
#include "function-input.h"
#include "symbol-constants.h"
#include "input.h"
#include "records.h"
#include "expression-fields.h"
#include "frame.h"
#include "instruction-selection.h"
#include "codeview-output.h"
#include <recon.h>
extern int main(int,char **,char **);
static void real_input(void *unused)
{
 char *args[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};
 (void)unused;main(1,args,0);
}
static int __fastcall parameter_entry(unsigned *used,Node *argument,int convention)
{
 TemporarySymbol *symbol=(TemporarySymbol*)argument->right.symbol;
 recon_native_entry_finish(used&&!*used&&convention==20&&argument->opcode==0x2d&&
   argument->attributes.value==0x400000&&argument->info.value==0&&symbol&&
   symbol->symbol.kind==1&&symbol->symbol.value==4&&symbol->attributes==0x233);
}
static int actual_parameter(void)
{
 return recon_native_at_entry((const void*)assign_parameter_register,
   (const void*)parameter_entry,real_input,0)==1;
}
static int parameter_controls(void)
{
 static const unsigned types[]={0x101,0x102,0x104,0x201,0x202,0x204,0x404,0x108,0x408,0x1004,0x8000,0};
 static const unsigned masks[]={0,2,4,6,0x40000002U,0xffffffffU};
 Node argument,before;TemporarySymbol symbol;unsigned used,bits,expected,reg;
 int i,j,k,ok,result;
 for(i=0;i<RECON_COUNT(types);i++)for(j=0;j<RECON_COUNT(masks);j++)for(k=0;k<2;k++){
  memset(&argument,0x5a,sizeof argument);argument.opcode=0x55;
  argument.attributes.words.type=(unsigned short)types[i];before=argument;used=masks[j];
  bits=k?2:6;expected=masks[j];reg=0;ok=i<7&&((masks[j]&bits)!=bits);
  if(ok){reg=!(masks[j]&2)?1:2;expected|=1U<<reg;before.attributes.words.flags|=0x20;before.info.words.identifier=(short)reg;}
  result=assign_parameter_register(&used,&argument,k?20:4);
  if(result!=ok||used!=expected||memcmp(&argument,&before,sizeof argument))return 0;
 }
 memset(&symbol,0,sizeof symbol);symbol.attributes=0x233;symbol.symbol.value=4;
 memset(&argument,0,sizeof argument);argument.opcode=0x2d;argument.right.symbol=&symbol.symbol;
 used=0;if(assign_parameter_register(&used,&argument,20)!=1||used!=2||argument.info.words.identifier!=1||argument.attributes.words.flags!=0x20)return 0;
 symbol.symbol.value=8;argument.attributes.words.flags=0;used=0;
 if(assign_parameter_register(&used,&argument,4)||used||argument.attributes.words.flags)return 0;
 return 1;
}
extern int c2_ledger_token_inputs(void);
extern int c2_decorated_token_controls(void);
extern int c2_ledger_qualified_controls(void);
static void __fastcall constant_entry(SymbolHeader *symbol,int arena)
{
 SymbolConstant *constant=(SymbolConstant*)symbol;
 recon_native_entry_finish(arena==1&&symbol->kind==8&&symbol->value==4&&
   !strcmp(symbol->name,"_Small")&&(constant->debug_type&0xffff)==0x102b&&
   (constant->attributes&15)==1&&input_file&&input_file->_cnt>=3&&
   !memcmp(input_file->_ptr,"\x02\x1f\0",3));
}
static int actual_constant(void)
{
 return recon_native_at_entry((const void*)read_qualified_symbol,
   (const void*)constant_entry,real_input,0)==1;
}
static int constant_controls(void)
{
 static const unsigned sizes[]={0,1,2,4,127,128,255,1021};
 SymbolConstant value,before;FILE *saved=input_file,*file;unsigned i,j,size;int result=1;
 for(i=0;i<RECON_COUNT(sizes);i++){
  size=sizes[i];file=fopen("ledger-constant-control.bin","w+b");if(!file)return 0;
  if(size<128)fputc(size,file);else{fputc(0x80,file);fputc(size&255,file);fputc(size>>8,file);}
  for(j=0;j<size;j++)fputc((j*37+11)&255,file);
  fputc(0x7e,file);rewind(file);input_file=file;
  memset(&value,0x5a,sizeof value);before=value;
  /* Establish a real arena cursor before the zero-length allocation. */
  arena_alloc(i&1,4);
  read_qualified_symbol(&value.symbol,i&1);
  before.value_bytes=value.value_bytes;before.value_size=(short)size;
  if(memcmp(&value,&before,sizeof value)||!value.value_bytes||fgetc(file)!=0x7e)result=0;
  for(j=0;j<size&&result;j++)if(value.value_bytes[j]!=((j*37+11)&255))result=0;
  fclose(file);remove("ledger-constant-control.bin");if(!result)break;
 }
 input_file=saved;reset_arena(0);reset_arena(1);return result;
}
extern Node *floating_temporary_nodes;
extern unsigned next_serial;
static Node *__fastcall typed_entry(Function *function,Node *node)
{
 Node *reference;TemporarySymbol *symbol;unsigned serial=next_serial;SymbolHeader *old_bucket;
 int valid=function&&function->scope&&node->opcode==0x9b&&
   node->attributes.words.type==0x8000&&node->info.value==8&&node->detail.value==0xd3&&
   !floating_temporary_nodes;
 if(!valid)recon_native_entry_finish(0);
 old_bucket=function->scope->buckets[serial&63];
 reference=intern_typed_label(function,node);symbol=(TemporarySymbol*)reference->right.symbol;
 valid=floating_temporary_nodes==node&&!node->left.node&&node->second.node==reference&&
   node->detail.value==0xd3&&reference->opcode==0x26&&!reference->left.value&&symbol&&
   symbol->symbol.kind==1&&symbol->symbol.field0d==0x26&&!symbol->symbol.name&&
   symbol->symbol.identifier==serial&&next_serial==serial+1&&symbol->symbol.value==8&&
   (symbol->attributes&0x7ff)==0x216&&symbol->symbol.field18.identifier==6&&
   (symbol->symbol.field0e&0x800)&&symbol->symbol.field04==(unsigned)local_stack_offset&&
   function->scope->buckets[serial&63]==&symbol->symbol&&symbol->symbol.hash_next==old_bucket;
 recon_native_entry_finish(valid);
}
static int actual_typed(void)
{
 return recon_native_at_entry((const void*)intern_typed_label,
   (const void*)typed_entry,real_input,0)==1;
}
static int typed_cache_hits(void)
{
 Node first,second,incoming,before,result;Node *saved=floating_temporary_nodes,*answer;
 memset(&first,0,sizeof first);memset(&second,0,sizeof second);memset(&result,0,sizeof result);
 first.opcode=second.opcode=0x9b;first.detail.value=3;second.detail.value=9;
 first.left.node=&second;second.second.node=&result;floating_temporary_nodes=&first;
 memset(&incoming,0,sizeof incoming);incoming.opcode=0x9b;incoming.detail.value=9;
 /* The original cache compares only identifiers, even when the incoming
    descriptor carries a different type. A hit never reads the Function. */
 incoming.attributes.words.type=0x104;before=incoming;
 answer=intern_typed_label(0,&incoming);
 floating_temporary_nodes=saved;
 return answer==&result&&!memcmp(&incoming,&before,sizeof incoming)&&first.left.node==&second&&
   second.second.node==&result;
}
static MachineOperand *__fastcall high_entry(MachineOperand *operand)
{
 MachineOperand *result;int valid=operand&&operand->flags==1&&operand->value.reg.value==0x8028;
 if(!valid)recon_native_entry_finish(0);
 result=high_dword_operand(operand);
 recon_native_entry_finish(result==operand&&operand->flags==1&&operand->value.reg.value==0x8029);
}
static int actual_high(void)
{
 return recon_native_at_entry((const void*)high_dword_operand,
   (const void*)high_entry,real_input,0)==1;
}
static int high_controls(void)
{
 static const unsigned registers[]={0,1,0x8000,0xfffe,0xffff};
 static const unsigned displacements[]={0,4,0xfffffffcU,0xfffffffeU,0x7ffffffeU,0x80000000U};
 static const unsigned kinds[]={3,4,6};
 MachineOperand operand,before;MachineAddress address,expected;unsigned i,j;
 for(i=0;i<RECON_COUNT(registers);i++){
  operand.flags=0x5a5a0001;operand.value.reg.value=0xabcd0000|registers[i];before=operand;
  before.value.reg.words.number=(unsigned short)(registers[i]+1);
  if(high_dword_operand(&operand)!=&operand||memcmp(&operand,&before,sizeof operand))return 0;
 }
 for(i=0;i<RECON_COUNT(kinds);i++)for(j=0;j<RECON_COUNT(displacements);j++){
  memset(&address,0x5a,sizeof address);address.first.displacement=(long)displacements[j];expected=address;
  expected.first.displacement=(long)(displacements[j]+4U);
  operand.flags=0xa5a50000|kinds[i];operand.value.address=&address;before=operand;
  if(high_dword_operand(&operand)!=&operand||memcmp(&operand,&before,sizeof operand)||memcmp(&address,&expected,sizeof address))return 0;
 }
 return 1;
}
static unsigned __fastcall numeric_prefix_entry(unsigned char *output,long value)
{
 unsigned short *prefix=(unsigned short*)((uintptr_t)output-6);
 recon_native_entry_finish(output&&value==0x8765&&!prefix[0]&&prefix[1]==3&&prefix[2]==0x102b);
}
static int actual_numeric_prefix(void)
{
 return recon_native_at_entry((const void*)encode_codeview_numeric,
   (const void*)numeric_prefix_entry,real_input,0)==1;
}
static int numeric_controls(void)
{
 static const struct { long value;unsigned count;unsigned char bytes[6]; } rows[]={
  {-32769,6,{3,0x80,0xff,0x7f,0xff,0xff}},
  {-32768,4,{1,0x80,0,0x80}}, {-129,4,{1,0x80,0x7f,0xff}},
  {-128,3,{0,0x80,0x80}}, {-1,3,{0,0x80,0xff}},
  {0,2,{0,0}}, {127,2,{0x7f,0}}, {128,2,{0x80,0}},
  {32767,2,{0xff,0x7f}}, {32768,4,{2,0x80,0,0x80}},
  {34661,4,{2,0x80,0x65,0x87}}, {65535,4,{2,0x80,0xff,0xff}},
  {65536,6,{4,0x80,0,0,1,0}}, {0x7fffffffL,6,{4,0x80,0xff,0xff,0xff,0x7f}},
  {(long)0x80000000U,6,{3,0x80,0,0,0,0x80}}
 };
 unsigned char buffer[16];unsigned i,j;
 for(i=0;i<RECON_COUNT(rows);i++){
  memset(buffer,0x5a,sizeof buffer);
  if(encode_codeview_numeric(buffer,rows[i].value)!=rows[i].count||memcmp(buffer,rows[i].bytes,rows[i].count))return 0;
  for(j=rows[i].count;j<sizeof buffer;j++)if(buffer[j]!=0x5a)return 0;
 }
 return 1;
}
static unsigned char emitted_constant[64];
static unsigned emitted_bytes,writer_order;
static Segment *selected_constant_segment;
static void __fastcall constant_select(Segment *segment)
{
 selected_constant_segment=segment;writer_order=writer_order==0?1:99;
}
static void __fastcall constant_data(void *data,unsigned count)
{
 writer_order=writer_order==1?2:99;emitted_bytes=count;
 if(count<=sizeof emitted_constant)memcpy(emitted_constant,data,count);
}
static int constant_writer_controls(void)
{
 static const struct { unsigned short width;unsigned value,count;unsigned char numeric[6]; } rows[]={
  {1,0,2,{0,0}}, {1,0xff,2,{0xff,0}}, {2,0xffff,4,{2,0x80,0xff,0xff}},
  {4,0xffffffffU,3,{0,0x80,0xff}}, {4,0x80000000U,6,{3,0x80,0,0,0,0x80}},
  {4,0x7fffffffU,6,{4,0x80,0xff,0xff,0xff,0x7f}}
 };
 OutputCallbacks saved=output_callbacks;Segment segment,*saved_segment=debug_symbols_segment;
 SymbolConstant symbol;unsigned i,total;unsigned char expected[32];int valid=1;
 output_callbacks.select_segment=constant_select;output_callbacks.data=constant_data;debug_symbols_segment=&segment;
 for(i=0;i<RECON_COUNT(rows);i++){
  memset(&segment,0,sizeof segment);segment.symbol.field04=100;
  memset(&symbol,0,sizeof symbol);symbol.symbol.kind=8;symbol.symbol.name="_Value";
  symbol.debug_type=0x1234;symbol.value_size=rows[i].width;symbol.value_bytes=(unsigned char*)&rows[i].value;
  writer_order=emitted_bytes=0;selected_constant_segment=0;memset(emitted_constant,0x5a,sizeof emitted_constant);
  total=rows[i].count+12;expected[0]=(unsigned char)(total-2);expected[1]=0;expected[2]=3;expected[3]=0;
  expected[4]=0x34;expected[5]=0x12;memcpy(expected+6,rows[i].numeric,rows[i].count);
  memcpy(expected+6+rows[i].count,"\x05Value",6);
  write_codeview_constant(&symbol.symbol);
  if(writer_order!=2||emitted_bytes!=total||selected_constant_segment!=&segment||
     segment.symbol.field04!=100+total||memcmp(emitted_constant,expected,total)){valid=0;break;}
 }
 output_callbacks=saved;debug_symbols_segment=saved_segment;return valid;
}
const ReconTest ledger_tests[]={
    {"actual-token-cursor",1,c2_ledger_token_inputs},
    {"token-identity-cache-and-bytes",1,c2_decorated_token_controls},
    {"qualified-ledger-spellings",1,c2_ledger_qualified_controls},
    {"actual-parameter-register-inputs",1,actual_parameter},
    {"parameter-register-types-and-masks",1,parameter_controls},
    {"actual-constant-record-inputs",1,actual_constant},
    {"constant-payload-length-and-storage",1,constant_controls},
    {"actual-typed-temporary-state",1,actual_typed},
    {"typed-temporary-cache-identity",1,typed_cache_hits},
    {"actual-high-dword-operand",1,actual_high},
    {"high-dword-identity-and-wrap",1,high_controls},
    {"actual-CodeView-constant-prefix",1,actual_numeric_prefix},
    {"CodeView-numeric-leaf-boundaries",1,numeric_controls},
    {"CodeView-constant-widths-and-records",1,constant_writer_controls}
};
