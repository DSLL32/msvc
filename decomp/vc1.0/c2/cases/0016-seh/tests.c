#include <recon-tool.h>
#include "exception-scope.h"
#include "c2.h"
#include "expression-fields.h"
#include "tree-rewrite.h"
#include "intrinsics.h"
#include "global-registers.h"
#include "register-transfers.h"
#include "seh-selection.h"
#include "initializers.h"
#include "intrinsic-selection.h"
#include "coff-writer.h"
#include "backend-passes.h"
#include "coff-output.h"
#include "options.h"
#include "output.h"
#include <recon-native.h>
#include <recon.h>
extern unsigned next_serial;
extern Node *free_nodes;
extern Expression *free_expressions;
extern int c2_scope_owned_record(void);
extern int c2_scope_push_inputs(void);
extern int c2_scope_current_inputs(void);
extern int c2_scope_pop_inputs(void);
extern int c2_scope_rewrite_inputs(void);
extern int c2_scope_annotation_inputs(void);
extern int c2_scope_unwind_inputs(void);
extern int c2_scope_finally_inputs(void);
extern int c2_finally_inputs(void);
extern int c2_scope_finish_inputs(void);
extern int c2_scope_collect_inputs(void);
extern int c2_scope_remember_inputs(void);
extern int c2_scope_find_local_inputs(void);
extern int c2_scope_insert_local_inputs(void);
extern int c2_scope_mark_inputs(void);
extern int c2_scope_visible_inputs(void);
extern int c2_scope_lookup_inputs(void);
extern int c2_protected_edge_inputs(void);
extern int c2_refresh_region_inputs(void);
extern int c2_restore_excluded_inputs(void);
extern int c2_register_ends_inputs(void);
extern int c2_assembly_record_inputs(void);
extern int c2_scope_table_inputs(void);
extern int c2_external_name_inputs(void);
extern int c2_narrow_condition_inputs(void);
extern int c2_coff_dword_inputs(void);
extern int c2_function_ranges_inputs(void);
/* Cold valid-stack controls support the recovered whole45B search. These
   synthetic kind lists are not claimed executed by the current public IR. */
static int enclosing_scopes(void)
{
    static const struct { unsigned depth, zero, expected; } rows[] = {
        {0,~0U,~0U}, {1,0,0}, {1,~0U,~0U}, {2,0,0}, {2,1,1},
        {3,1,1}, {20,0,0}, {20,9,9}, {20,19,19}, {20,~0U,~0U}
    };
    ExceptionScope records[20]; unsigned i,j;
    for(i=0;i<RECON_COUNT(rows);++i){
        memset(records,0,sizeof records);
        for(j=0;j<20;++j){records[j].kind=(j&1)+1;exception_scope_stack[j]=&records[j];}
        if(rows[i].zero!=~0U){records[0].kind=0;records[rows[i].zero].kind=0;}
        exception_scope_stack_depth=rows[i].depth;
        if(enclosing_try_scope()!=(rows[i].expected==~0U?0:&records[rows[i].expected]))return 0;
        if(exception_scope_stack_depth!=rows[i].depth)return 0;
        for(j=0;j<20;++j)
            if(exception_scope_stack[j]!=&records[j]||records[j].kind!=((j==rows[i].zero||(j==0&&rows[i].zero!=~0U))?0:(j&1)+1))return 0;
    }
    return 1;
}
/* Pointer-only store: the helper never consumes scope pointee fields. */
static int push_scopes(void)
{
    ExceptionScope records[2];unsigned i,j,k;
    static const unsigned depths[]={0,1,18,19};
    memset(records,0x5a,sizeof records);
    for(i=0;i<RECON_COUNT(depths);++i)for(k=0;k<2;++k){
        ExceptionScope *value=k?&records[0]:0;
        for(j=0;j<20;++j)exception_scope_stack[j]=&records[1];
        exception_scope_stack_depth=depths[i];push_exception_scope(value);
        if(exception_scope_stack_depth!=depths[i]+1)return 0;
        for(j=0;j<20;++j)
            if(exception_scope_stack[j]!=(j==depths[i]?value:&records[1]))return 0;
        for(j=0;j<sizeof records;++j)if(((unsigned char *)records)[j]!=0x5a)return 0;
    }
    return 1;
}
static void __fastcall overflow_entry(char *source,unsigned code)
{
    recon_native_entry_finish(code==0x301&&!strcmp(source,"@(#)except.c:1.25"));
}
static void overflowing_push(void *argument)
{
    exception_scope_stack_depth=20;push_exception_scope((ExceptionScope *)argument);
}
static int push_overflow_boundary(void)
{
    ExceptionScope scope;
    return recon_native_at_entry((const void *)compiler_internal_error,
        (const void *)overflow_entry,overflowing_push,&scope)==1;
}
/* Return identity and read-only storage, including null top pointers. */
static int current_scopes(void)
{
    ExceptionScope records[20];unsigned d,i;
    for(d=0;d<=20;++d){
        for(i=0;i<20;++i)exception_scope_stack[i]=&records[i];
        exception_scope_stack_depth=d;
        if(current_exception_scope()!=(d?&records[d-1]:0))return 0;
        if(exception_scope_stack_depth!=d)return 0;
        for(i=0;i<20;++i)if(exception_scope_stack[i]!=&records[i])return 0;
        if(d){exception_scope_stack[d-1]=0;if(current_exception_scope()!=0)return 0;}
    }
    return 1;
}
static int pop_scopes(void)
{
    ExceptionScope records[20];unsigned d,i;
    for(d=1;d<=20;++d){
        for(i=0;i<20;++i)exception_scope_stack[i]=&records[i];
        exception_scope_stack_depth=d;pop_exception_scope();
        if(exception_scope_stack_depth!=d-1)return 0;
        for(i=0;i<20;++i)if(exception_scope_stack[i]!=(i==d-1?0:&records[i]))return 0;
    }
    return 1;
}
static void __fastcall underflow_entry(char *source,unsigned code)
{
    recon_native_entry_finish(code==0x307&&!strcmp(source,"@(#)except.c:1.25"));
}
static void underflowing_pop(void *argument)
{
    exception_scope_stack_depth=0;pop_exception_scope();
}
static int pop_underflow_boundary(void)
{
    return recon_native_at_entry((const void *)compiler_internal_error,
        (const void *)underflow_entry,underflowing_pop,0)==1;
}
/* The plain stream has no SEH transformation: preserve each link/tree
   and source position, including the initial marker/predecessor alias. */
