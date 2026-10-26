/* Dormant App helper qualification by constructor-based native units.
   These tests do NOT claim original paired or CLI execution of either helper.
   The instruction/region constructors own fresh typed storage; the real
   summary pass derives the constant induction from ADD and entry liveness.
   The original49bb0 caller contract and4acb0..4ae25 disassembly define the
   expected shape/count changes. No live compiler globals are forced. */
#include "../../src/machine-region-motion.h"
#include "../../src/instruction-selection.h"
#include "../../src/instruction-rewrite.h"
#include "../../src/bitsets.h"
#include "../../src/symbols.h"
#include "../../src/float-machine.h"
#include <recon-tool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <recon.h>
unsigned short __fastcall new_value_register(unsigned short type);
static MachineFlowRegion *loop(unsigned short *number, MachineInstruction **update)
{
    MachineInstruction *before,*first,*label,*last;
    MachineOperand left,right,target;
    LabelSymbol *symbol = allocate_descriptor(3,1);
    MachineFlowRegion *region;
    *number = new_value_register(4);
    before = append_machine_instruction(7);
    append_machine_instruction(3); /* splice API requires a real successor */
    first = new_machine_record(7,bitset_include(0,*number&0x3fff),0,1,0);
    insert_machine_after(before,first);
    label = new_machine_record(1,&symbol->symbol);
    insert_machine_after(first,label);
    region = new_machine_region(0,first,label);
    initialize_operand(&left,OPERAND_REGISTER,*number,0,0);
    initialize_operand(&right,OPERAND_IMMEDIATE,0,4,0);
    *update = new_machine_record(0,MC_ADD,4,&left,&right);
    insert_machine_after(label,*update);
    initialize_operand(&target,OPERAND_SYMBOL,0,0,&symbol->symbol);
    last = new_machine_record(0,MC_JMP,4,&target);
    insert_machine_after(*update,last);
    symbol->references = 1; /* one newly constructed backedge */
    region->last = last;    /* closed owned interval, not a borrowed region */
    prepare_region_insertion(region);
    simplify_machine_region_operations(region);
    return region;
}
static int increment_summary(void)
{
    static const unsigned shifts[]={0,1,2,3,31,32,33};
    MachineFlowRegion *region;
    MachineRegionRegister *record;
    MachineInstruction *update;
    unsigned short number;
    unsigned i,old_flags,old_uses,old_definitions;
    for(i=0;i<sizeof(shifts)/sizeof(shifts[0]);++i){
        region=loop(&number,&update);
        record=find_machine_region_register(region,number,0);
        if(!record||!(record->flags&4)||record->increment!=4||
            record->definition!=update||record->definitions!=1)return 0;
        old_flags=record->flags;old_uses=record->uses;old_definitions=record->definitions;
        scale_region_register_increment(number,shifts[i]);
        if(record->increment!=(4U<<(shifts[i]&31))||record->flags!=old_flags||
            record->uses!=old_uses||record->definitions!=old_definitions||
            record->definition!=update||update->operands[1].value.constant!=4)return 0;
    }
    return 1;
}
static int normalized_addresses(void)
{
    MachineInstruction *update,*instruction[6],*source_marker;
    MachineOperand destination,indexed;
    MachineAddress *address[6];
    MachineFlowRegion *region;
    SymbolHeader *symbol=allocate_descriptor(1,0);
    unsigned short number,related,other,result;
    unsigned i,flags[6],opcode[6];
    region=loop(&number,&update);
    related=new_value_register(4);other=new_value_register(4);result=new_value_register(4);
    initialize_operand(&destination,OPERAND_REGISTER,result,0,0);
    for(i=0;i<6;++i){
        address[i]=new_machine_address();
        address[i]->base.words.number=i==3?REG_EBP:27;
        address[i]->index.value=i==4?other:i==5?related:number;
        address[i]->first.displacement=i==1?8:0;
        address[i]->symbol=i==2?symbol:0;
        indexed.flags=0x9045U|(2U<<8); /* defined width/scale/annotation bits */
        indexed.value.address=address[i];
        instruction[i]=new_machine_record(0,MC_LEA,4,&destination,&indexed);
        insert_machine_after(update->previous,instruction[i]);
        flags[i]=instruction[i]->operands[1].flags;opcode[i]=instruction[i]->opcode;
    }
    source_marker=new_machine_record(4,0x20003);
    insert_machine_after(update->previous,source_marker);
    normalize_region_scaled_indices(number,related);
    for(i=0;i<6;++i){
        MachineOperand *operand=&instruction[i]->operands[1];
        if(i==4){
            if(operand->flags!=flags[i]||instruction[i]->opcode!=opcode[i])return 0;
        }else if(i==0||i==5){
            if(operand->flags!=((flags[i]&~0x30eU)|OPERAND_REGISTER)||
                operand->value.reg.value!=(i==5?related:number)||
                ((instruction[i]->opcode>>8)&255)!=MC_MOV)return 0;
        }else{
            if(instruction[i]->opcode!=opcode[i]||operand->value.address!=address[i])return 0;
            if(i==3){if(operand->flags!=(flags[i]&~0x300U)||
                address[i]->base.words.number!=REG_EBP)return 0;}
            else if(operand->flags!=((flags[i]&~0x30bU)|OPERAND_BASED)||
                address[i]->base.value!=number)return 0;
            if(address[i]->first.displacement!=(i==1?8:0)||
                address[i]->symbol!=(i==2?symbol:0)||address[i]->index.value!=number)return 0;
        }
        if(instruction[i]->operands[0].value.reg.value!=result)return 0;
    }
    return source_marker->record.source_location==0x20003&&
        update->operands[1].value.constant==4&&active_machine_region==region;
}
/* Six explicit original switch rows. FICOM/FICOMP/FILD are the three
   intervening bare-return slots and are deliberately NOT treated as valid. */
