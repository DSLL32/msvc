/* INTERNAL DIRECT scalar/Boolean contracts at the first real optimizer entry.
   All expression nodes and labels come from the target's own constructors.
   This does not claim that C1/C1XX leave short-circuit nodes in public IR. */
#include "../../src/tree-rewrite.h"
#include "../../src/algebra.h"
#include "../../src/flow-edges.h"
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
typedef Node *(__fastcall *IntegerNode)(unsigned,unsigned);
typedef Node *(__cdecl *TreeBuilder)(const char *,...);
typedef Node *(__fastcall *UnaryTree)(Node *);
typedef Node *(__fastcall *BinaryTree)(Node *,Node *);
typedef Node *(__cdecl *LabelNode)(void);
typedef void (__cdecl *Splice)(const char *,...);
typedef Expression *(__fastcall *InsertTree)(Expression *,Node *,Node *);
typedef void (__fastcall *RewriteExpression)(Expression *);
typedef struct Contracts {
 IntegerNode integer;TreeBuilder build;UnaryTree lift;
 BinaryTree simplify;LabelNode label;Splice splice;
 InsertTree insert;RewriteExpression expression_rewrite;
 Expression **expression;Function **function;
} Contracts;
static unsigned result_words[2048],result_count;
static SymbolHeader *labels[16];static unsigned label_count;
static int put(unsigned value)
{if(result_count==2048)return 0;result_words[result_count++]=value;return 1;}
static unsigned label_id(SymbolHeader *symbol)
{
 unsigned i;if(!symbol)return ~0U;
 for(i=0;i<label_count;++i)if(labels[i]==symbol)return i;
 if(label_count==16)return ~0U;labels[label_count]=symbol;return label_count++;
}
/* Independent finite modulo32 evaluator, restricted to constructed shapes.
   Never read hash links, node padding or inactive union alternatives. */
static int evaluate(Node *node,unsigned *value,unsigned depth)
{
 unsigned left,right;if(!node||depth==32)return 0;
 if(node->opcode==0x33){*value=node->left.value;return 1;}
 if(!evaluate(node->left.node,&left,depth+1))return 0;
 switch(node->opcode){
 case 8:*value=0U-left;return 1;
 case 0xe:*value=~left;return 1;
 case 0x1a:*value=!left;return 1;
 case 0x34:*value=left;return 1;
 }
 if(node->opcode==0x42)
  return evaluate(left?node->second.node->left.node:node->second.node->second.node,value,depth+1);
 if(!evaluate(node->second.node,&right,depth+1))return 0;
 switch(node->opcode){
 case 2:*value=left+right;break;
 case 4:*value=left*right;break;
 case 0xd:*value=left^right;break;
 case 0x1b:*value=!!left||!!right;break;
 case 0x1c:*value=!!left&&!!right;break;
 case 0x1f:*value=left==right;break;
 case 0x20:*value=left!=right;break;
 case 0x44:*value=right;break;
 default:return 0;
 }return 1;
}
/* Preorder shape/value report. Branch/label identity uses a bounded map;
   actual pointers are used only for within-process ownership assertions. */