static int rewrite_plain_stream(void)
{
    Function function;Expression expressions[3];Node nodes[3];unsigned i;
    memset(&function,0,sizeof function);
    rewrite_exception_expressions(&function);
    memset(expressions,0,sizeof expressions);memset(nodes,0,sizeof nodes);
    for(i=0;i<3;++i){
        expressions[i].next=i==2?0:&expressions[i+1];
        expressions[i].tree=&nodes[i];expressions[i].source_location=19+i;
        nodes[i].opcode=0x56+i;
    }
    function.expressions=expressions;rewrite_exception_expressions(&function);
    if(function.expressions!=expressions)return 0;
    for(i=0;i<3;++i)
        if(expressions[i].next!=(i==2?0:&expressions[i+1])||
           expressions[i].tree!=&nodes[i]||expressions[i].source_location!=19+i||
           nodes[i].opcode!=0x56+i)return 0;
    return 1;
}
static int annotation_values(void)
{
    ExceptionScope scopes[20];Node *node;unsigned i,d;
    static const unsigned short codes[]={0,0x26,0x2d,0xffff};
    for(d=0;d<=20;d+=d?19:1){
        exception_scope_stack_depth=d;
        for(i=0;i<20;++i)exception_scope_stack[i]=&scopes[i];
        for(i=0;i<RECON_COUNT(codes);++i){
            node=new_exception_annotation(codes[i]);
            if(node->opcode!=0x4f||node->left.pointer!=(d?&scopes[d-1]:0)||
               (unsigned short)node->detail.control.code!=codes[i]||
               node->detail.control.flags||node->attributes.value||
               node->info.value||node->second.value||node->right.value)return 0;
        }
    }
    return 1;
}
static int unwind_bypass(void)
{
    ExceptionScope scope;
    exception_scope_stack_depth=0;
    if(insert_exception_unwind(0,0,-1))return 0;
    memset(&scope,0,sizeof scope);scope.identifier=9;
    exception_scope_stack[0]=&scope;exception_scope_stack_depth=1;
    if(insert_exception_unwind(0,0,9))return 0;
    scope.state=0x8e;scope.parent_identifier=-1;
    if(insert_exception_unwind(0,0,3))return 0;
    return exception_scope_stack_depth==1&&exception_scope_stack[0]==&scope&&
        scope.identifier==9&&scope.state==0x8e&&scope.parent_identifier==-1;
}
static int finally_unwind_call(void)
{
    static const long targets[]={-1,0,7};unsigned i;Node *node,*argument;
    sentinel_node=new_node(0x4c);
    for(i=0;i<RECON_COUNT(targets);++i){
        node=new_scope_finally_unwind(targets[i]);
        if(node->opcode!=0x40||node->attributes.words.type!=0x104||
           node->left.node->opcode!=0x33||node->left.node->left.value!=0xf1||
           node->left.node->attributes.words.type!=0x104)return 0;
        argument=node->second.node;
        if(argument->opcode!=0x55||argument->second.node!=sentinel_node||
           argument->left.node->opcode!=0x33||
           argument->left.node->left.value!=(unsigned)targets[i]||
           argument->left.node->attributes.words.type!=0x104)return 0;
    }
    return 1;
}
static int finally_depth_call(void)
{
    static const unsigned depths[]={0,1,20};unsigned i;Node *node,*argument;
    sentinel_node=new_node(0x4c);
    for(i=0;i<RECON_COUNT(depths);++i){
        node=new_finally_unwind(depths[i]);
        if(node->opcode!=0x40||node->attributes.words.type!=0x104||
           node->left.node->opcode!=0x33||node->left.node->left.value!=0xf2||
           node->left.node->attributes.words.type!=0x104)return 0;
        argument=node->second.node;
        if(argument->opcode!=0x55||argument->second.node!=sentinel_node||
           argument->left.node->opcode!=0x33||
           argument->left.node->left.value!=depths[i]*4||
           argument->left.node->attributes.words.type!=0x104)return 0;
    }
    return 1;
}
static int seh_intrinsic_lowering(void)
{
    Node *nodes[2],*node,*callee,*argument;unsigned i;
    sentinel_node=new_node(0x4c);
    nodes[0]=new_scope_finally_unwind(-1);nodes[1]=new_finally_unwind(3);
    for(i=0;i<2;++i){
        node=nodes[i];callee=node->left.node;argument=node->second.node;
        if(lower_intrinsic(node,0)!=node||node->left.node!=callee||
           node->second.node!=argument||node->attributes.words.type!=0x104||
           node->detail.control.code!=8||
           node->left.node->left.value!=(i?0xf2:0xf1)||
           argument->left.node->left.value!=(i?12:~0U)||
           argument->second.node!=sentinel_node)return 0;
    }
    return 1;
}
static int seh_status_lowering(void)
{
    Node *node,*callee;const unsigned *types;unsigned i;
    sentinel_node=new_node(0x4c);callee=integer_node(0xd7,0x104);
    node=build_tree("PPNYt",sentinel_node,callee,0x40,0x104);
    types=intrinsic_argument_types(0xd7);
    /* The fixed signature must not alias the mutable scalar-type cache. */
    intrinsic_argument_types(0xd9);
    for(i=0;i<5;++i)if(types[i])return 0;
    return lower_intrinsic(node,0)==node&&node->opcode==0x40&&
        node->left.node==callee&&node->second.node==sentinel_node&&
        node->attributes.words.type==0x104&&node->detail.control.code==8;
}
static int seh_d9_lowering(void)
{
    Node *node,*callee;const unsigned *types;unsigned i;
    sentinel_node=new_node(0x4c);callee=integer_node(0xd9,0x104);
    node=build_tree("PPNYt",sentinel_node,callee,0x7a,0x202);
    types=intrinsic_argument_types(0xd9);
    for(i=0;i<5;++i)if(types[i]!=0x202)return 0;
    return lower_intrinsic(node,0)==node&&node->opcode==0x7a&&
        node->left.node==callee&&node->second.node==sentinel_node&&
        node->attributes.words.type==0x202&&node->detail.control.code==8;
}
static int finish_scope_owners(void)
{
    Function function;ExceptionScope scopes[2];Expression expression;Node node;
    memset(&function,0,sizeof function);memset(scopes,0,sizeof scopes);
    memset(&expression,0,sizeof expression);memset(&node,0,sizeof node);
    scopes[0].next=&scopes[1];scopes[0].locals=(struct ExceptionLocalSet *)&node;
    scopes[1].locals=(struct ExceptionLocalSet *)&expression;
    prepared_exception_scopes=scopes;exception_scope_stack_depth=0;
    node.opcode=0x56;expression.tree=&node;expression.source_location=37;
    function.expressions=&expression;finish_inline_function(&function);
    return function.expressions==&expression&&function.exception_scopes==scopes&&
        !prepared_exception_scopes&&!scopes[0].locals&&!scopes[1].locals&&
        scopes[0].next==&scopes[1]&&!scopes[1].next&&!exception_scope_stack_depth&&
        expression.tree==&node&&node.opcode==0x56&&expression.source_location==37;
}
static unsigned expected_finish_error;
static void __fastcall finish_error_entry(char *source,unsigned code)
{
    recon_native_entry_finish(code==expected_finish_error&&
        !strcmp(source,"@(#)except.c:1.25"));
}
static void unfinished_scope_marker(void *argument)
{
    Function function;Expression expression;Node node;ExceptionScope scope;
    memset(&function,0,sizeof function);memset(&expression,0,sizeof expression);
    memset(&node,0,sizeof node);memset(&scope,0,sizeof scope);
    exception_scope_stack_depth=0;function.expressions=&expression;
    expression.tree=&node;node.opcode=0x4f;node.left.pointer=&scope;
    node.detail.control.code=(short)(unsigned)argument;
    finish_inline_function(&function);
}
static int finish_filter_boundary(void)
{
    expected_finish_error=0x6b;
    return recon_native_at_entry((const void *)compiler_internal_error,
        (const void *)finish_error_entry,unfinished_scope_marker,(void *)0x28)==1;
}
static int finish_protected_boundary(void)
{
    expected_finish_error=0x8f;
    return recon_native_at_entry((const void *)compiler_internal_error,
        (const void *)finish_error_entry,unfinished_scope_marker,(void *)0x26)==1;
}
static int finish_marker_maps(void)
{
    static const unsigned short codes[][6]={{0x26,0x27,0x28,0x29,0x2a,0x2b},
        {0x26,0x27,0x2c,0x2d,0,0}};
    static const unsigned lengths[]={6,4};
    Function function;ExceptionScope scope;Expression list[6];Node nodes[6];
    unsigned row,i;
    for(row=0;row<2;++row){
        memset(&function,0,sizeof function);memset(&scope,0,sizeof scope);
        memset(list,0,sizeof list);memset(nodes,0,sizeof nodes);
        function.expressions=list;prepared_exception_scopes=&scope;
        exception_scope_stack_depth=0;
        for(i=0;i<lengths[row];++i){
            list[i].next=i+1==lengths[row]?0:&list[i+1];list[i].tree=&nodes[i];
            nodes[i].opcode=0x4f;nodes[i].detail.control.code=codes[row][i];
            nodes[i].left.pointer=&scope;
        }
        finish_inline_function(&function);
        if(exception_scope_stack_depth||prepared_exception_scopes||
           function.exception_scopes!=&scope||exception_scope_stack[0])return 0;
        for(i=0;i<lengths[row];++i)
            if(list[i].tree!=&nodes[i]||nodes[i].detail.control.code!=codes[row][i])return 0;
    }
    return 1;
}
static int collect_tree_paths(void)
{
    Node leaves[2],root;ExceptionScope scope;
    memset(leaves,0xa5,sizeof leaves);leaves[0].opcode=leaves[1].opcode=0x33;
    collect_exception_references(&leaves[0],1);
    memset(&root,0,sizeof root);root.opcode=2;
    root.left.node=&leaves[0];root.second.node=&leaves[1];
    collect_exception_references(&root,0);
    if(root.opcode!=2||root.left.node!=&leaves[0]||root.second.node!=&leaves[1])return 0;
    memset(&scope,0,sizeof scope);scope.flags=0x40;
    exception_scope_stack_depth=1;exception_scope_stack[0]=&scope;
    root.opcode=0x40;leaves[0].left.value=0xd9;leaves[1].opcode=0x4c;
    collect_exception_references(&root,0);
    return scope.flags==0x41&&exception_scope_stack_depth==1&&
        exception_scope_stack[0]==&scope&&root.opcode==0x40;
}
static void __fastcall missing_scope_entry(char *source,unsigned code)
{
    recon_native_entry_finish(code==0xca&&!strcmp(source,"@(#)except.c:1.25"));
}
static void collecting_without_scope(void *argument)
{
    Node node,callee;
    memset(&node,0,sizeof node);memset(&callee,0,sizeof callee);
    node.opcode=0x40;node.left.node=&callee;callee.left.value=0xd9;
    exception_scope_stack_depth=0;collect_exception_references(&node,0);
}
static int collect_missing_scope_boundary(void)
{
    return recon_native_at_entry((const void *)compiler_internal_error,
        (const void *)missing_scope_entry,collecting_without_scope,0)==1;
}
static int remember_without_scope(void)
{
    exception_scope_stack_depth=0;
    remember_exception_local((SymbolHeader *)1);
    return exception_scope_stack_depth==0;
}
static int local_hash_lookup(void)
{
    ExceptionLocalSet set;ExceptionLocalEntry links[3];SymbolHeader symbols[3];
    unsigned i;
    memset(&set,0,sizeof set);memset(links,0,sizeof links);memset(symbols,0,sizeof symbols);
    if(find_exception_local(&set,0xa6))return 0;
    for(i=0;i<3;++i){symbols[i].identifier=6+16*i;links[i].symbol=&symbols[i];
        links[i].next=i==2?0:&links[i+1];}
    set.buckets[6]=links;set.buckets[5]=(ExceptionLocalEntry *)1;
    for(i=0;i<3;++i)if(find_exception_local(&set,6+16*i)!=&symbols[i])return 0;
    if(find_exception_local(&set,54)||find_exception_local(&set,7))return 0;
    for(i=0;i<16;++i)
        if(set.buckets[i]!=(i==6?links:i==5?(ExceptionLocalEntry *)1:0))return 0;
    for(i=0;i<3;++i)
        if(links[i].symbol!=&symbols[i]||links[i].next!=(i==2?0:&links[i+1])||
           symbols[i].identifier!=6+16*i)return 0;
    return 1;
}
static int local_hash_insert(void)
{
    ExceptionLocalSet set;SymbolHeader symbols[3],before[3];
    ExceptionLocalEntry *first,*second,*other,*duplicate;unsigned i;
    memset(&set,0,sizeof set);memset(symbols,0xa5,sizeof symbols);
    symbols[0].identifier=6;symbols[1].identifier=22;symbols[2].identifier=7;
    memcpy(before,symbols,sizeof symbols);
    insert_exception_local(&set,&symbols[0]);first=set.buckets[6];
    insert_exception_local(&set,&symbols[1]);second=set.buckets[6];
    insert_exception_local(&set,&symbols[2]);other=set.buckets[7];
    insert_exception_local(&set,&symbols[0]);duplicate=set.buckets[6];
    if(duplicate==first||duplicate==second||duplicate->symbol!=&symbols[0]||
       duplicate->next!=second||second->symbol!=&symbols[1]||second->next!=first||
       first->symbol!=&symbols[0]||first->next||other->symbol!=&symbols[2]||other->next)return 0;
    for(i=0;i<16;++i)
        if(set.buckets[i]!=(i==6?duplicate:i==7?other:0))return 0;
    return !memcmp(symbols,before,sizeof symbols)&&find_exception_local(&set,6)==&symbols[0]&&
        find_exception_local(&set,22)==&symbols[1]&&find_exception_local(&set,7)==&symbols[2];
}
static int mark_reference_flags(void)
{
    static const struct {unsigned opcode,flags;} rows[]={{0x30,0x801},
        {0x32,0x805},{0x5a,1},{2,1}};
    Node root,leaves[2];SymbolHeader symbol;unsigned i;
    memset(&symbol,0,sizeof symbol);symbol.kind=1;
    for(i=0;i<RECON_COUNT(rows);++i){
        memset(&root,0,sizeof root);memset(leaves,0,sizeof leaves);
        leaves[0].opcode=leaves[1].opcode=0x33;
        root.opcode=rows[i].opcode;root.attributes.words.flags=1;
        root.left.node=&leaves[0];root.second.node=&leaves[1];root.right.symbol=&symbol;
        mark_exception_references(&root,1);
        if(root.attributes.words.flags!=rows[i].flags||root.opcode!=rows[i].opcode||
           root.left.node!=&leaves[0]||root.second.node!=&leaves[1]||
           root.right.symbol!=&symbol)return 0;
    }
    root.opcode=0x32;root.attributes.words.flags=1;root.right.symbol=0;
    mark_exception_references(&root,1);
    return root.attributes.words.flags==1;
}
static int visible_local_paths(void)
{
    TemporaryRegister temporary;SymbolHeader symbol,borrowed;
    ExceptionScope scope;ExceptionLocalSet set;ExceptionLocalEntry entry;
    memset(&temporary,0xa5,sizeof temporary);temporary.kind=19;
    exception_scope_stack_depth=0;
    if(exception_local_visible((SymbolHeader *)&temporary))return 0;
    memset(&symbol,0,sizeof symbol);symbol.kind=1;symbol.identifier=0xab;
    if(exception_local_visible(&symbol))return 0;
    memset(&scope,0,sizeof scope);memset(&set,0,sizeof set);borrowed=symbol;
    entry.symbol=&borrowed;entry.next=0;set.buckets[11]=&entry;
    scope.locals=&set;scope.parent_identifier=-1;scope.flags=0x40;
    exception_scope_stack_depth=1;exception_scope_stack[0]=&scope;
    if(exception_local_visible(&symbol)!=1)return 0;
    return entry.symbol==&borrowed&&!entry.next&&set.buckets[11]==&entry&&
        scope.locals==&set&&scope.flags==0x40&&symbol.identifier==0xab&&
        exception_scope_stack_depth==1&&exception_scope_stack[0]==&scope;
}
static int scope_list_lookup(void)
{
    ExceptionScope scopes[3];unsigned i;
    memset(scopes,0xa5,sizeof scopes);
    for(i=0;i<3;++i){scopes[i].identifier=i==1?7:3;scopes[i].next=i==2?0:&scopes[i+1];}
    if(lookup_exception_scope((ExceptionScope *)1,-1)||
       lookup_exception_scope(scopes,3)!=&scopes[0]||
       lookup_exception_scope(scopes,7)!=&scopes[1])return 0;
    for(i=0;i<3;++i)
        if(scopes[i].identifier!=(i==1?7:3)||scopes[i].next!=(i==2?0:&scopes[i+1]))return 0;
    return 1;
}
static void __fastcall missing_identifier_entry(char *source,unsigned code)
{
    recon_native_entry_finish(code==0x2f4&&!strcmp(source,"@(#)except.c:1.25"));
}
static void lookup_missing_identifier(void *argument)
{
    ExceptionScope scope;
    memset(&scope,0,sizeof scope);scope.identifier=3;
    lookup_exception_scope(&scope,7);
}
static int scope_missing_identifier_boundary(void)
{
    return recon_native_at_entry((const void *)compiler_internal_error,
        (const void *)missing_identifier_entry,lookup_missing_identifier,0)==1;
}
static int protected_edge_pairs(void)
{
    WorkItem source,target,other;FlowLink ordinary_out,ordinary_in,old_out,old_in;
    FlowLink *out,*in,*head_out,*head_in;unsigned row;
    for(row=0;row<6;++row){
        memset(&source,0,sizeof source);memset(&target,0,sizeof target);
        memset(&other,0,sizeof other);memset(&ordinary_out,0,sizeof ordinary_out);
        memset(&ordinary_in,0,sizeof ordinary_in);memset(&old_out,0,sizeof old_out);
        memset(&old_in,0,sizeof old_in);free_flow_links=0;
        if(row==1){
            ordinary_out.work=&target;ordinary_in.work=&source;
            source.successors=&ordinary_out;target.predecessors=&ordinary_in;
        }else if(row==2){
            old_out.work=old_in.work=&other;old_out.flags=old_in.flags=1;
            source.successors=&old_out;target.predecessors=&old_in;
        }else if(row==3){
            old_out.work=&target;old_out.flags=0xa1;source.successors=&old_out;
        }else if(row==4){
            old_in.work=&source;old_in.flags=0xb1;target.predecessors=&old_in;
        }
        add_protected_flow_edge(&source,row==5?&source:&target);
        out=source.successors;in=row==5?source.predecessors:target.predecessors;
        if(row==1){if(out!=&ordinary_out||in!=&ordinary_in||out->flags||in->flags)return 0;
            out=out->next;in=in->next;}
        if(row==2){if(out!=&old_out||in!=&old_in||out->work!=&other||in->work!=&other)return 0;
            out=out->next;in=in->next;}
        if(!out||!in||out==in||out->next||in->next||
           out->work!=(row==5?&source:&target)||in->work!=&source||
           !(out->flags&1)||!(in->flags&1))return 0;
        if(row==3&&(out!=&old_out||out->flags!=0xa1))return 0;
        if(row==4&&(in!=&old_in||in->flags!=0xb1))return 0;
        head_out=source.successors;head_in=row==5?source.predecessors:target.predecessors;
        add_protected_flow_edge(&source,row==5?&source:&target);
        if(source.successors!=head_out||(row==5?source.predecessors:target.predecessors)!=head_in||
           out->next||in->next||source.predecessors!=(row==5?in:0)||target.successors)return 0;
    }
    return 1;
}
static int protected_recycled_flags(void)
{
    WorkItem source,target;FlowLink recycled[2];
    memset(&source,0,sizeof source);memset(&target,0,sizeof target);
    memset(recycled,0,sizeof recycled);recycled[0].next=&recycled[1];
    recycled[0].flags=0xa4;recycled[1].flags=0xb2;free_flow_links=recycled;
    add_protected_flow_edge(&source,&target);
    return source.successors==&recycled[0]&&target.predecessors==&recycled[1]&&
        recycled[0].work==&target&&recycled[1].work==&source&&
        !recycled[0].next&&!recycled[1].next&&recycled[0].flags==0xa5&&
        recycled[1].flags==0xb3&&!free_flow_links;
}
static int region_last_signed_order(void)
{
    static const short numbers[]={-2,32767,-1,7,7,-32768,0,100};
    WorkItem work[8];FlowRegion regions[2];unsigned i,pass;
    memset(work,0,sizeof work);memset(regions,0,sizeof regions);
    refresh_region_entries(0);
    regions[0].last=&work[0];regions[1].last=&work[1];
    regions[0].flags=0xa0;regions[1].flags=0xb0;
    for(i=0;i<8;++i){
        work[i].next=i==7?0:&work[i+1];work[i].number=numbers[i];
        work[i].region=i==7?0:(i==1||i==5||i==6?&regions[1]:&regions[0]);
    }
    for(pass=0;pass<2;++pass){
        refresh_region_entries(work);
        if(regions[0].last!=&work[3]||regions[1].last!=&work[1]||
           regions[0].flags!=0xa0||regions[1].flags!=0xb0)return 0;
        for(i=0;i<8;++i)
            if(work[i].next!=(i==7?0:&work[i+1])||work[i].number!=numbers[i]||
               work[i].region!=(i==7?0:(i==1||i==5||i==6?&regions[1]:&regions[0])))return 0;
    }
    return 1;
}
/* Independent cold controls: live first entry is filename leaf56. These
   exercise recursion and the original sign-extension/mask, without claiming
   all opcode/number combinations were present in the public input. */
