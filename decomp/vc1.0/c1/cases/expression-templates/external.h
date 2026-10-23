/* External test protocol only. Never linked into the reconstructed application. */
#ifndef EXPRESSION_TEMPLATE_EXTERNAL_H
#define EXPRESSION_TEMPLATE_EXTERNAL_H
#include "../../src/expressions.h"
#include "../../src/tokens.h"
#include <string.h>
#include <recon.h>

typedef struct TemplateInput {
    ExpressionNode nodes[3];
    Extended extended;
    unsigned char text[8];
} TemplateInput;
typedef char TemplateInput_size[sizeof(TemplateInput)==80 ? 1 : -1];
typedef int (*TemplateRead)(void *, unsigned, void *, unsigned);
typedef void (*TemplateEmit)(void *, const char *, unsigned);
typedef struct TemplateObserver {
    void *context;
    TemplateRead read;
    TemplateEmit emit;
    unsigned owners[3];
    unsigned node_count;
    int failed;
} TemplateObserver;

static unsigned template_mode(unsigned unit) { return unit>=4 ? 2 : 6; }
static unsigned template_nodes(unsigned unit) { return unit==2 ? 2 : unit==3 ? 3 : 1; }
static void template_build_input(unsigned unit, unsigned base, unsigned *owners, TemplateInput *input)
{
    unsigned index;
    memset(input,0,sizeof(*input));
    for(index=0;index<3;++index) {
        input->nodes[index].kind=TOK_INT_CONSTANT;
        input->nodes[index].flags=3;
        input->nodes[index].type=(CType *)owners[0];
        input->nodes[index].value.words.low=17+index;
    }
    if(unit==2 || unit==3) {
        input->nodes[0].kind=unit==2 ? TOK_UNARY_MINUS : TOK_PLUS;
        input->nodes[0].flags=unit==2 ? 1 : 2;
        input->nodes[0].value.children.left=(IrToken *)(base+sizeof(ExpressionNode));
        input->nodes[0].value.children.right=unit==3 ? (IrToken *)(base+2*sizeof(ExpressionNode)) : 0;
        input->nodes[1].auxiliary=base;
        if(unit==3)input->nodes[2].auxiliary=base;
    } else if(unit==4) {
        IrToken *string=(IrToken *)input->nodes;
        string->kind=3; string->flags=5;
        string->type=(CType *)owners[2];
        string->value.text=(char *)(base+offsetof(TemplateInput,text));
        string->length=5;string->auxiliary_flags=0;
        memcpy(input->text,"iris",5);
    } else if(unit==5) {
        input->nodes[0].kind=TOK_DOUBLE_CONSTANT;
        input->nodes[0].type=(CType *)owners[1];
        input->nodes[0].value.words.low=base+offsetof(TemplateInput,extended);
        input->extended.low=0; input->extended.high=0xc0000000;
        input->extended.exponent=0x3fff; input->extended.format=8;
    }
}

static int template_read(TemplateObserver *observer, unsigned address, void *output, unsigned bytes)
{
    if(!address || !observer->read(observer->context,address,output,bytes)) {
        observer->failed=1;return 0;
    }
    return 1;
}
static void template_hex(TemplateObserver *observer, const char *label, unsigned address, const void *data, unsigned bytes)
{
    static char digits[]="0123456789abcdef";
    char line[4096];
    const unsigned char *source=data;
    unsigned index,length=sprintf(line,"expr-%s %08x %u ",label,address,bytes);
    if(bytes>1024) { observer->failed=1;return; }
    for(index=0;index<bytes;++index) {
        line[length++]=digits[source[index]>>4];line[length++]=digits[source[index]&15];
    }
    line[length++]='\n';observer->emit(observer->context,line,length);
}
static void template_memory(TemplateObserver *observer,const char *label,unsigned address,unsigned bytes)
{
    unsigned char data[1024];
    if(bytes>sizeof(data) || !template_read(observer,address,data,bytes))return;
    template_hex(observer,label,address,data,bytes);
}
static void template_owners(TemplateObserver *observer,const char *boundary)
{
    unsigned index;
    char label[100];
    CType type;
    for(index=0;index<3;++index) {
        sprintf(label,"type-%s-%u",boundary,index);
        template_memory(observer,label,observer->owners[index],sizeof(CType));
        if(!template_read(observer,observer->owners[index],&type,sizeof(type)))return;
        if(type.related) {
            sprintf(label,"shape-%s-%u",boundary,index);
            template_memory(observer,label,(unsigned)type.related,sizeof(TypeShape));
        }
    }
}
static void template_output(TemplateObserver *observer,unsigned address)
{
    ExpressionNode node;
    CType type;
    char label[100];
    unsigned index=observer->node_count++;
    if(index>=8 || !template_read(observer,address,&node,sizeof(node))) {observer->failed=1;return;}
    sprintf(label,"node-%u",index);
    template_hex(observer,label,address,&node,sizeof(node));
    switch(node.flags&7) {
    case 1:
        template_output(observer,(unsigned)node.value.children.left);break;
    case 2:
        if(node.value.children.left)template_output(observer,(unsigned)node.value.children.left);
        if(node.value.children.right)template_output(observer,(unsigned)node.value.children.right);
        break;
    case 3:
        if(!template_read(observer,(unsigned)node.type,&type,sizeof(type)))return;
        if((type.flags&0x100) && !type.related) {
            sprintf(label,"extended-%u",index);
            template_memory(observer,label,node.value.words.low,sizeof(Extended));
        }
        break;
    case 5:
        sprintf(label,"string-%u",index);
        template_memory(observer,label,node.value.words.low,(((IrToken *)&node)->length+3)&~3U);
        break;
    }
}
#endif
