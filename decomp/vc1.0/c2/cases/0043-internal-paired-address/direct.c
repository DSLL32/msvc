/* INTERNAL DIRECT offset/selector pair arithmetic and near-address narrowing.
   Reuse case0034's captured source/IR and genuine optimizer entry/arena;
   this case owns its copies. All new nodes come from target constructors.
   The opaque16-bit selector literal is never dereferenced. No selector global,
   code pointer width or flags change. The established helper path is:
   build-direct direct.c --include ../../../lib/recon/include; marker-rva247232.
   Abrupt completion bypasses compiler cleanup, retaining its empty partial
   listing and strict absence of any object. Fixtures never invoke Wine. */
#include "../../src/tree-rewrite.h"
#include "../../src/expression-fields.h"
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
typedef Node *(__fastcall *Integer)(unsigned,unsigned);
typedef Node *(__cdecl *Build)(const char *,...);
typedef Node *(__fastcall *Unary)(Node *);
typedef void (__fastcall *Release)(Node *);
typedef struct PairApi {
 Integer integer;Build build;Unary left,right,narrow,finish;
 Release release;Expression **expression;Function **function;
} PairApi;
static unsigned words[1024],used;
static int put(unsigned value){if(used==1024)return 0;words[used++]=value;return 1;}
/* Restricted evaluator: offset arithmetic is modulo32 and the selector is
   retained by identity. It never dereferences the represented address. */
static int scalar(Node *node,unsigned *value,unsigned depth)
{
 unsigned a,b;if(!node||depth==16)return 0;
 if(node->opcode==0x33){*value=node->left.value;return 1;}
 if(node->opcode==0x34)return scalar(node->left.node,value,depth+1);
 if(node->opcode!=2&&node->opcode!=3)return 0;
 if(!scalar(node->left.node,&a,depth+1)||!scalar(node->second.node,&b,depth+1))return 0;
 *value=node->opcode==2?a+b:a-b;return 1;
}
static int exercise(PairApi *api,Function *entry)
{
 static const unsigned values[]={0,7,0xfffffff0U},amounts[]={0,3,17};
 Expression *anchor=*api->expression,*expressions[128],*scan;Node *roots[128];
 Function *function=*api->function;Node *pair,*selector,*offset,*amount,*node,*answer,*narrow;
 unsigned i,j,side,n=0,original_opcode,original_attributes,value,expected;
 if(!entry||entry!=function||!function->scope||!function->symbol||!anchor||
  !anchor->tree||anchor->tree->opcode!=0x47||strcmp(function->symbol->name,"_rotate_profile"))return 0;
 for(scan=function->expressions;scan;scan=scan->next){if(n==128)return 0;
  expressions[n]=scan;roots[n++]=scan->tree;}
 if(!n)return 0;for(i=0;i<n&&expressions[i]!=anchor;++i) {}if(i==n)return 0;
 original_opcode=anchor->tree->opcode;original_attributes=anchor->tree->attributes.value;
 used=0;if(!put(0x31524950)||!put(n))return 0;
 for(side=0;side<2;++side)for(i=0;i<3;++i)for(j=0;j<3;++j){
  offset=api->integer(values[i],0x404);selector=api->integer(0x23,0x102);
  pair=api->build("PPNt",offset,selector,0x68,0x408);
  amount=api->integer(amounts[j],0x104);
  node=side?api->build("PPNt",amount,pair,2,0x408):api->build("PPNt",pair,amount,3,0x408);
  expected=side?amounts[j]+values[i]:values[i]-amounts[j];
  answer=side?api->right(node):api->left(node);
  if(answer!=pair||answer->opcode!=0x68||answer->second.node!=selector||
   answer->left.node!=node||node->left.node!=(side?amount:offset)||
   node->second.node!=(side?offset:amount)||!scalar(node,&value,0)||value!=expected||
   !put(side)||!put(values[i])||!put(amounts[j])||!put(answer->opcode)||
   !put(answer->attributes.words.type)||!put(node->opcode)||!put(value))return 0;
  answer->left.node=api->finish(answer->left.node);
  if(!scalar(answer->left.node,&value,0)||value!=expected||answer->second.node!=selector||
   !put(answer->left.node->attributes.words.type)||!put(value))return 0;
  narrow=api->narrow(answer);
  if(!narrow||narrow->attributes.words.type!=0x404||!scalar(narrow,&value,0)||
   value!=expected||!put(narrow->attributes.words.type)||!put(value))return 0;
  api->release(narrow);
 }
 if(*api->function!=function||*api->expression!=anchor||anchor->tree->opcode!=original_opcode||
  anchor->tree->attributes.value!=original_attributes)return 0;
 scan=function->expressions;for(i=0;i<n;++i){if(scan!=expressions[i]||scan->tree!=roots[i])return 0;scan=scan->next;}
 return !scan&&put(n)&&put(18);
}
#ifdef RECON_ORIGINAL_DIRECT
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *probe)
{
 PairApi api;int ok;
 api.integer=(Integer)RECON_ORIGINAL_ADDRESS(probe,0x48430);
 api.build=(Build)RECON_ORIGINAL_ADDRESS(probe,0x3fe10);
 api.left=(Unary)RECON_ORIGINAL_ADDRESS(probe,0x41380);
 api.right=(Unary)RECON_ORIGINAL_ADDRESS(probe,0x413a0);
 api.narrow=(Unary)RECON_ORIGINAL_ADDRESS(probe,0x427b0);
 api.finish=(Unary)RECON_ORIGINAL_ADDRESS(probe,0x42860);
 api.release=(Release)RECON_ORIGINAL_ADDRESS(probe,0x3fb90);
 api.expression=(Expression**)RECON_ORIGINAL_ADDRESS(probe,0x6e818);
 api.function=(Function**)RECON_ORIGINAL_ADDRESS(probe,0x731b4);
 ok=exercise(&api,(Function*)probe->original_ecx);
 if(ok)ok=recon_original_emit(probe,"observed.paired-address",words,used*sizeof(unsigned));
 recon_original_finish(probe,ok);
}
#else
extern int main(int,char **,char **);
static int expected(void)
{
 FILE *file;char path[1024];unsigned other[1024],count;const char *assets=recon_case_assets();
 if(!assets||strlen(assets)+32>=sizeof(path))return 0;
 strcpy(path,assets);strcat(path,"/paired-address.expected");file=fopen(path,"rb");if(!file)return 0;
 count=fread(other,sizeof(unsigned),1024,file);if(ferror(file)||!feof(file)){fclose(file);return 0;}
 fclose(file);return count==used&&!memcmp(words,other,used*sizeof(unsigned));
}
static void __fastcall at_transform(Function *function)
{
 PairApi api;int ok;api.integer=integer_node;api.build=build_tree;
 api.left=lift_left_sequence;api.right=lift_right_sequence;api.narrow=discard_sequence_tail;
 api.finish=finish_canonical_node;api.release=release_tree;
 api.expression=&current_expression;api.function=&current_function;
 ok=exercise(&api,function);if(ok)ok=expected();recon_native_entry_finish(ok);
}
static void real_input(void *unused)
{char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);}
int paired_address(void)
{
 int result=recon_native_at_entry((const void*)transform_function_trees,(const void*)at_transform,real_input,0);
 recon_record_terminal_status(result==1?0:125);_exit(result==1?0:125);
}
#endif