static int restore_excluded_numbers(void)
{
    static const unsigned opcodes[]={0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
        0x17,0x18,0x19,0x30,0x32,0x35,0x36,0x5a,0x7d,0x87};
    static const unsigned numbers[]={5,0x4005,0x8005,0xc005,6};
    BitSet set[2];Node node,reg,leaf,saved;unsigned i,j;
    memset(set,0,sizeof set);set[0].base=-1;set[0].next=&set[1];set[1].bits=1U<<5;
    allocation_excluded_registers=set;
    for(i=0;i<RECON_COUNT(opcodes);++i)for(j=0;j<RECON_COUNT(numbers);++j){
        memset(&node,0,sizeof node);memset(&reg,0,sizeof reg);
        memset(&leaf,0,sizeof leaf);memset(&saved,0,sizeof saved);
        node.opcode=opcodes[i];reg.opcode=0xa2;reg.left.word=(short)numbers[j];
        node.left.node=&reg;node.second.node=&leaf;node.detail.node=&saved;
        restore_excluded_registers(&node);
        if(node.left.node!=(j<2?&saved:&reg)||node.second.node!=&leaf||
           node.detail.node!=&saved||reg.left.word!=(short)numbers[j]||
           allocation_excluded_registers!=set||set[0].base!=-1||set[0].next!=&set[1]||
           set[0].bits||set[1].base||set[1].next||set[1].bits!=(1U<<5))return 0;
    }
    return 1;
}
static int restore_excluded_tree_paths(void)
{
    Node root,wrapper,loads[2],regs[2],saved[2],leaf;BitSet set[2];unsigned i;
    memset(&root,0,sizeof root);memset(&wrapper,0,sizeof wrapper);
    memset(loads,0,sizeof loads);memset(regs,0,sizeof regs);
    memset(saved,0,sizeof saved);memset(&leaf,0,sizeof leaf);memset(set,0,sizeof set);
    set[0].base=-1;set[0].next=&set[1];set[1].bits=(1U<<5)|(1U<<7);
    allocation_excluded_registers=set;
    for(i=0;i<2;++i){loads[i].opcode=0x30;loads[i].left.node=&regs[i];
        loads[i].detail.node=&saved[i];regs[i].opcode=0xa2;regs[i].left.word=(short)(5+i*2);}
    root.opcode=2;root.left.node=&loads[0];root.second.node=&loads[1];
    wrapper.opcode=0x3c;wrapper.left.node=&root;
    restore_excluded_registers(&wrapper);
    if(loads[0].left.node!=&saved[0]||loads[1].left.node!=&saved[1]||
       root.left.node!=&loads[0]||root.second.node!=&loads[1]||wrapper.left.node!=&root)return 0;
    loads[0].left.node=&regs[0];allocation_excluded_registers=0;
    restore_excluded_registers(&loads[0]);if(loads[0].left.node!=&regs[0])return 0;
    allocation_excluded_registers=set;loads[0].left.node=&leaf;
    restore_excluded_registers(&loads[0]);if(loads[0].left.node!=&leaf)return 0;
    root.opcode=8;root.left.node=&regs[0];root.detail.node=&saved[0];
    restore_excluded_registers(&root);if(root.left.node!=&regs[0])return 0;
    return allocation_excluded_registers==set&&set[0].next==&set[1]&&
        set[1].bits==((1U<<5)|(1U<<7));
}
static int register_end_annotations(void)
{
    static const unsigned opcodes[]={0x0f,0x30,8,0x0f,0x32};
    WorkItem work;AllocationValue value;AllocationRange range;
    Expression expressions[5],spares[4],*cursor;Node trees[5],regs[5],nodes[4],type;
    unsigned i,inserted=0;memset(&work,0,sizeof work);
    mark_register_definition_ends(&work,0,0);
    memset(&value,0,sizeof value);memset(&range,0,sizeof range);memset(&type,0,sizeof type);
    memset(expressions,0,sizeof expressions);memset(spares,0,sizeof spares);
    memset(trees,0,sizeof trees);memset(regs,0,sizeof regs);memset(nodes,0,sizeof nodes);
    value.node=&type;type.attributes.value=0x50104;range.number=0x4027;
    work.body.expressions=expressions;
    for(i=0;i<4;++i){nodes[i].left.node=i==3?0:&nodes[i+1];spares[i].next=i==3?0:&spares[i+1];}
    free_nodes=nodes;free_expressions=spares;
    for(i=0;i<5;++i){expressions[i].next=i==4?0:&expressions[i+1];
        expressions[i].tree=&trees[i];expressions[i].source_location=70+i;
        trees[i].opcode=opcodes[i];trees[i].left.node=&regs[i];
        regs[i].opcode=0xa2;regs[i].left.word=i==3?0x4028:0x4027;}
    mark_register_definition_ends(&work,&value,&range);
    cursor=work.body.expressions;
    for(i=0;i<5;++i){
        if(cursor!=&expressions[i]||cursor->tree!=&trees[i]||cursor->source_location!=70+i||
           trees[i].left.node!=&regs[i]||trees[i].opcode!=opcodes[i])return 0;
        cursor=cursor->next;
        if(i==0||i==4){Node *node;if(cursor!=&spares[inserted])return 0;
            node=cursor->tree;if(node!=&nodes[inserted]||node->opcode!=0x4f||
                node->attributes.value!=0x104||node->info.value||node->left.value!=0x4027||
                node->second.value||node->right.value||node->detail.value!=0x25||
                cursor->field08||cursor->source_location)return 0;
            ++inserted;cursor=cursor->next;}
    }
    return !cursor&&inserted==2&&free_nodes==&nodes[2]&&free_expressions==&spares[2]&&
        range.number==0x4027&&value.node==&type&&type.attributes.value==0x50104;
}
static int register_end_signed_comparison(void)
{
    static const unsigned words[]={5,0x4005,0x8005,0xffff,5};
    static const unsigned ranges[]={5,0x4005,0x8005,0xffff,0x4005};
    WorkItem work;AllocationValue value;AllocationRange range;Expression expression,spare;
    Node store,reg,type,node;unsigned i;
    for(i=0;i<RECON_COUNT(words);++i){
        memset(&work,0,sizeof work);memset(&value,0,sizeof value);memset(&range,0,sizeof range);
        memset(&expression,0,sizeof expression);memset(&spare,0,sizeof spare);
        memset(&store,0,sizeof store);memset(&reg,0,sizeof reg);memset(&type,0,sizeof type);
        memset(&node,0,sizeof node);work.body.expressions=&expression;expression.tree=&store;
        store.opcode=0x0f;store.left.node=&reg;reg.opcode=0xa2;reg.left.word=(short)words[i];
        value.node=&type;range.number=(unsigned short)ranges[i];free_nodes=&node;free_expressions=&spare;
        mark_register_definition_ends(&work,&value,&range);
        if(i<2){if(expression.next!=&spare||spare.tree!=&node||spare.next||
            node.left.value!=ranges[i]||node.detail.control.code!=0x25)return 0;}
        else if(expression.next||free_nodes!=&node||free_expressions!=&spare)return 0;
    }
    return 1;
}
/* Cold instruction-list assertions isolate the recovered selector's
   operation order and operand ownership; they are not public-input traces. */
