#include <recon-tool.h>
#include <recon-native.h>
#include "float-machine.h"
#include "instruction-selection.h"
#include <string.h>
#include <recon.h>
extern int main(int,char **,char **);
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);
}
/* First genuine save:61 is resident ST0, initially homeless, width8 and
   flags1c. Original adds one FST before the FADD, retains the reload and
   changes flags to1e. Padding/pointers are checked by ownership, not values. */
static void __fastcall save_entry(MachineInstruction *instruction,
 FloatMachineRegister *reg,unsigned short number,MachineInstruction *reload)
{
 MachineInstruction *previous=instruction->previous,*store;
 MachineInstruction saved_reload=*reload;
 if(instruction->opcode!=0x14580||number!=61||reg->stack_slot||reg->home||
  reg->last_store||reg->width!=8||reg->flags!=0x1c||reload->opcode!=0x4580||
  reload->operands[1].flags!=1||reload->operands[1].value.constant!=0x1403d)
  recon_native_entry_finish(0);
 save_float_machine_register(instruction,reg,number,reload);
 store=instruction->previous;
 recon_native_entry_finish(store!=previous&&previous->next==store&&
  store->previous==previous&&store->next==instruction&&store->opcode==0x6280&&
  reg->home&&reg->home->flags==OPERAND_BASED&&reg->flags==0x1e&&
  store->operands[0].flags==reg->home->flags&&
  store->operands[0].value.address!=reg->home->value.address&&
  !memcmp(store->operands[0].value.address,reg->home->value.address,sizeof(MachineAddress))&&
  store->operands[1].flags==1&&store->operands[1].value.constant==REG_ST0&&
  !memcmp(reload,&saved_reload,sizeof(saved_reload)));
}
static int actual_save(void)
{
 return recon_native_at_entry((const void*)save_float_machine_register,
  (const void*)save_entry,real_input,0)==1;
}
/* Cold stable-store reuse borrows the genuine declaration home, while
   its local instruction chain deliberately contains no intervening write. */
static void __fastcall reuse_entry(MachineInstruction *instruction,
 FloatMachineRegister *reg,unsigned short number,MachineInstruction *reload)
{
 MachineInstruction previous,store,load;
 FloatMachineRegister copy;
 save_float_machine_register(instruction,reg,number,reload);
 memset(&previous,0,sizeof(previous));memset(&store,0,sizeof(store));memset(&load,0,sizeof(load));
 previous.next=&store;store.previous=&previous;store.next=&load;load.previous=&store;
 store.opcode=(MC_FSTP<<8)|0x80;store.operands[0]=*reg->home;
 store.operands[1].flags=1;store.operands[1].value.constant=REG_ST0;
 load.opcode=(MC_FLD<<8)|0x80;load.operands[1].flags=0x401;
 load.operands[1].value.constant=0x1403d;
 copy=*reg;copy.flags=0x24;
 save_float_machine_register(&store,&copy,number,&load);
 recon_native_entry_finish(copy.flags==0x24&&copy.home==reg->home&&
  previous.next==&store&&store.next==&load&&load.previous==&store&&
  load.operands[1].flags==reg->home->flags&&
  load.operands[1].value.address!=reg->home->value.address&&
  !memcmp(load.operands[1].value.address,reg->home->value.address,sizeof(MachineAddress)));
}
static int stable_reuse(void)
{
 return recon_native_at_entry((const void*)save_float_machine_register,
  (const void*)reuse_entry,real_input,0)==1;
}
static void __fastcall exit_entry(MachineInstruction *first,MemoryMotionContext *context)
{
 MachineInstruction *previous=first->previous,*store;
 MemoryMotionContext before=*context;
 FloatStreamUse *use=float_stream_uses,saved;
 if(first->opcode!=4||first->record.source_location!=0x82||context->stack_depth||
  !use||use->hash_next||use->previous_selected||use->stack_slot||use->width!=8||use->flags!=0x6c)
  recon_native_entry_finish(0);
 saved=*use;
 emit_float_exit_stores(first,context);store=first->previous;
 recon_native_entry_finish(store!=previous&&store->previous==previous&&
  previous->next==store&&store->next==first&&store->opcode==0x6380&&
  store->operands[0].flags==use->operand->flags&&
  store->operands[0].value.address!=use->operand->value.address&&
  !memcmp(store->operands[0].value.address,use->operand->value.address,sizeof(MachineAddress))&&
  store->operands[1].flags==1&&store->operands[1].value.constant==REG_ST0&&
  float_stream_uses==use&&!memcmp(use,&saved,sizeof(saved))&&!memcmp(context,&before,sizeof(before)));
}
static int actual_exit_stores(void)
{
 return recon_native_at_entry((const void*)emit_float_exit_stores,
  (const void*)exit_entry,real_input,0)==1;
}
/* Cold one-result control: original3bcda..3bceb chooses the oldest by
   previous_selected, then3be26 exits after that store. It does not drain
   the remaining hash chain as finish_float_context_motion does. */
static void __fastcall result_exit_entry(MachineInstruction *first,MemoryMotionContext *context)
{
 FloatStreamUse uses[2],*saved=float_stream_uses;
 MemoryMotionContext copy=*context;
 MachineInstruction *previous=first->previous,*exchange,*store;
 uses[0]=uses[1]=*saved;uses[0].stack_slot=0;uses[1].stack_slot=1;
 uses[0].hash_next=&uses[1];uses[0].previous_selected=&uses[1];
 uses[1].hash_next=0;uses[1].previous_selected=0;uses[1].flags=1;
 copy.stack_depth=1;float_stream_uses=&uses[0];
 emit_float_exit_stores(first,&copy);
 exchange=previous->next;store=exchange->next;
 float_stream_uses=saved;
 recon_native_entry_finish(exchange->opcode==0x5f80&&
  exchange->operands[0].flags==1&&exchange->operands[0].value.constant==REG_ST0+2&&
  store->opcode==0x6380&&store->previous==exchange&&store->next==first&&first->previous==store&&
  store->operands[0].flags==1&&store->operands[0].value.constant==REG_ST0&&
  uses[0].stack_slot==0&&uses[1].stack_slot==1&&copy.stack_depth==1);
}
static int one_result_exit(void)
{
 return recon_native_at_entry((const void*)emit_float_exit_stores,
  (const void*)result_exit_entry,real_input,0)==1;
}
const ReconTest metric_tests[]={
 {"actual-resident-save",1,actual_save},
 {"cold-stable-store-reuse",1,stable_reuse},
 {"actual-branch-exit-stores",1,actual_exit_stores},
 {"cold-one-result-oldest-exit",1,one_result_exit}
};
