#include <recon-tool.h>
#include <recon-native.h>
#include "tree-rewrite.h"
#include "function-input.h"
#include "tail-recursion.h"
#include "intrinsics.h"
#include "intrinsic-selection.h"
#include <recon.h>
extern int main(int,char **,char **);
extern int c2_decorated_wrapper_inputs(void),c2_decorated_decoder_inputs(void);
extern int c2_decorated_operator_controls(void),c2_decorated_failure_controls(void);
extern const IntrinsicDescription string_concatenate_intrinsic, memory_compare_intrinsic;
static void real_input(void *unused)
{
    char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};
    (void)unused;main(1,argv,0);
}
static int __fastcall tail_position_entry(Expression *head,Expression *call)
{
    Node *tree=call->tree;
    int valid=head&&head!=call&&head->next&&head->tree&&
        !head->field08&&!head->source_location&&call->next&&
        !call->field08&&call->source_location==0x30&&
        tree->opcode==0x41&&tree->attributes.value==0x204&&
        !tree->info.value&&tree->left.node&&!tree->second.value&&
        !tree->right.value&&!tree->detail.value;
    recon_native_entry_finish(valid);
}
static int actual_tail_position_input(void)
{
    return recon_native_at_entry((const void *)is_tail_call_position,
        (const void *)tail_position_entry,real_input,0)==1;
}
static int intrinsic_descriptors(void)
{
    const IntrinsicDescription *s=&string_concatenate_intrinsic,*m=&memory_compare_intrinsic;
    const unsigned *types=intrinsic_argument_types(170);
    return sizeof(*s)==16&&s->result_type==0x404&&!s->reserved&&
        !strcmp(s->commands,"nvyE0E1")&&!*s->runtime_name&&s->flags==0x40&&
        !m->result_type&&!m->reserved&&!strcmp(m->commands,"vE0E1")&&
        !*m->runtime_name&&m->flags==0x10&&types[0]==0x104&&
        types[1]==0x404&&types[2]==0x404&&!types[3]&&!types[4]&&!types[5];
}
/* Cold shape controls do not claim the current real return enters these
   later-expression paths. Label-lookup remains a named unimplemented child. */
