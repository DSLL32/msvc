/* Test-only metadata interfaces at a genuine completed source call. */
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../src/pch-relocate.h"
#include "../../src/expressions.h"
#include "../../src/browser.h"
#include "../../src/debug-info.h"
#include "../../src/literal-names.h"
#include <recon.h>
extern int __real_main(int,char **,char **);
static unsigned selected_unit,invoked;
static int report_descriptor;
static ArgumentNode *nodes[3];
static unsigned initial_nodes[3][5];
static unsigned char *mark_snapshots[511];
static unsigned vector_fields[3],vector_items[16];
static unsigned encoded_fields[3],encoded_items[16];
static unsigned copy_capacity,copy_bytes,copy_available;
static unsigned char *copy_cursor,copy_before[64];
static FrontendSymbol *qualified_symbol;
static unsigned qualifier_words[2];
static unsigned browser_initial[4],browser_template[6],browser_bytes;
static unsigned char browser_prefix[4096];
static void fail(void) { syscall(SYS_exit_group,88); }
static void emit(char *line)
{
    unsigned bytes=strlen(line);
    if(syscall(SYS_write,report_descriptor,line,bytes)!=(int)bytes)fail();
}
static int permanent(void *pointer,unsigned bytes)
{
    unsigned address=(unsigned)pointer,start,i;
    if(!address||address>0xffffffffU-bytes)return 0;
    for(i=0;i<(unsigned)permanent_page_count;++i) {
        start=(unsigned)permanent_pages[i].start;
        if(address>=start&&address-start<=0xeffc&&bytes<=0xeffc-(address-start))return 1;
    }
    return 0;
}
static void temporary(void *pointer,unsigned bytes)
{
    ArenaPool *pool=&compiler_arena.pools[2];ArenaBlock *block=pool->first;
    unsigned i,count=0,used,owned=0,address=(unsigned)pointer,start;
    if(!address||address>0xffffffffU-bytes)fail();
    while(block) {
        if(++count>32||block->bytes>0x3ff4)fail();
        used=block==pool->last?(unsigned)pool->next-(unsigned)block->data:block->bytes;
        if(used>0x3ff4||(block==pool->last&&(block->next||used+(unsigned)pool->remaining!=0x3ff4)))fail();
        start=(unsigned)block->data;
        if(address>=start&&address-start<=used&&bytes<=used-(address-start))owned=1;
        if(!block->next&&block!=pool->last)fail();block=block->next;
    }
    if(!owned)fail();
    for(i=0;i<(unsigned)permanent_page_count;++i) {
        start=(unsigned)permanent_pages[i].start;
        if(address<start+0xeffc&&address+bytes>start)fail();
    }
}
static void pending(char *when)
{
    FILE *streams[7];unsigned i,j,count,n;char line[8300];
    static char digits[]="0123456789abcdef";
    streams[0]=expression_file;streams[1]=symbol_file;streams[2]=line_file;
    streams[3]=global_file;streams[4]=debug_file;streams[5]=pch_file;streams[6]=browser_output;
    sprintf(line,"metadata-streams %s\n",when);emit(line);
    for(i=0;i<7;++i) {
        n=sprintf(line,"buffer %u ",i);
        if(!streams[i])n+=sprintf(line+n,"unopened\n");
        else {
            count=(unsigned)streams[i]->_ptr-(unsigned)streams[i]->_base;if(count>4096)fail();
            n+=sprintf(line+n,"%u ",count);
            for(j=0;j<count;++j){unsigned byte=(unsigned char)streams[i]->_base[j];line[n++]=digits[byte>>4];line[n++]=digits[byte&15];}line[n++]='\n';line[n]=0;
        }
        emit(line);
    }
}
static void cache(char *when)
{
    char line[160];unsigned i,j;
    if(pch_reference_base&&(pch_reference_page>=(unsigned)permanent_page_count||
       pch_reference_base!=permanent_pages[pch_reference_page].start))fail();
    if(!strcmp(when,"before"))for(i=0;i<3;++i)for(j=1;j<5;++j)
        if(initial_nodes[i][j]&&(unsigned)((unsigned)nodes[i]+4*j-(unsigned)pch_reference_base)<=0xeffc)fail();
    sprintf(line,"metadata-cache %s reference %08x base %08x page %u\n",when,pch_reference,(unsigned)pch_reference_base,pch_reference_page);emit(line);
    if(!strcmp(when,"before"))emit("metadata-cache-path before lookup-for-all-nonnull-slots\n");
}
static void vector(Vector *owner,char *label)
{
    unsigned i;char line[500];char *text;
    if(!permanent(owner,12)||owner->count<3||owner->count>16||owner->capacity<owner->count||owner->capacity>256||owner->arena||!permanent(owner->items,4*owner->count))fail();
    sprintf(line,"metadata-vector %s %08x count %d capacity %d arena %d items %08x\n",label,(unsigned)owner,owner->count,owner->capacity,owner->arena,(unsigned)owner->items);emit(line);
    for(i=0;i<(unsigned)owner->count;++i) {
        text=owner->items[i];if(!permanent(text,1)||strlen(text)>=260||!permanent(text,strlen(text)+1))fail();
        sprintf(line,"metadata-string %s-%u %08x %s\n",label,i,(unsigned)text,text);emit(line);
    }
}
static FrontendSymbol *global_symbol(char *name)
{
    unsigned bucket,count;FrontendSymbol *symbol;
    if(!global_scope||global_scope->bucket_mask>1023)fail();
    for(bucket=0;bucket<=global_scope->bucket_mask;++bucket) {
        for(symbol=global_scope->buckets[bucket],count=0;symbol;symbol=symbol->next) {
            if(++count>1024)fail();if(symbol->name&&!strcmp(symbol->name,name))return symbol;
        }
    }
    fail();return 0;
}
static void browser_after(unsigned call)
{
    unsigned *fields=(unsigned *)browser_output;char line[180];
    if(browser_suppressed!=1||fields[0]!=browser_initial[0]+1||fields[1]!=browser_initial[1]-1||
       fields[2]!=browser_initial[2]||fields[3]!=browser_initial[3]||
       memcmp(browser_output->_base,browser_prefix,browser_bytes)||(unsigned char)browser_output->_base[browser_bytes]!=14||
       memcmp(&browser_line_template,browser_template,24)||auxiliary_ir_stream!=browser_output)fail();
    sprintf(line,"metadata-browser-after call%u suppressed1 appended%s pending%u template-unchanged24 aux-browser1\n",call,call==1?"0e":"none",browser_bytes+1);emit(line);
}
void __stdcall __wrap_finish_call_arguments(IrToken *call)
{
    ArgumentNode *node;ArenaBlock *block;unsigned i,j,start,before,after;
    unsigned short control_before,control_after;char line[300];Vector *result,*encoded;unsigned char *copied;
    unsigned char pascal_output[256],work_output[256];short pascal_result;unsigned qualifier_result,format_result,read_result;
    WorkingStack *owned_work=0,*old_work;char *format="%s:%d";unsigned char *work_storage;
    if(invoked++||selected_unit>9||!call_context||call_context->argument_count!=3||
       !call_context->symbol||strcmp(call_context->name,"metadata_sink")||
       strcmp(call_context->symbol->name,"metadata_sink")||call->kind!=TOK_ARGUMENT_END||call->flags!=2||
       compiler_arena.block_bytes!=0x3ff4||permanent_page_count<1||permanent_page_count>511)fail();
    for(i=0;i<(unsigned)permanent_page_count;++i) {
        start=(unsigned)permanent_pages[i].start;
        if(!start||(start&4095)||start>0xffffffffU-0xeffc||!permanent_pages[i].relocations||permanent_pages[i].relocations->bit_count!=0xeffc)fail();
        for(j=0;j<i;++j)if(start<(unsigned)permanent_pages[j].start+0xeffc&&start+0xeffc>(unsigned)permanent_pages[j].start)fail();
        sprintf(line,"metadata-page %u start %08x bytes0000effc marks %08x bits %u\n",i,start,(unsigned)permanent_pages[i].relocations,permanent_pages[i].relocations->bit_count);emit(line);
        mark_snapshots[i]=malloc(7680);if(!mark_snapshots[i])fail();memcpy(mark_snapshots[i],permanent_pages[i].relocations->bits,7680);
    }
    temporary(call,20);temporary(call_context,36);
    for(block=compiler_arena.pools[2].first,i=0;block;block=block->next,++i) {
        unsigned used=block==compiler_arena.pools[2].last?(unsigned)compiler_arena.pools[2].next-(unsigned)block->data:block->bytes;
        sprintf(line,"metadata-arena-block %u address %08x allocation 16384 data %08x used %u next %08x last %u\n",i,(unsigned)block,(unsigned)block->data,used,(unsigned)block->next,block==compiler_arena.pools[2].last);emit(line);
    }
    node=(ArgumentNode *)((ExpressionNode *)call)->value.children.right;
    for(i=0;i<3;++i) {
        temporary(node,20);
        if(node->kind!=TOK_ARGUMENT||node->flags!=2||!permanent(node->type,8)||!node->expression)fail();
        nodes[i]=node;memcpy(initial_nodes[i],node,20);
        sprintf(line,"metadata-argument %u address %08x type %08x next %08x expression %08x previous %08x owned20 outside-all-pages\n",i,(unsigned)node,(unsigned)node->type,(unsigned)node->next,(unsigned)node->expression,(unsigned)node->previous);emit(line);node=node->previous;
    }
    if(node)fail();for(i=0;i<3;++i)if(nodes[i]->next!=nodes[(i+2)%3])fail();
    vector(pch_saved_defines,"defines");vector(pch_saved_include_directories,"directories");
    if(selected_unit>=1){memcpy(vector_fields,pch_saved_defines,12);memcpy(vector_items,pch_saved_defines->items,4*pch_saved_defines->count);}
    cache("before");pending("before");
    if(selected_unit==2) {
        encoded=new_vector(pch_saved_defines->count,0);
        if(!permanent(encoded,12)||encoded->count||encoded->capacity!=pch_saved_defines->count||encoded->arena||!permanent(encoded->items,4*encoded->capacity))fail();
        for(i=0;i<(unsigned)pch_saved_defines->count;++i) {
            pointer_to_pch_reference(pch_saved_defines->items[i]);
            encoded_items[i]=pch_reference;
            if((pch_reference>>16)>=(unsigned)permanent_page_count||(pch_reference&65535)>=0xeffc||
               permanent_pages[pch_reference>>16].start+(pch_reference&65535)!=pch_saved_defines->items[i])fail();
            sprintf(line,"metadata-reference %u original %08x encoded %08x\n",i,(unsigned)pch_saved_defines->items[i],pch_reference);emit(line);
            append_vector(encoded,(void *)pch_reference);
            if(encoded->count!=i+1||memcmp(encoded->items,encoded_items,4*(i+1)))fail();
        }
        memcpy(encoded_fields,encoded,12);
        sprintf(line,"metadata-encoded-vector before owner %08x count%u capacity%u arena0 items %08x\n",(unsigned)encoded,encoded->count,encoded->capacity,(unsigned)encoded->items);emit(line);cache("prepared");
    }
    if(selected_unit==3||selected_unit==4) {
        copy_bytes=pch_saved_defines->count*4;copy_capacity=selected_unit==3?copy_bytes+8:10;
        copy_cursor=permanent_next;copy_available=permanent_available;
        if(copy_capacity+16>sizeof(copy_before)||copy_available<copy_capacity+16||!permanent(copy_cursor,copy_capacity+16)||
           ((unsigned)copy_cursor<(unsigned)pch_saved_defines->items+copy_bytes&&(unsigned)copy_cursor+copy_capacity>(unsigned)pch_saved_defines->items))fail();
        memcpy(copy_before,copy_cursor,copy_capacity+16);
        sprintf(line,"metadata-copy-input cursor %08x available %u source %08x bytes%u allocation%u\n",(unsigned)copy_cursor,copy_available,(unsigned)pch_saved_defines->items,copy_bytes,copy_capacity);emit(line);
    }
    if(selected_unit==5) {
        if(strlen(call_context->symbol->name)>=128)fail();memset(pascal_output,0xa5,sizeof(pascal_output));
        sprintf(line,"metadata-pascal-input buffer %08x allocation256 symbol %08x name %08x length%u text %s\n",(unsigned)pascal_output,(unsigned)call_context->symbol,(unsigned)call_context->symbol->name,strlen(call_context->symbol->name),call_context->symbol->name);emit(line);
    }
    if(selected_unit==6||selected_unit==7) {
        qualified_symbol=global_symbol(selected_unit==6?"metadata_qualified":"metadata_mutable");
        if(!permanent(qualified_symbol,16)||!permanent(qualified_symbol->type,8)||qualified_symbol->type->related||
           !!(qualified_symbol->type->attributes&0x3000)!=(selected_unit==6))fail();
        memcpy(qualifier_words,qualified_symbol->type,8);
        sprintf(line,"metadata-qualifier-input symbol %08x name %s type %08x flags%04x attributes%04x related%08x\n",(unsigned)qualified_symbol,qualified_symbol->name,(unsigned)qualified_symbol->type,qualified_symbol->type->flags,qualified_symbol->type->attributes,(unsigned)qualified_symbol->type->related);emit(line);
    }
    if(selected_unit==8) {
        if(!browser_output||browser_suppressed||!(browser_output->_flag&2)||browser_output->_cnt<2)fail();
        memcpy(browser_initial,browser_output,16);memcpy(browser_template,&browser_line_template,24);
        for(i=0;i<6;++i)if(browser_template[i])fail();
        browser_bytes=(unsigned)browser_output->_ptr-(unsigned)browser_output->_base;
        if(browser_bytes>4094)fail();memcpy(browser_prefix,browser_output->_base,browser_bytes);
        sprintf(line,"metadata-browser-input stream %08x pointer %08x base %08x count %u flags %08x pending%u suppressed0 template-zero24 hook-known-empty1\n",(unsigned)browser_output,browser_initial[0],browser_initial[2],browser_initial[1],browser_initial[3],browser_bytes);emit(line);
    }
    if(selected_unit==9) {
        old_work=compiler_stack;if(!old_work)fail();memset(work_output,0xa5,sizeof(work_output));
        sprintf(line,"metadata-format-setup slot %08x initial-null1 format %08x output %08x allocation256 original-work %08x\n",(unsigned)&owned_work,(unsigned)format,(unsigned)work_output,(unsigned)old_work);emit(line);
        initialize_work_stack(&owned_work);
        if(!owned_work||owned_work==old_work||compiler_stack!=old_work||owned_work->count||!owned_work->storage||
           owned_work->next!=owned_work->storage||owned_work->read!=owned_work->storage||
           ((WorkingBlock *)owned_work->storage)->next||((WorkingBlock *)owned_work->storage)->used_bytes)fail();
        work_storage=owned_work->storage;
        sprintf(line,"metadata-format-input buffer %08x owner16 storage %08x block2048 format %08x name %08x number%u count0 trailer-zero8 original-work-distinct1\n",(unsigned)owned_work,(unsigned)work_storage,(unsigned)format,(unsigned)call_context->symbol->name,pch_saved_defines->count);emit(line);
    }
    __asm { mov before,esp }
    __asm { fnstcw control_before }
    if(selected_unit==0)mark_pch_ir_node((IrToken *)nodes[0]);
    else if(selected_unit==1) {
        result=encode_pch_vector(pch_saved_defines);
        if(result!=pch_saved_defines||memcmp(vector_fields,result,12)||memcmp(vector_items,result->items,4*result->count))fail();
        sprintf(line,"metadata-vector-after unit1 owner %08x items-unchanged%u returned-owner1\n",(unsigned)result,result->count);emit(line);
    } else if(selected_unit==2) {
        result=decode_pch_vector(encoded);
        if(result!=encoded||memcmp(encoded_fields,result,12)||memcmp(encoded_items,result->items,4*result->count)||
           memcmp(vector_fields,pch_saved_defines,12)||memcmp(vector_items,pch_saved_defines->items,4*pch_saved_defines->count))fail();
        sprintf(line,"metadata-vector-after unit2 owner %08x items-unchanged%u returned-owner1\n",(unsigned)result,result->count);emit(line);
    } else if(selected_unit==3||selected_unit==4) {
        copied=copy_permanent_bytes(pch_saved_defines->items,copy_bytes,copy_capacity);
        i=copy_bytes<copy_capacity?copy_bytes:copy_capacity;
        if(copied!=copy_cursor||memcmp(copied,vector_items,i)||memcmp(copied+i,copy_before+i,copy_capacity+16-i)||
           permanent_next!=copy_cursor+copy_capacity||permanent_available!=copy_available-copy_capacity||
           memcmp(vector_fields,pch_saved_defines,12)||memcmp(vector_items,pch_saved_defines->items,copy_bytes))fail();
        sprintf(line,"metadata-copy-after returned-owner1 copied%u source-prefix-equal1 tail-guard-unchanged%u cursor-advance%u available-drop%u\n",i,copy_capacity+16-i,copy_capacity,copy_capacity);emit(line);
    } else if(selected_unit==5) {
        pascal_result=copy_pascal_symbol_name((char *)pascal_output,call_context->symbol);i=strlen(call_context->symbol->name);
        if((unsigned short)pascal_result!=(unsigned short)(short)(signed char)i||pascal_output[0]!=i||memcmp(pascal_output+1,call_context->symbol->name,i+1))fail();
        for(j=i+2;j<sizeof(pascal_output);++j)if(pascal_output[j]!=0xa5)fail();
        sprintf(line,"metadata-pascal-after ax%04x prefix-count%u text %s nul1 tail-a5%u\n",(unsigned short)pascal_result,i,call_context->symbol->name,254-i);emit(line);
    } else if(selected_unit==6||selected_unit==7) {
        qualifier_result=symbol_has_qualifiers(qualified_symbol);
        if(qualifier_result!=(selected_unit==6)||memcmp(qualifier_words,qualified_symbol->type,8))fail();
        sprintf(line,"metadata-qualifier-after eax%u type-unchanged8\n",qualifier_result);emit(line);
    } else if(selected_unit==8) {
        suppress_browser_output();browser_after(1);pending("browser-first");
        suppress_browser_output();browser_after(2);
    } else {
        format_result=format_initializer_text(owned_work,format,call_context->symbol->name,(int)pch_saved_defines->count);
        if(format_result!=15||owned_work->count!=15||owned_work->next!=work_storage+15||owned_work->read!=work_storage||owned_work->storage!=work_storage||
           memcmp(work_storage,"metadata_sink:6",15)||((WorkingBlock *)work_storage)->next||((WorkingBlock *)work_storage)->used_bytes||compiler_stack!=old_work)fail();
        sprintf(line,"metadata-format-result eax%u count%u next-written-prefix15 source metadata_sink:6 original-work-unchanged1\n",format_result,owned_work->count);emit(line);
        finish_initializer_buffer(owned_work);if(((WorkingBlock *)work_storage)->next||((WorkingBlock *)work_storage)->used_bytes!=15)fail();
        reset_work_stack(owned_work);if(owned_work->count||owned_work->next!=work_storage||owned_work->read!=work_storage)fail();
        read_result=read_work_stack(owned_work,work_output,15);
        if(read_result!=15||memcmp(work_output,"metadata_sink:6",15)||owned_work->count!=15||owned_work->next!=work_storage+15||
           owned_work->read!=work_storage||owned_work->storage!=work_storage||((WorkingBlock *)work_storage)->next||((WorkingBlock *)work_storage)->used_bytes!=15||compiler_stack!=old_work)fail();
        for(i=15;i<256;++i)if(work_output[i]!=0xa5)fail();
        sprintf(line,"metadata-format-after bytes15 read%u text metadata_sink:6 tail-a5241 original-work-unchanged1\n",read_result);emit(line);
    }
    __asm { mov after,esp }
    __asm { fnstcw control_after }
    if(before!=after||control_before!=control_after)fail();
    for(i=0;i<3;++i)if(memcmp(initial_nodes[i],nodes[i],20))fail();
    for(i=0;i<(unsigned)permanent_page_count;++i)if(memcmp(mark_snapshots[i],permanent_pages[i].relocations->bits,7680))fail();
    sprintf(line,"metadata-after nodes-unchanged3 page-marks-unchanged%u no-recursion\n",permanent_page_count);emit(line);
    cache("after");pending("after");syscall(SYS_exit_group,99);fail();
}
int __wrap_main(int argc,char **argv,char **environment)
{
    char *end,*original[2];
    if(argc!=3)return 89;
    selected_unit=strtoul(argv[1],&end,10);if(*end||selected_unit>9)return 89;
    report_descriptor=syscall(SYS_open,argv[2],577,0600);if(report_descriptor<0)return 89;
    original[0]=argv[0];original[1]=0;
    return __real_main(1,original,environment);
}
