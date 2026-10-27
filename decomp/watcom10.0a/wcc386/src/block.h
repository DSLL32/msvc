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
    unsigned char join_level; /* Actual old interval-depth byte at +12. */
    unsigned short flags;
} block_edge;
struct interval_def;
/* Actual old global variable sets are four DWORDs, not the later larger
 * backend universe. The seven encountered data-flow sets occupy 112 bytes. */
typedef struct global_bit_set { unsigned bits[4]; } global_bit_set;
typedef struct data_flow_def {
    global_bit_set in,out,def,use,call_exempt,need_load,need_store;
} data_flow_def;
typedef char old_data_flow_size[(sizeof(data_flow_def)==112)?1:-1];
/* Released conflict_node layout with the actual old 16-byte global set.
 * AddConflictNode now proves the complete 88-byte allocation and final
 * WORD state / BYTE possible fields. */
typedef struct reg_tree {
    struct reg_tree *lo,*hi;
    name *temp,*alt;
    hw_reg_set *regs,chosen;
    long size,offset;
    unsigned char idx,has_name;
} reg_tree;
typedef char old_reg_tree_size[(sizeof(reg_tree)==36)?1:-1];
typedef char old_reg_tree_idx[(offsetof(reg_tree,idx)==32)?1:-1];
typedef struct conflict_node {
    struct conflict_node *next_conflict,*next_for_name;
    name *name;
    struct block *start_block;
    struct reg_tree *tree;
    unsigned savings;
    int num_constrained,available;
    name_set with;
    struct { instruction *first,*last; } ins_range;
    struct { unsigned within_block;global_bit_set out_of_block; } id;
    unsigned short state;
    unsigned char possible;
} conflict_node;
typedef char old_conflict_global_id[(offsetof(conflict_node,id.out_of_block)==68)?1:-1];
typedef char old_conflict_size[(sizeof(conflict_node)==88)?1:-1];
typedef char old_conflict_state[(offsetof(conflict_node,state)==84)?1:-1];
typedef char old_conflict_possible[(offsetof(conflict_node,possible)==86)?1:-1];
extern conflict_node *ConfList;

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
    unsigned char depth;
    unsigned short id,gen_id,inputs,targets;
    unsigned class;
    long iterations;
    unsigned unroll_count;
    block_edge edge[1];
} block;
#pragma pack()
typedef char old_ins_header_size[(sizeof(ins_header)==36)?1:-1];
typedef char old_block_depth[(offsetof(block,depth)==80)?1:-1];
typedef char old_block_id[(offsetof(block,id)==82)?1:-1];
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