static int tail_positions(void)
{
    Expression e[5];Node n[5];unsigned i;
    memset(e,0,sizeof e);memset(n,0,sizeof n);
    for(i=0;i<5;i++){e[i].tree=&n[i];e[i].next=i<4?&e[i+1]:0;}
    n[0].opcode=0x41;if(is_tail_call_position(e,e)!=1)return 0;
    n[0].opcode=0x3e;n[1].opcode=1;n[2].opcode=0x29;n[3].opcode=0x4f;n[4].opcode=0x47;
    if(is_tail_call_position(e,e)!=1)return 0;
    n[4].opcode=0x32;if(is_tail_call_position(e,e)!=0)return 0;
    n[4].opcode=1;e[4].next=e;if(is_tail_call_position(e,e)!=0)return 0;
    return 1;
}
static void __fastcall missing_end_entry(char *source,unsigned code)
{
    recon_native_entry_finish(code==0x22e&&!strcmp(source,"@(#)misc.c:1.100"));
}
static void missing_end_input(void *unused)
{
    Expression e;Node n;(void)unused;memset(&e,0,sizeof e);memset(&n,0,sizeof n);
    e.tree=&n;n.opcode=0x3e;is_tail_call_position(&e,&e);
}
static int missing_end(void)
{
    return recon_native_at_entry((const void *)compiler_internal_error,
        (const void *)missing_end_entry,missing_end_input,0)==1;
}
static SymbolHeader *__fastcall tail_argument_entry(Node *node)
{
    recon_native_entry_finish(node&&node->opcode==0x30&&
        node->attributes.value==0x10204&&!node->info.value&&node->left.node&&
        !node->second.value&&node->right.symbol&&node->detail.node==node);
}
static int actual_tail_argument_input(void)
{
    return recon_native_at_entry((const void *)tail_argument_address,
        (const void *)tail_argument_entry,real_input,0)==1;
}
static int tail_addresses(void)
{
    Node root,left,right,leaf;SymbolHeader first,second;
    memset(&root,0,sizeof root);memset(&left,0,sizeof left);
    memset(&right,0,sizeof right);memset(&leaf,0,sizeof leaf);
    leaf.opcode=0x33;left.opcode=0x26;left.right.symbol=&first;
    right.opcode=0x26;right.right.symbol=&second;
    if(tail_argument_address(&left)!=&first||tail_argument_address(&leaf))return 0;
    root.left.node=&left;root.second.node=&right;
    root.opcode=0x30;if(tail_argument_address(&root))return 0;
    root.opcode=0x3c;if(tail_argument_address(&root))return 0;
    root.opcode=0x3e;if(tail_argument_address(&root))return 0;
    root.opcode=0x32;if(tail_argument_address(&root)!=&second)return 0;
    root.opcode=2;if(tail_argument_address(&root)!=&first)return 0;
    root.left.node=&leaf;if(tail_argument_address(&root)!=&second)return 0;
    root.opcode=0x34;root.left.node=&left;
    if(tail_argument_address(&root)!=&first)return 0;
    root.left.node=&leaf;if(tail_argument_address(&root))return 0;
    root.opcode=0x32;root.second.node=&root;root.left.node=&right;
    /* Exercise nested assignment values without making a cyclic input. */
    root.second.node=&left;left.opcode=0x32;left.second.node=&right;
    if(tail_argument_address(&root)!=&second)return 0;
    return 1;
}
static unsigned short __fastcall memcmp_entry(Node *node,int wide)
{
    Node *a=node->second.node,*b=a->second.node,*c=b->second.node;
    int valid=!wide&&node->opcode==0x40&&node->attributes.value==0x104&&
        !node->info.value&&!node->right.value&&node->detail.value==8&&
        node->left.node->opcode==0x33&&node->left.node->left.value==170&&
        a->opcode==0x55&&a->attributes.value==0x80020404&&
        b->opcode==0x55&&b->attributes.value==0x80020404&&
        c->opcode==0x55&&c->attributes.value==0x20104&&c->second.node->opcode==0x4c&&
        a->left.node->opcode==0x30&&a->left.node->attributes.value==0x210404&&
        b->left.node->opcode==0x30&&b->left.node->attributes.value==0x110404&&
        c->left.node->opcode==0x34&&c->left.node->attributes.value==0x40000104;
    recon_native_entry_finish(valid);
}
static int actual_memcmp_input(void)
{
    return recon_native_at_entry((const void *)select_intrinsic_aa_ab,
        (const void *)memcmp_entry,real_input,0)==1;
}
/* Cold dword selection at a genuine live selection owner. The current public
   source emits byte memcmp; these controls exercise the separately decoded
   mode1 body without claiming public reach or unmasking its pending children. */