static int record(Node *node,unsigned depth)
{
 unsigned op;if(!node||depth==32)return 0;op=node->opcode;
 if(!put(op)||!put(node->attributes.words.type))return 0;
 if(op==0x33)return put(node->left.value);
 if(op==0x29||op==0x38){
  if(!node->right.symbol||node->right.symbol->kind!=3||
   !put(label_id(node->right.symbol))||
   !put(((LabelSymbol*)node->right.symbol)->references))return 0;
  return op==0x29||record(node->left.node,depth+1);
 }
 if(!record(node->left.node,depth+1))return 0;
 if(op==8||op==0xe||op==0x1a||op==0x34)return 1;
 if(op==2||op==4||op==0xd||op==0x1b||op==0x1c||op==0x1f||
  op==0x20||op==0x42||op==0x43||op==0x44)return record(node->second.node,depth+1);
 return 0;
}
static int exercise(Contracts *api,Function *entry)
{
 Expression *anchor=*api->expression,*borrowed_anchor=anchor,*scan,*old[128];
 Node *old_trees[128],*original,*left,*right,*node,*answer,*context,*label;
 unsigned i,j,op,position,truth,control,old_count=0,value,expected;
 unsigned original_opcode,original_attributes;
 Function *function=*api->function;
 if(!anchor||entry!=function||anchor->tree->opcode!=0x47||
  !function||!function->scope||!function->symbol||
  strcmp(function->symbol->name,"_rotate_profile"))return 0;
 original=anchor->tree;original_opcode=original->opcode;original_attributes=original->attributes.value;

 for(scan=function->expressions;scan;scan=scan->next){
  if(old_count==128)return 0;old[old_count]=scan;old_trees[old_count++]=scan->tree;
 }
 for(i=0;i<old_count&&old[i]!=anchor;++i);
 if(!old_count||i==old_count)return 0;
 result_count=label_count=0;if(!put(0x31435442)||!put(old_count))return 0;
 /* Both literal positions and truth values, with/without Boolean context. */
 for(op=0x1b;op<=0x1c;++op)for(position=0;position<2;++position)
 for(truth=0;truth<2;++truth)for(control=0;control<2;++control){
  left=api->integer(truth?2:0,0x204);
  right=api->build("PNt",api->integer(7,0x204),0xe,0x204);
  node=position?api->build("PPNt",right,left,op,0x104):
                api->build("PPNt",left,right,op,0x104);
  context=control?api->build("PNt",api->integer(0,0x204),0x1a,0x104):0;
  if(!evaluate(node,&expected,0))return 0;
  answer=api->simplify(node,context);
  if(!answer||!evaluate(answer,&value,0)||value!=expected||
   !put(position)||!put(truth)||!put(control)||!put(value)||!record(answer,0))return 0;
 }
 /* Genuine caller contracts: unsigned multiplication/negation and XOR/NOT.
    Values are finite modulo32, with independent before/after evaluation. */
 for(i=0;i<2;++i){
  left=api->integer(0x12345678,0x204);right=api->build("PNt",api->integer(37,0x204),i?0xe:8,0x204);
  node=api->build("PPNt",left,right,i?0xd:4,0x204);
  if(!evaluate(node,&expected,0))return 0;answer=api->lift(node);
  if(answer!=right||answer->left.node!=node||node->left.node!=left||
   !evaluate(answer,&value,0)||value!=expected||!put(value)||!record(answer,0))return 0;
 }
 if(*api->expression!=anchor||anchor->tree!=original)return 0;
 /* Place each genuine target-created label before the active expression,
    then use the established insertion API to place a constructed branch
    before it. rewrite_expression publishes the true owner and integrates
    the returned root normally. No private global assignment is used. */
 for(op=0x1b;op<=0x1c;++op){
  label=api->label();if(!label||label->opcode!=0x29)return 0;
  api->splice("I",label);
  left=api->build("PPNY",api->integer(3,0x204),api->integer(0,0x204),0x20);
  right=api->build("PPNY",api->integer(5,0x204),api->integer(0,0x204),0x20);
  node=api->build("PPNt",left,right,op,0x104);
  node=api->build("PNST",node,0x38,label);
  if(api->insert(function->expressions,label,node)!=function->expressions)return 0;
  for(scan=function->expressions,j=0;scan&&scan->tree!=node;scan=scan->next)
   if(++j==256)return 0;
  if(!scan)return 0;api->expression_rewrite(scan);answer=(*api->expression)->tree;
  if(answer!=node||answer->opcode!=0x38||answer->right.symbol!=label->right.symbol||
   !put(op)||!record(answer,0))return 0;
 }
 /* Report the complete resulting order using old-root indices plus defined
    branch/label trees. Target-created labels use relative identity IDs. */
 for(scan=function->expressions,j=0;scan;scan=scan->next){
  if(++j>256)return 0;
  for(i=0;i<old_count&&old_trees[i]!=scan->tree;++i);
  if(i<old_count){if(!put(0x10000+i))return 0;}
  else if(!record(scan->tree,0))return 0;
 }
 if(!put(j))return 0;
 /* Every original expression tree survives in order. Other borrowed owners
    and their defined payloads were never supplied to these transformations. */
 for(scan=function->expressions,i=0;scan&&i<old_count;scan=scan->next)
  if(scan->tree==old_trees[i]){
   if(old[i]!=borrowed_anchor&&scan!=old[i])return 0;++i;
  }
 return i==old_count&&*api->function==function&&old_trees[old_count-1]->opcode==original_opcode&&
  old_trees[old_count-1]->attributes.value==original_attributes&&
  put(label_count)&&put(i);
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *probe)
{
 Contracts api;int ok;
 api.integer=(IntegerNode)RECON_ORIGINAL_ADDRESS(probe,0x48430);
 api.build=(TreeBuilder)RECON_ORIGINAL_ADDRESS(probe,0x3fe10);
 api.lift=(UnaryTree)RECON_ORIGINAL_ADDRESS(probe,0x46e50);
 api.simplify=(BinaryTree)RECON_ORIGINAL_ADDRESS(probe,0x41570);
 api.label=(LabelNode)RECON_ORIGINAL_ADDRESS(probe,0x48b60);
 api.splice=(Splice)RECON_ORIGINAL_ADDRESS(probe,0x420d0);
 api.insert=(InsertTree)RECON_ORIGINAL_ADDRESS(probe,0x3fd10);
 api.expression_rewrite=(RewriteExpression)RECON_ORIGINAL_ADDRESS(probe,0x416b0);
 api.expression=(Expression**)RECON_ORIGINAL_ADDRESS(probe,0x6e818);
 api.function=(Function**)RECON_ORIGINAL_ADDRESS(probe,0x731b4);
 ok=exercise(&api,(Function*)probe->original_ecx);
 if(ok)ok=recon_original_emit(probe,"observed.canonical-trees",result_words,result_count*sizeof(unsigned));
 recon_original_finish(probe,ok);
}
#else
extern int main(int,char**,char**);
static int expected(void)
{
 FILE *file;char path[1024];unsigned words[2048],count;const char *assets=recon_case_assets();
 if(!assets||strlen(assets)+32>=sizeof(path))return 0;
 strcpy(path,assets);strcat(path,"/canonical-trees.expected");file=fopen(path,"rb");if(!file)return 0;
 count=fread(words,sizeof(unsigned),2048,file);
 if(ferror(file)||!feof(file)){fclose(file);return 0;}fclose(file);
 return count==result_count&&!memcmp(words,result_words,count*sizeof(unsigned));
}
static void __fastcall at_transform(Function *function)
{
 Contracts api;int ok;
 api.integer=integer_node;api.build=build_tree;api.lift=lift_right_unary;
 api.simplify=simplify_short_circuit;
 api.label=new_flow_label_node;api.splice=splice_expressions;
 api.insert=insert_tree_expression;api.expression_rewrite=rewrite_expression;
 api.expression=&current_expression;api.function=&current_function;
 ok=exercise(&api,function);if(ok)ok=expected();recon_native_entry_finish(ok);
}
static void real_input(void *unused)
{char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);}
int c2_canonical_trees(void)
{
 int result=recon_native_at_entry((const void*)transform_function_trees,(const void*)at_transform,real_input,0);
 recon_record_terminal_status(result==1?0:125);_exit(result==1?0:125);
}
#endif