static int resident_arithmetic_forms(void)
{
    static const unsigned memory[]={MC_FIADD,MC_FIDIV,MC_FIDIVR,MC_FIMUL,MC_FISUB,MC_FISUBR};
    static const unsigned resident[]={MC_FADD,MC_FDIV,MC_FDIVR,MC_FMUL,MC_FSUB,MC_FSUBR};
    unsigned i;
    int status,pid;
    for(i=0;i<6;++i)if(float_register_arithmetic_opcode(memory[i])!=resident[i])return 0;
    pid=fork();if(pid<0)return 0;
    if(!pid){signal(SIGTRAP,SIG_DFL);float_register_arithmetic_opcode(MC_FICOM);_exit(17);}
    return waitpid(pid,&status,0)==pid&&WIFSIGNALED(status)&&WTERMSIG(status)==SIGTRAP;
}
static int scalar_bit_widths(void)
{
    return integer_width_bits(0)==0&&integer_width_bits(1)==8&&
        integer_width_bits(2)==16&&integer_width_bits(3)==0&&integer_width_bits(4)==32;
}
static int aggregate_secondary_result(void)
{
    static const unsigned sizes[]={1,2,4,8};
    FunctionSymbol *symbol=allocate_descriptor(14,1);
    unsigned i;
    symbol->field24=6;
    for(i=0;i<4;++i){
        symbol->symbol.value=sizes[i];
        if(return_value_register(symbol)!=REG_EAX||second_return_value_register(symbol)!=REG_EDX||
            symbol->field24!=6||symbol->symbol.value!=sizes[i])return 0;
    }
    return 1;
}
const ReconTest scaled_induction_tests[]={
    {"unit-qualified induction summary uses original x86 shift count",1,increment_summary},
    {"unit-qualified indexed normalization preserves owned address annotations",1,normalized_addresses},
    {"unit-qualified integer memory arithmetic maps to resident ST forms",1,resident_arithmetic_forms},
    {"unit-qualified scalar operand widths retain actual bit counts",1,scalar_bit_widths},
    {"unit-qualified plain aggregate secondary result uses EDX",1,aggregate_secondary_result}
};