static unsigned short __fastcall memcmp_wide_entry(Node *node,int mode)
{
    static const int counts[]={1,2,3,4,5,6,7,8,9,17,-1};
    Node call=*node,args[3],count;MachineInstruction *before,*instruction;
    unsigned i,j,compares,repeats,cmps,branches,expected,widths[3],offsets[3];
    int valid=mode==0;
    args[0]=*node->second.node;args[1]=*args[0].second.node;args[2]=*args[1].second.node;
    args[0].second.node=&args[1];args[1].second.node=&args[2];
    call.second.node=args;memset(&count,0,sizeof count);
    count.opcode=0x33;count.attributes.words.type=0x104;args[2].left.node=&count;
    for(i=0;i<RECON_COUNT(counts)&&valid;i++){
        count.left.value=(unsigned)counts[i];before=machine_tail;
        if(select_intrinsic_aa_ab(&call,1)!=0x1a){valid=0;break;}
        compares=repeats=cmps=branches=0;
        for(instruction=before->next;instruction;instruction=instruction->next){
            if((instruction->opcode&15)!=0)continue;
            switch((instruction->opcode>>8)&255){
            case MC_CMP:
                if(compares>=3||(instruction->operands[1].flags&15)!=OPERAND_BASED){valid=0;break;}
                widths[compares]=(instruction->opcode>>4)&15;
                offsets[compares]=instruction->operands[1].value.address->first.displacement;
                ++compares;break;
            case MC_REP:++repeats;break;
            case MC_CMPS:++cmps;valid=valid&&((instruction->opcode>>4)&15)==4;break;
            case MC_JCC:++branches;valid=valid&&(instruction->operands[1].flags&15)==5;break;
            }
        }
        expected=(counts[i]>=4&&counts[i]<=7)?1:0;
        j=expected;if(expected&&(widths[0]!=4||offsets[0]))valid=0;
        if((counts[i]&3)>=2){if(j>=compares||widths[j]!=2||offsets[j]!=(expected?4:0))valid=0;++j;}
        if(counts[i]&1){if(j>=compares||widths[j]!=1||offsets[j]!=(expected?4:0)+((counts[i]&2)?2:0))valid=0;++j;}
        if(compares!=j||branches!=compares+(counts[i]>7)||repeats!=(counts[i]>7)||cmps!=repeats)valid=0;
    }
    recon_native_entry_finish(valid);
}
static int memcmp_wide_constants(void)
{
    return recon_native_at_entry((const void *)select_intrinsic_aa_ab,
        (const void *)memcmp_wide_entry,real_input,0)==1;
}
static MachineOperand *__fastcall zero_entry(unsigned short type)
{
    recon_native_entry_finish(type==0x104);
}
static int actual_zero_input(void)
{
    return recon_native_at_entry((const void *)zero_value_operand,
        (const void *)zero_entry,real_input,0)==1;
}
static MachineOperand *__fastcall zero_control_entry(unsigned short type)
{
    static const unsigned short types[]={0x101,0x102,0x104,0x204,0x108};
    MachineInstruction *before,*clear,*barrier;MachineOperand *operand;
    unsigned i;int valid=type==0x104;
    for(i=0;i<RECON_COUNT(types)&&valid;i++){
        before=machine_tail;operand=zero_value_operand(types[i]);
        clear=before->next;barrier=clear?clear->next:0;
        valid=operand&&clear&&barrier&&!barrier->next&&machine_tail==barrier&&
            (clear->opcode&15)==0&&((clear->opcode>>8)&255)==0x32&&
            ((clear->opcode>>4)&15)==(types[i]&15)&&
            (barrier->opcode&15)==10&&((barrier->opcode>>8)&255)==MC_MOV&&
            ((barrier->opcode>>4)&15)==4&&operand!=&barrier->operands[0]&&
            !memcmp(&clear->operands[0],operand,sizeof *operand)&&
            !memcmp(&clear->operands[1],operand,sizeof *operand)&&
            !memcmp(&barrier->operands[0],operand,sizeof *operand)&&
            !memcmp(&barrier->operands[1],operand,sizeof *operand)&&
            (operand->flags&15)==OPERAND_REGISTER;
    }
    recon_native_entry_finish(valid);
}
static int zero_controls(void)
{
    return recon_native_at_entry((const void *)zero_value_operand,
        (const void *)zero_control_entry,real_input,0)==1;
}
static unsigned short __fastcall memcmp_byte_entry(Node *node,int mode)
{
    static const int counts[]={0,1,2,3,4,8,17};
    Node call=*node,args[3],count;MachineInstruction *before,*instruction;
    unsigned i,sub,sbb,cmp,rep,cmps,jcc;unsigned short result;int valid=mode==0;
    args[0]=*node->second.node;args[1]=*args[0].second.node;args[2]=*args[1].second.node;
    args[0].second.node=&args[1];args[1].second.node=&args[2];call.second.node=args;
    memset(&count,0,sizeof count);count.opcode=0x33;count.attributes.words.type=0x104;
    args[2].left.node=&count;
    for(i=0;i<RECON_COUNT(counts)&&valid;i++){
        count.left.value=(unsigned)counts[i];before=machine_tail;
        result=select_intrinsic_aa_ab(&call,0);sub=sbb=cmp=rep=cmps=jcc=0;
        for(instruction=before->next;instruction;instruction=instruction->next){
            if((instruction->opcode&15)!=0)continue;
            switch((instruction->opcode>>8)&255){
            case MC_SUB:
                ++sub;valid=valid&&instruction->operands[0].value.reg.words.number==result&&
                    instruction->operands[1].value.reg.words.number==result;break;
            case MC_SBB:++sbb;valid=valid&&instruction->operands[0].value.reg.words.number==result;break;
            case MC_CMP:++cmp;valid=valid&&((instruction->opcode>>4)&15)==1;break;
            case MC_REP:++rep;break;
            case MC_CMPS:++cmps;valid=valid&&((instruction->opcode>>4)&15)==1;break;
            case MC_JCC:++jcc;valid=valid&&(instruction->operands[1].flags&15)==4;break;
            }
        }
        if(sub!=1||sbb!=2||cmp!=(counts[i]==1)||rep!=(counts[i]>1)||cmps!=rep||jcc!=(counts[i]!=0))valid=0;
    }
    recon_native_entry_finish(valid);
}
static int memcmp_byte_constants(void)
{
    return recon_native_at_entry((const void *)select_intrinsic_aa_ab,
        (const void *)memcmp_byte_entry,real_input,0)==1;
}
static unsigned short __fastcall strcat_entry(Node *node)
{
    Node *a=node->second.node,*b=a->second.node;
    int valid=node->opcode==0x40&&node->attributes.value==0x404&&
        !node->info.value&&!node->right.value&&node->detail.value==8&&
        node->left.node->opcode==0x33&&node->left.node->left.value==166&&
        a->opcode==0x55&&a->attributes.value==0x80020404&&
        b->opcode==0x55&&b->attributes.value==0x80020404&&b->second.node->opcode==0x4c&&
        a->left.node->opcode==0x30&&a->left.node->attributes.value==0x110404&&
        b->left.node->opcode==0x30&&b->left.node->attributes.value==0x80404;
    recon_native_entry_finish(valid);
}
static int actual_strcat_input(void)
{
    return recon_native_at_entry((const void *)select_intrinsic_a6,
        (const void *)strcat_entry,real_input,0)==1;
}
/* A synthetic named literal varies its NUL-inclusive length at the genuine
   active selection owner. No literal bytes are read during instruction selection. */
