#include "case.h"
#include <recon-native.h>
#include "output.h"
#include "machine-layout.h"
#include "instruction-selection.h"
#include "options.h"
#include <string.h>
#include <recon.h>
extern int main(int,char **,char **);
extern int cpu_model;
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};
 (void)unused;main(1,argv,0);
}
/* Actual first owner is the default-profile data symbol with output flags4.
   Check only defined fields and preserve its identity; kind3 is a cold no-write
   control, not a claimed public path through the callback. */
static void __fastcall mark_entry(SymbolHeader *symbol)
{
 BackendSymbol *backend=(BackendSymbol*)symbol,other={0};
 char *name=symbol->name;long value=symbol->value;
 if(symbol->kind!=1||strcmp(name,"_default_profile")||backend->output_flags!=4)
  recon_native_entry_finish(0);
 mark_unwritten_symbol(symbol);
 if(backend->output_flags!=5||symbol->name!=name||symbol->value!=value||symbol->kind!=1)
  recon_native_entry_finish(0);
 mark_unwritten_symbol(symbol);
 other.symbol.kind=3;other.output_flags=0x40;mark_unwritten_symbol(&other.symbol);
 recon_native_entry_finish(backend->output_flags==5&&other.output_flags==0x40);
}
static int actual_mark(void)
{return recon_native_at_entry((const void*)mark_unwritten_symbol,(const void*)mark_entry,real_input,0)==1;}
/* Genuine first insertion returns1 and pads before a full-width MOV to EDI.
   Preserve every old owner/opcode/order, prove one new linked INC EDI with
   encoded length/flagsE001, then exercise bounded cold eligibility rejections
   using a copied real candidate. No graph fabrication drives acceptance. */
static int __fastcall padding_entry(MachineInstruction *input)
{
 MachineInstruction *boundary=input,*owners[128],*scan,*after=input->next,*padding=0;
 MachineInstruction copy;unsigned opcodes[128],count=0,index=0,added=0;
 if(cpu_model!=3||input->opcode!=0x800140||
  (input->operands[0].flags&15)!=OPERAND_BASED)recon_native_entry_finish(0);
 while(boundary&&(boundary->opcode&15)!=1&&(boundary->opcode&15)!=2)
  boundary=boundary->previous;
 if(!boundary)recon_native_entry_finish(0);
 for(scan=boundary->next;scan!=after&&count<128;scan=scan->next){
  if(!scan)recon_native_entry_finish(0);owners[count]=scan;opcodes[count++]=scan->opcode;
 }
 if(scan!=after||!insert_alignment_increment(input))recon_native_entry_finish(0);
 for(scan=boundary->next;scan!=after;scan=scan->next){
  if(!scan)recon_native_entry_finish(0);
  if(index<count&&scan==owners[index]){
   if(scan->opcode!=opcodes[index++])recon_native_entry_finish(0);
  }else{padding=scan;if(++added!=1)recon_native_entry_finish(0);}
 }
 if(index!=count||added!=1||padding->opcode!=0x2140||
  padding->record.encoding_length!=0xe001||padding->operands[0].flags!=OPERAND_REGISTER||
  padding->operands[0].value.reg.words.number!=7||padding->previous->next!=padding||
  padding->next->previous!=padding||alignment_increment_register(padding->next)!=7)
  recon_native_entry_finish(0);
 copy=*padding->next;copy.operands[0].value.reg.words.number=4;
 if(alignment_increment_register(&copy)!=0xffff)recon_native_entry_finish(0);
 copy.operands[0].value.reg.words.number=5;
 if(alignment_increment_register(&copy)!=0xffff)recon_native_entry_finish(0);
 copy.operands[0].value.reg.words.number=8;
 if(alignment_increment_register(&copy)!=0xffff)recon_native_entry_finish(0);
 copy=*padding->next;copy.opcode=(copy.opcode&~0xf0U)|0x20;
 if(alignment_increment_register(&copy)!=0xffff)recon_native_entry_finish(0);
 copy=*padding->next;copy.operands[1]=copy.operands[0];
 recon_native_entry_finish(alignment_increment_register(&copy)==0xffff);return 0;
}
static int actual_padding(void)
{return recon_native_at_entry((const void*)insert_alignment_increment,(const void*)padding_entry,real_input,0)==1;}
const ReconTest listing_tests[]={
 {"actual no-object symbol mark and cold other-kind control",1,actual_mark},
 {"actual INC EDI padding owners and cold register exclusions",1,actual_padding}
};
