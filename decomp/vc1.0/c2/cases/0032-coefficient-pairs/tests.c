#include "case.h"
#include <recon-native.h>
#include "tree-rewrite.h"
#include "extended.h"
#include <recon.h>
extern int main(int,char **,char **);
extern int default_integer_bytes,tree_changed;
static int cold_dispatch;
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};
 (void)unused;main(1,argv,0);
}
/* The actual first comparison is <= on the distinct extended quarter/half
   payloads. Cold controls copy that real tree through the accepted allocator,
   testing each original opcode mapping with both operand orders. */
static Node *__fastcall comparison_entry(Node *node)
{
 const Extended *left=node->left.node->left.pointer,*right=node->second.node->left.pointer;
 Node *copy,*temporary,*result;unsigned op,swap;int changed;
 static const unsigned less[]={0,1,1,1,0,0};
 static const unsigned greater[]={0,1,0,0,1,1};
 if(node->opcode!=0x21||node->attributes.words.type!=0x1008||
  node->left.node->opcode!=0x33||node->second.node->opcode!=0x33||
  default_integer_bytes!=4||left==right||left->low||right->low||
  left->high!=0x80000000U||right->high!=0x80000000U||
  left->exponent!=0x3ffd||right->exponent!=0x3ffe||left->format!=8||right->format!=8)
  recon_native_entry_finish(0);
 if(cold_dispatch)for(swap=0;swap<2;++swap)for(op=0x1f;op<=0x24;++op){
  copy=copy_tree(node);copy->opcode=op;
  if(swap){temporary=copy->left.node;copy->left.node=copy->second.node;copy->second.node=temporary;}
  changed=tree_changed;result=fold_floating_comparison(copy);
  if(result->opcode!=0x33||result->attributes.words.type!=0x104||
   result->left.value!=(swap?greater:less)[op-0x1f]||tree_changed!=changed)
   recon_native_entry_finish(0);
 }
 changed=tree_changed;result=fold_floating_comparison(node);
 recon_native_entry_finish(result->opcode==0x33&&result->attributes.words.type==0x104&&
  result->left.value==1&&tree_changed==changed);return 0;
}
static int actual_comparison(void)
{
 cold_dispatch=0;return recon_native_at_entry((const void*)fold_floating_comparison,
  (const void*)comparison_entry,real_input,0)==1;
}
static int dispatch_controls(void)
{
 cold_dispatch=1;return recon_native_at_entry((const void*)fold_floating_comparison,
  (const void*)comparison_entry,real_input,0)==1;
}
const ReconTest coefficient_tests[]={
 {"actual quarter-half comparison and integer result",1,actual_comparison},
 {"cold six-opcode mapping on borrowed literal payloads",1,dispatch_controls}
};