static int seh_annotation_operations(void)
{
    static const unsigned short codes[]={0x26,0x27,0x27,0x27,0x2b,0x28,0x28,0x2d,0x2a};
    static const unsigned expected[][4]={{MC_EFWAIT,MC_MOV,0,0},{MC_EFWAIT,MC_MOV,MC_PUSH,0},
        {MC_EFWAIT,MC_MOV,MC_PUSH,0},{MC_EFWAIT,MC_MOV,0,0},{MC_MOV,0,0,0},
        {0,0,0,0},{MC_MOV,MC_MOV,MC_MOV,MC_MOV},{MC_RET,0,0,0},{0,0,0,0}};
    static const unsigned counts[]={2,3,3,2,1,0,4,1,0};
    BackendSymbol registration,finally_stack;ExceptionScope scope;SymbolHeader label;
    Node node;MachineInstruction *instruction;unsigned i,j;MachineOperand *destination,*source;
    memset(&registration,0,sizeof registration);memset(&finally_stack,0,sizeof finally_stack);
    memset(&label,0,sizeof label);
    for(i=0;i<RECON_COUNT(codes);++i){
        memset(&node,0,sizeof node);memset(&scope,0,sizeof scope);
        node.opcode=0x4f;node.left.pointer=&scope;node.detail.control.code=(short)codes[i];
        scope.identifier=9;scope.parent_identifier=-1;scope.kind=i==3?1:2;
        scope.alternate_label=i==2?&label:0;scope.flags=i==6?1:0;
        seh_registration=&registration;seh_finally_stack=&finally_stack;
        machine_instructions=machine_tail=0;temporary_operand_count=0;
        if(select_assembly_record(&node)!=0x1a)return 0;
        instruction=machine_instructions;
        for(j=0;j<counts[i];++j){
            if(!instruction||((instruction->opcode>>8)&255)!=expected[i][j])return 0;
            destination=&instruction->operands[0];source=&instruction->operands[1];
            if(expected[i][j]==MC_MOV&&i!=6){
                if((destination->flags&15)!=OPERAND_BASED||
                    destination->value.address->symbol!=&registration.symbol||
                    destination->value.address->first.displacement!=12||
                    source->flags!=OPERAND_IMMEDIATE||
                    source->value.constant!=(i==0?9:~0U))return 0;
            }
            if(expected[i][j]==MC_PUSH){
                if(i==2){if((destination->flags&15)!=OPERAND_LABEL_ADDRESS||
                    destination->value.address->symbol!=&label)return 0;}
                else if(destination->flags!=OPERAND_IMMEDIATE||destination->value.constant)return 0;
            }
            if(expected[i][j]==MC_RET&&(destination->flags!=OPERAND_IMMEDIATE||
                destination->value.constant))return 0;
            if(i==6){
                if(j==0){if(source->value.address->symbol!=&registration.symbol||
                    source->value.address->first.displacement!=20)return 0;}
                else if(j==1||j==2){if((source->flags&15)!=OPERAND_BASED||source->value.address->symbol||
                    source->value.address->first.displacement||
                    source->value.address->base.words.number!=destination->value.reg.words.number)return 0;}
                else if(destination->value.address->symbol!=&finally_stack.symbol)return 0;
            }
            instruction=instruction->next;
        }
        if(instruction||seh_registration!=&registration||seh_finally_stack!=&finally_stack||
           scope.identifier!=9||scope.parent_identifier!=-1)return 0;
    }
    return 1;
}
static int seh_intrinsic_operations(void)
{
    static const unsigned ids[]={0xf2,0xf2,0xf4,0xd9,0xda,0xd8,0xdb,0xdc};
    static const unsigned expected[][3]={{MC_MOV,0,0},{MC_MOV,MC_SUB,MC_MOV},
        {MC_EFWAIT,MC_MOV,0},{MC_MOV,0,0},{MC_MOV,0,0},{MC_MOV,0,0},{MC_MOV,0,0},{0,0,0}};
    static const unsigned counts[]={1,3,2,1,1,1,1,0};
    BackendSymbol registration,finally_stack,saved_stack;Node node,callee,argument,list;
    MachineInstruction *instruction;unsigned i,j;unsigned short result;
    memset(&registration,0,sizeof registration);memset(&finally_stack,0,sizeof finally_stack);
    memset(&saved_stack,0,sizeof saved_stack);
    for(i=0;i<RECON_COUNT(ids);++i){
        memset(&node,0,sizeof node);memset(&callee,0,sizeof callee);
        memset(&argument,0,sizeof argument);memset(&list,0,sizeof list);
        node.opcode=i&1?0x7a:0x40;node.left.node=&callee;node.second.node=&list;
        callee.left.value=ids[i];list.left.node=&argument;argument.left.value=i==0?0:20;
        seh_registration=&registration;seh_finally_stack=&finally_stack;seh_saved_stack=&saved_stack;
        machine_instructions=machine_tail=0;temporary_operand_count=0;
        result=select_assembly_record(&node);instruction=machine_instructions;
        if(i==3||i==4){if(!instruction||result!=instruction->operands[0].value.reg.words.number)return 0;}
        else if(result!=0x1a)return 0;
        for(j=0;j<counts[i];++j){
            if(!instruction||((instruction->opcode>>8)&255)!=expected[i][j])return 0;
            if(i==1&&j==1&&(instruction->operands[1].flags!=OPERAND_IMMEDIATE||
                instruction->operands[1].value.constant!=20))return 0;
            if(i==2&&j==1&&(instruction->operands[0].value.address->symbol!=&registration.symbol||
                instruction->operands[0].value.address->first.displacement!=12||
                instruction->operands[1].value.constant!=20))return 0;
            if((i==3||i==4)&&(instruction->operands[1].value.address->symbol!=
                (i==3?&finally_stack.symbol:&registration.symbol)||
                instruction->operands[1].value.address->first.displacement!=(i==3?0:20)))return 0;
            instruction=instruction->next;
        }
        if(instruction)return 0;
    }
    return 1;
}
static unsigned missing_seh_code;
static void __fastcall missing_seh_owner(char *source,unsigned code)
{
    recon_native_entry_finish(code==missing_seh_code&&!strcmp(source,"@(#)cgintrin.c:1.57"));
}
static void invoke_missing_seh_owner(void *argument)
{
    unsigned index=(unsigned)argument;Node node,callee;ExceptionScope scope;
    memset(&node,0,sizeof node);memset(&callee,0,sizeof callee);memset(&scope,0,sizeof scope);
    seh_finally_stack=seh_registration=seh_saved_stack=0;
    if(index==2)node.opcode=0x47;
    else if(index==3){node.opcode=0x4f;node.detail.control.code=0x28;
        node.left.pointer=&scope;scope.flags=1;}
    else{node.opcode=0x40;node.left.node=&callee;callee.left.value=index?0xd9:0xd8;}
    select_assembly_record(&node);
}
static int seh_missing_owner_boundaries(void)
{
    static const unsigned codes[]={0x409,0x3fb,0x412,0x3b7};unsigned i;
    for(i=0;i<RECON_COUNT(codes);++i){missing_seh_code=codes[i];
        if(recon_native_at_entry((const void *)compiler_internal_error,
            (const void *)missing_seh_owner,invoke_missing_seh_owner,(void *)i)!=1)return 0;}
    return 1;
}
static int seh_scope_table_records(void)
{
    Function function;BackendSymbol table;ExceptionScope scopes[3];SymbolHeader labels[6];
    Initializer *block,*datum;unsigned row,i,j;long parents[]={-1,0,0x7fffffffL};
    memset(&function,0,sizeof function);memset(&table,0,sizeof table);
    memset(labels,0,sizeof labels);for(i=0;i<6;++i)labels[i].identifier=100+i;
    current_function=&function;seh_scope_table=&table;table.symbol.identifier=991;
    global_initializers=0;initialize_exception_scope_table();block=global_initializers;
    if(!block||block->kind||block->next||block->content.reference.link||
       block->content.reference.value||block->content.reference.symbol_id!=991)return 0;
    for(row=0;row<4;++row){
        memset(scopes,0,sizeof scopes);function.exception_scopes=scopes;
        for(i=0;i<3;++i){scopes[i].next=i==2?0:&scopes[i+1];scopes[i].parent_identifier=parents[i];
            scopes[i].filter_label=row&1?&labels[i*2]:0;
            scopes[i].handler_label=row&2?&labels[i*2+1]:0;}
        global_initializers=0;initialize_exception_scope_table();block=global_initializers;
        if(!block||block->kind||block->content.reference.symbol_id!=991||
           block->content.reference.link||block->content.reference.value)return 0;
        datum=block->next;
        for(i=0;i<3;++i)for(j=0;j<3;++j){
            SymbolHeader *label=j==1?scopes[i].filter_label:scopes[i].handler_label;
            if(!datum)return 0;
            if(j&&label){if(datum->kind!=2||datum->content.relocation.offset||
                datum->content.relocation.flags!=4||datum->repeat||
                datum->content.relocation.symbol_id!=label->identifier)return 0;}
            else if(datum->kind!=1||datum->repeat!=1||datum->content.datum.bytes!=4||
                datum->content.datum.type!=1||datum->content.datum.value.integer!=(j?0:parents[i]))return 0;
            datum=datum->next;
        }
        if(datum||function.exception_scopes!=scopes||table.symbol.identifier!=991)return 0;
    }
    return 1;
}
unsigned __fastcall external_name_hash(char *name);
SymbolHeader *__fastcall require_external_symbol(char *name);
static int external_data_cache_identity(void)
{
    char first_name[]="ab",second_name[]="ba";SymbolHeader *first,*second,*function;
    unsigned first_id;
    next_serial=1; /* The cold constructor requires the normal nonzero module serial. */
    if(external_name_hash(first_name)!=external_name_hash(second_name))return 0;
    first=lookup_external_name(first_name);first_id=first->identifier;
    if(first->kind!=1||first->value!=4||first->field0d||first->name==first_name||
       strcmp(first->name,"ab"))return 0;
    second=lookup_external_name(second_name);
    if(second==first||second->kind!=1||second->value!=4||second->field0d||
       second->name==second_name||strcmp(second->name,"ba"))return 0;
    first_name[0]='z';second_name[0]='z';
    if(lookup_external_name("ab")!=first||lookup_external_name("ba")!=second||
       first->identifier!=first_id||strcmp(first->name,"ab")||strcmp(second->name,"ba")||
       require_external_symbol("ab")!=first||first->kind!=1)return 0;
    function=require_external_symbol("c2_cache_function");
    return function->kind==4&&lookup_external_name("c2_cache_function")==function&&
        function->kind==4&&lookup_external_name("")==lookup_external_name("");
}
static void __fastcall external_cache_full_entry(char *source,unsigned code)
{
    recon_native_entry_finish(code==0x545&&!strcmp(source,"@(#)p2symtab.c:1.89"));
}
static void fill_external_cache(void *argument)
{
    char name[24];unsigned i;
    next_serial=1; /* Valid cold module state, not a public-invocation override. */
    for(i=0;i<128;++i){sprintf(name,"c2_data_%u",i);lookup_external_name(name);}
    lookup_external_name("c2_data_missing");
}
static int external_cache_capacity_boundary(void)
{
    return recon_native_at_entry((const void *)compiler_internal_error,
        (const void *)external_cache_full_entry,fill_external_cache,0)==1;
}
static int narrowed_comparison_conditions(void)
{
    /* Independently decoded original16B table, not a host condition formula. */
    static const int expected[]={0,1,2,3,4,5,6,7,9,9,10,11,2,3,6,7};unsigned i;
    for(i=0;i<RECON_COUNT(expected);++i)
        if(narrow_comparison_condition(i)!=expected[i])return 0;
    return 1;
}
static int coff_dword_little_endian(void)
{
    static const unsigned values[]={0,0x89abcdefU,~0U};CoffScope scope;CoffDataBlock blocks[2];
    unsigned i,j;
    for(i=0;i<RECON_COUNT(values);++i){
        memset(&scope,0,sizeof scope);memset(blocks,0x3c,sizeof blocks);
        scope.kind=1;scope.block=blocks;blocks[0].next=blocks;coff_active_scope=&scope;
        coff_write_dword(values[i]);
        if(scope.buffered_bytes!=4||scope.completed_bytes||scope.write_count!=1||scope.block!=blocks)return 0;
        for(j=0;j<4;++j)if(blocks[0].bytes[j]!=(unsigned char)(values[i]>>(j*8)))return 0;
        for(j=4;j<500;++j)if(blocks[0].bytes[j]!=0x3c)return 0;
    }
    memset(&scope,0,sizeof scope);memset(blocks,0x3c,sizeof blocks);
    scope.kind=1;scope.block=blocks;scope.buffered_bytes=498;blocks[0].next=blocks;
    blocks[1].next=0;coff_free_blocks=&blocks[1];coff_active_scope=&scope;
    coff_write_dword(0x89abcdefU);
    if(scope.completed_bytes!=500||scope.buffered_bytes!=2||scope.write_count!=1||
       scope.block!=&blocks[1]||blocks[0].next!=&blocks[1]||blocks[1].next!=blocks||coff_free_blocks||
       blocks[0].bytes[498]!=0xef||blocks[0].bytes[499]!=0xcd||
       blocks[1].bytes[0]!=0xab||blocks[1].bytes[1]!=0x89)return 0;
    for(j=0;j<498;++j)if(blocks[0].bytes[j]!=0x3c)return 0;
    for(j=2;j<500;++j)if(blocks[1].bytes[j]!=0x3c)return 0;
    scope.value=1;coff_write_dword(~0U);
    return scope.buffered_bytes==6&&scope.completed_bytes==500&&scope.write_count==2&&
        blocks[1].bytes[2]==0x3c&&blocks[1].bytes[3]==0x3c;
}
typedef struct ScopeFrameCapture {
    unsigned offset,bytes,locals;unsigned short parameters;unsigned char prologue,attributes;
} ScopeFrameCapture;
static ScopeFrameCapture captured_scope_frames[4];
static unsigned scope_frame_events,scope_frame_count;static int scope_frame_valid;
static Segment *scope_frame_segment;static SymbolHeader *scope_frame_symbol;
static void __fastcall capture_scope_frame_segment(Segment *segment)
{
    if(scope_frame_events%3||segment!=scope_frame_segment)scope_frame_valid=0;
    ++scope_frame_events;
}
static void __fastcall capture_scope_frame_data(void *data,unsigned bytes)
{
    if(scope_frame_events%3!=1||bytes!=16||scope_frame_count==4)scope_frame_valid=0;
    else memcpy(&captured_scope_frames[scope_frame_count++],data,bytes);
    ++scope_frame_events;
}
static void __fastcall capture_scope_frame_address(unsigned offset,SymbolHeader *symbol)
{
    if(scope_frame_events%3!=2||offset!=0U-16||symbol!=scope_frame_symbol)scope_frame_valid=0;
    ++scope_frame_events;
}
static int function_scope_frame_ranges(void)
{
    static const unsigned handlers[]={0x150,0x120,0x180,0x1f0};
    static const unsigned starts[]={0x130,0x140,0x1d0,0x1e0};
    static const unsigned offsets[]={0x30,0x40,0x80,0xf0},lengths[]={0x20,0x20,0x50,0x50};
    Function function;FunctionSymbol symbol;ExceptionScope scopes[5];SymbolHeader labels[8];
    Segment segment;unsigned i;extern int coff_object;
    memset(&function,0,sizeof function);memset(&symbol,0,sizeof symbol);
    memset(scopes,0,sizeof scopes);memset(labels,0,sizeof labels);memset(&segment,0,sizeof segment);
    function.symbol=&symbol.symbol;symbol.symbol.field04=0x100;
    object_filename="scope-frame-test.obj";coff_object=1;coff_function_fpo_segment=&segment;
    scope_frame_events=scope_frame_count=0;scope_frame_valid=1;
    scope_frame_segment=&segment;scope_frame_symbol=&symbol.symbol;
    output_callbacks.select_segment=capture_scope_frame_segment;output_callbacks.data=capture_scope_frame_data;
    output_callbacks.address=capture_scope_frame_address;
    emit_function_inline_data(&function);if(scope_frame_events)return 0;
    function.exception_scopes=scopes;
    for(i=0;i<5;++i){scopes[i].next=i==4?0:&scopes[i+1];scopes[i].kind=i<2?1:2;
        if(i==4)continue;
        labels[i*2].field04=handlers[i];labels[i*2+1].field04=starts[i];
        scopes[i].handler_label=&labels[i*2];
        if(i<2)scopes[i].filter_label=&labels[i*2+1];else scopes[i].alternate_label=&labels[i*2+1];}
    emit_function_inline_data(&function);
    if(!scope_frame_valid||scope_frame_events!=12||scope_frame_count!=4||segment.symbol.field04!=64)return 0;
    for(i=0;i<4;++i)if(captured_scope_frames[i].offset!=offsets[i]||
        captured_scope_frames[i].bytes!=lengths[i]||captured_scope_frames[i].locals||
        captured_scope_frames[i].parameters||captured_scope_frames[i].prologue||
        captured_scope_frames[i].attributes!=8||labels[i*2].field04!=handlers[i]||
        labels[i*2+1].field04!=starts[i])return 0;
    return symbol.symbol.field04==0x100&&function.exception_scopes==scopes;
}
static void __fastcall invalid_scope_frame_kind(char *source,unsigned code)
{
    recon_native_entry_finish(code==0x334&&!strcmp(source,"@(#)except.c:1.25"));
}
static void invoke_invalid_scope_frame(void *argument)
{
    Function function;ExceptionScope scope;SymbolHeader handler;
    memset(&function,0,sizeof function);memset(&scope,0,sizeof scope);memset(&handler,0,sizeof handler);
    function.exception_scopes=&scope;scope.kind=3;scope.handler_label=&handler;
    emit_function_inline_data(&function);
}
static int function_scope_frame_kind_boundary(void)
{
    return recon_native_at_entry((const void *)compiler_internal_error,
        (const void *)invalid_scope_frame_kind,invoke_invalid_scope_frame,0)==1;
}
const ReconTest c2_tests[]={
    {"actual SEH owned48B allocation at parent query",1,c2_scope_owned_record},
    {"nearest active try scope through valid20-slot stack",1,enclosing_scopes},
    {"actual fastcall scope pointer/defined48B before push",1,c2_scope_push_inputs},
    {"scope push opaque-pointer store and20-slot boundary",1,push_scopes},
    {"scope push overflow reaches exact pending diagnostic",1,push_overflow_boundary},
    {"actual current query consumes two live owned pointers",1,c2_scope_current_inputs},
    {"current scope pointer identity with valid active depths",1,current_scopes},
    {"actual scope pop receives two owned active pointers",1,c2_scope_pop_inputs},
    {"scope pop clears exactly the active top slot",1,pop_scopes},
    {"scope pop underflow reaches exact pending diagnostic",1,pop_underflow_boundary},
    {"actual rewrite Function formal and expression-list owner",1,c2_scope_rewrite_inputs},
    {"SEH rewrite empty and ordinary streams preserve owners",1,rewrite_plain_stream},
    {"actual annotation CX and active-scope identity",1,c2_scope_annotation_inputs},
    {"annotation code width and borrowed scope pointer",1,annotation_values},
    {"actual unwind previous/tree/stack target and scope owner",1,c2_scope_unwind_inputs},
    {"unwind absent/same/outermost-finally bypasses preserve state",1,unwind_bypass},
    {"actual finally-unwind scalar ECX target",1,c2_scope_finally_inputs},
    {"finally-aware unwind call typed target and intrinsic identity",1,finally_unwind_call},
    {"actual finally-depth constructor scalar ECX",1,c2_finally_inputs},
    {"finally depth becomes four-byte displacement call argument",1,finally_depth_call},
    {"encountered SEH intrinsic recipes preserve typed arguments",1,seh_intrinsic_lowering},
    {"SEH status fixed signature and empty recipe",1,seh_status_lowering},
    {"encountered SEH d9 scalar default and empty recipe",1,seh_d9_lowering},
    {"actual scope finalizer Function and expression owner",1,c2_scope_finish_inputs},
    {"scope finalizer transfers list and clears temporary sets",1,finish_scope_owners},
    {"scope finalizer unbalanced filter diagnostic boundary",1,finish_filter_boundary},
    {"scope finalizer unbalanced protected diagnostic boundary",1,finish_protected_boundary},
    {"finalizer both eight-entry annotation maps balance",1,finish_marker_maps},
    {"actual collect tree ECX/filter EDX and current scope",1,c2_scope_collect_inputs},
    {"scope collector leaf/binary/call paths and retained flags",1,collect_tree_paths},
    {"scope collector missing active owner diagnostic",1,collect_missing_scope_boundary},
    {"actual remembered symbol identifier and owning scope",1,c2_scope_remember_inputs},
    {"remembered symbol is unused without active scope",1,remember_without_scope},
    {"actual empty64B local set and arena ownership",1,c2_scope_find_local_inputs},
    {"scope-local hash identity collisions/misses and read-only owners",1,local_hash_lookup},
    {"actual local insertion set/symbol aliases after read-only find",1,c2_scope_insert_local_inputs},
    {"local insertion prepends distinct collision/duplicate links",1,local_hash_insert},
    {"actual marking leaf ECX/filter EDX and scope state",1,c2_scope_mark_inputs},
    {"scope marking distinguishes loads/stores and absent symbols",1,mark_reference_flags},
    {"actual visible-local symbol and parent-scope contract",1,c2_scope_visible_inputs},
    {"visible local packed-tag/no-owner/borrowed-id match",1,visible_local_paths},
    {"actual parent lookup ECX head/EDX identifier and aliases",1,c2_scope_lookup_inputs},
    {"scope lookup sentinel/first identity and unchanged links",1,scope_list_lookup},
    {"scope lookup missing identifier exact diagnostic boundary",1,scope_missing_identifier_boundary},
    {"actual protected-edge Work owners and empty directional lists",1,c2_protected_edge_inputs},
    {"protected reciprocal edges coexist/append/deduplicate/self-link",1,protected_edge_pairs},
    {"protected allocation retains recycled high flags",1,protected_recycled_flags},
    {"actual25-work region aliases/numbers/initial last",1,c2_refresh_region_inputs},
    {"region last uses signed maxima and retains tie identity",1,region_last_signed_order},
    {"actual excluded-register root/Expression/Work ownership",1,c2_restore_excluded_inputs},
    {"excluded registers preserve signed-short mask across memory shapes",1,restore_excluded_numbers},
    {"excluded restoration traverses binary and special leaf owners",1,restore_excluded_tree_paths},
    {"actual definition-end Work/value/stack-range and binding aliases",1,c2_register_ends_inputs},
    {"definition ends insert typed markers after matching writes",1,register_end_annotations},
    {"definition range compares signed tree and unsigned packed number",1,register_end_signed_comparison},
    {"actual assembly selector Node ECX and defined28B fields",1,c2_assembly_record_inputs},
    {"SEH annotations preserve operation order and registration owners",1,seh_annotation_operations},
    {"SEH intrinsic memory operations and returned register identities",1,seh_intrinsic_operations},
    {"SEH missing owners reach exact diagnostic boundaries",1,seh_missing_owner_boundaries},
    {"actual scope-table initializer hidden function/two-scope/symbol owners",1,c2_scope_table_inputs},
    {"scope table owns threeDWORD rows and optional label relocations",1,seh_scope_table_records},
    {"actual external-name ECX consumes exact except-list literal",1,c2_external_name_inputs},
    {"external data collisions/copies share function-cache identities",1,external_data_cache_identity},
    {"external cache full128-slot wrap reaches exact diagnostic",1,external_cache_capacity_boundary},
    {"actual narrow comparison scalar condition4 in ECX",1,c2_narrow_condition_inputs},
    {"narrowed conditions preserve all16 original signed-byte rows",1,narrowed_comparison_conditions},
    {"actual COFF DWORD scalarECX preserves negative-one bits",1,c2_coff_dword_inputs},
    {"COFF DWORD little-endian bytes cross500-byte scope boundary",1,coff_dword_little_endian},
    {"actual function/scope/label offsets at SEH frame-range output",1,c2_function_ranges_inputs},
    {"SEH frame ranges retain length across nonincreasing pairs",1,function_scope_frame_ranges},
    {"SEH invalid frame-scope kind reaches exact diagnostic",1,function_scope_frame_kind_boundary}
};