static unsigned short __fastcall strcat_literal_entry(Node *node)
{
    static const unsigned lengths[]={1,4,5,8,9,17,20};
    Node call=*node,args[2],source;TemporarySymbol literal;
    MachineInstruction *before,*instruction,*first;unsigned i,scans,addresses,destination;
    unsigned short result;int valid=1;
    args[0]=*node->second.node;args[1]=*args[0].second.node;
    args[0].second.node=&args[1];call.second.node=args;args[1].left.node=&source;
    for(i=0;i<RECON_COUNT(lengths)&&valid;i++){
        memset(&source,0,sizeof source);memset(&literal,0,sizeof literal);
        source.opcode=0x26;source.attributes.words.type=0x404;source.right.symbol=&literal.symbol;
        literal.symbol.kind=1;literal.symbol.field0d=0x25;literal.symbol.value=lengths[i];
        before=machine_tail;result=select_intrinsic_a6(&call);first=before->next;
        if(!first||((first->opcode>>8)&255)!=MC_MOV||
            (first->operands[1].flags&15)!=OPERAND_IMMEDIATE||
            first->operands[1].value.constant!=(lengths[i]>>2)){valid=0;break;}
        scans=addresses=0;destination=0;
        for(instruction=first;instruction;instruction=instruction->next){
            if((instruction->opcode&15)!=0)continue;
            if(((instruction->opcode>>8)&255)==MC_MOV&&
                (instruction->operands[0].flags&15)==OPERAND_REGISTER&&
                instruction->operands[0].value.reg.words.number==REG_EDI&&
                (instruction->operands[1].flags&15)==OPERAND_REGISTER)
                destination=instruction->operands[1].value.reg.words.number;
            if(((instruction->opcode>>8)&255)==MC_SCAS){++scans;if(result!=destination)valid=0;}
            if(((instruction->opcode>>8)&255)==MC_LEA&&
                instruction->operands[1].value.address->symbol==&literal.symbol)++addresses;
        }
        if(scans!=1||addresses!=1)valid=0;
    }
    recon_native_entry_finish(valid);
}
static int strcat_literals(void)
{
    return recon_native_at_entry((const void *)select_intrinsic_a6,
        (const void *)strcat_literal_entry,real_input,0)==1;
}
const ReconTest textrec_tests[]={
    {"actual tail-position arguments",1,actual_tail_position_input},
    {"decorated operator truncation and register POD",1,c2_decorated_operator_controls},
    {"decorated malformed names and shared buffer",1,c2_decorated_failure_controls},
    {"actual decorated wrapper name",1,c2_decorated_wrapper_inputs},
    {"actual decorated decoder owners and limit",1,c2_decorated_decoder_inputs},
    {"actual strcat arguments",1,actual_strcat_input},
    {"strcat named literal lengths",1,strcat_literals},
    {"actual zero-value operand type",1,actual_zero_input},
    {"zero-value opcode and operand identity",1,zero_controls},
    {"memcmp byte constant emission",1,memcmp_byte_constants},
    {"actual memcmp arguments",1,actual_memcmp_input},
    {"memcmp dword constant emission",1,memcmp_wide_constants},
    {"actual tail-argument address input",1,actual_tail_argument_input},
    {"tail-argument address cold shapes",1,tail_addresses},
    {"strcat and memcmp descriptor layout",1,intrinsic_descriptors},
    {"tail-position cold shapes",1,tail_positions},
    {"tail-position missing-end diagnostic",1,missing_end}
};
