#ifndef WCC_BLOCK_H
#define WCC_BLOCK_H
#include "procdef.h"
#include <recon.h>
/* Older 386 block layout: 16-bit block IDs, no later dominator/stack fields. */
#pragma pack(4)
typedef struct name_set {
    hw_reg_set regs;
    unsigned out_of_block[4],within_block;
} name_set;
typedef struct ins_header {
    struct instruction *prev,*next;
    name_set live;
    unsigned short line_num;
    unsigned char opcode,state;
} ins_header;
/* The 1994 pattern bits fit a WORD, before the released DWORD expansion. */
typedef struct opcode_entry {
    unsigned short op_type;
    unsigned char verify,generate,reg_set,func_unit;
} opcode_entry;
typedef char old_opcode_entry_size[(sizeof(opcode_entry)==6)?1:-1];
typedef struct instruction {
    ins_header head;
    struct opcode_entry *table;
    union { struct opcode_entry *gen_table;struct instruction *parm_list,*cse_link; } u;
    register_name *zap;
    name *result;
    unsigned id;
    unsigned char type_class,base_type_class;
    unsigned short sequence;
    union { unsigned char byte,bool,call_flags,nop_flags,zap_value; } flags;
    union { unsigned char index_needs,stk_max; } t;
    unsigned char stk_entry,num_operands,ins_flags,stk_exit;
    union { unsigned char stk_extra,stk_depth; } s;
    name *operands[4];
} instruction;
typedef char old_instruction_size[(sizeof(instruction)==84)?1:-1];
typedef char old_instruction_result[(offsetof(instruction,result)==48)?1:-1];
typedef char old_instruction_type[(offsetof(instruction,type_class)==56)?1:-1];
typedef char old_instruction_operands[(offsetof(instruction,operands)==68)?1:-1];
typedef struct block_ins { ins_header hd;struct block *blk; } block_ins;
typedef struct block_edge {
    struct block *destination,*source;
    struct block_edge *next_source;
    unsigned short join_level,flags;
} block_edge;
struct interval_def;
struct data_flow_def;
typedef struct block {
    block_ins ins;
    struct block *next_block,*prev_block;
    union { struct interval_def *interval;struct block *partition,*loop; } u;
    struct block *loop_head;
    struct data_flow_def *dataflow;
    block_edge *input_edges;
    void *cc;
    union { struct block *alter_ego,*next; } v;
    struct code_lbl *label;
    unsigned available_bit;
    unsigned short depth,id,gen_id,inputs,targets;
    unsigned class;
    long iterations;
    unsigned unroll_count;
    block_edge edge[1];
} block;
#pragma pack()
typedef char old_ins_header_size[(sizeof(ins_header)==36)?1:-1];
typedef char old_block_label[(offsetof(block,label)==72)?1:-1];
typedef char old_block_class[(offsetof(block,class)==92)?1:-1];
extern block *CurrBlock,*BlockList,*HeadBlock;
int FELexLevel(void *symbol);
void NewProc(int level);
void SaveToTargProc(void);
void InitTargProc(void);
int AskDisplaySize(int level);
extern unsigned InsId;
extern unsigned char BlockByBlock,HaveCurrBlock,BlocksUnTrimmed;
extern long MaxStack;
extern name *Names[5],*LastTemp,*DummyIndex;
struct code_lbl *AskForSymLabel(void *symbol,unsigned char kind);
void EnLink(struct code_lbl *label,unsigned char label_dies);
block *NewBlock(struct code_lbl *label,unsigned char label_dies);
block *MakeBlock(struct code_lbl *label,unsigned short edges);
unsigned char InitCallState(type_def *type);
struct instruction *MakeNop(void);
instruction *NewIns(int operands);
void DoNothing(instruction *entry);
struct opcode_entry *OpcodeTable(unsigned char index);
instruction *MakeNary(unsigned char opcode,name *left,name *right,name *result,unsigned char type_class,unsigned char base_class,int operands);
void NamesCrossBlocks(void);
#endif
