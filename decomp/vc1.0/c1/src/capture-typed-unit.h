/* Test-only typed/declaration units over real parsed owners. Fixed accepted
   constructor calls reuse the existing bounded container return mechanism. */
#include <recon.h>
static DWORD typed_symbol,typed_callback,typed_sink_type,typed_callback_type;
static DWORD typed_shape,typed_tag,typed_member,typed_context;
static int typed_read(HANDLE process,DWORD address,void *value,DWORD bytes)
{
    DWORD got;
    return address&&ReadProcessMemory(process,(void *)address,value,bytes,&got)&&got==bytes;
}
static int typed_text(HANDLE process,HANDLE log,DWORD address,const char *label)
{
    char text[256],line[384];DWORD index,written;
    for(index=0;index<sizeof(text);++index) {
        if(!typed_read(process,address+index,text+index,1))return 0;
        if(!text[index])break;
    }
    if(index==sizeof(text))return 0;
    sprintf(line,"typed-name %s address %08lx text %s\n",label,address,text);
    return WriteFile(log,line,strlen(line),&written,0)&&written==strlen(line);
}
static int typed_state(HANDLE process,HANDLE log,DWORD base,const char *boundary)
{
    DWORD errors,counter,override,parameters,written;char line[256];
    if(!typed_read(process,base+0x4b228,&errors,4)||
       !typed_read(process,base+0x5ce70,&counter,4)||
       !typed_read(process,base+0x5d9f4,&override,4)||
       !typed_read(process,base+0x4ff14,&parameters,4))return 0;
    sprintf(line,"typed-state %s errors %lu counter %lu override %lu parameters %08lx\n",boundary,errors,counter,override,parameters);
    return WriteFile(log,line,strlen(line),&written,0)&&written==strlen(line);
}
static int typed_ir(HANDLE process,HANDLE log,DWORD address,const char *label,int binary)
{
    unsigned char kind,flags;unsigned short type_flags;
    DWORD type,value,right=0,written;char line[256];
    /* Skip all unwritten token fields. */
    if(!typed_read(process,address,&kind,1)||!typed_read(process,address+1,&flags,1)||
       !typed_read(process,address+4,&type,4)||!typed_read(process,type,&type_flags,2)||
       !typed_read(process,address+12,&value,4)||
       (binary&&!typed_read(process,address+16,&right,4)))return 0;
    sprintf(line,"typed-ir %s address %08lx kind %u flags %u type %08lx typeflags %04x value %08lx right %08lx\n",label,address,kind,flags,type,type_flags,value,right);
    return WriteFile(log,line,strlen(line),&written,0)&&written==strlen(line);
}
static int typed_aggregate(HANDLE process,HANDLE log,const char *boundary)
{
    DWORD fields[4],written,member_type;unsigned short bitfield,alignment,member_flags;
    unsigned char used,flags;char line[384];
    /* begin_tag_definition initializes these 22 bytes; skip reserved padding. */
    if(!typed_read(process,typed_context,fields,16)||
       !typed_read(process,typed_context+16,&bitfield,2)||
       !typed_read(process,typed_context+18,&alignment,2)||
       !typed_read(process,typed_context+20,&used,1)||
       !typed_read(process,typed_context+21,&flags,1))return 0;
    sprintf(line,"typed-aggregate %s context %08lx tag %08lx last %08lx type %08lx size %lu bitfield %u alignment %u used %u flags %u member %08lx\n",boundary,typed_context,fields[0],fields[1],fields[2],fields[3],bitfield,alignment,used,flags,typed_member);
    if(!WriteFile(log,line,strlen(line),&written,0)||written!=strlen(line)||
       !typed_read(process,typed_member+12,&member_type,4)||
       !typed_read(process,member_type,&member_flags,2))return 0;
    sprintf(line,"typed-member %s type %08lx flags %04x\n",boundary,member_type,member_flags);
    return WriteFile(log,line,strlen(line),&written,0)&&written==strlen(line);
}
static int typed_prepare(HANDLE process,HANDLE log,DWORD base)
{
    DWORD name,callback_name,stream,parameters,depth,written;
    char line[384];
    if(container_unit>=27&&container_unit<=29) {
        if(!typed_read(process,base+0x5bf94,&depth,4)||depth>=16)return 0;
        typed_context=base+0x500a8+depth*24;
        if(!typed_read(process,typed_context,&typed_tag,4)||!typed_tag||
           !typed_read(process,direct_saved.Esp+4,&typed_member,4)||!typed_member||
           !typed_read(process,typed_tag+8,&name,4)||
           !typed_text(process,log,name,"aggregate")||
           !typed_aggregate(process,log,"before"))return 0;
        if(container_unit==29&&!typed_read(process,typed_member+12,direct_arguments,4))return 0;

        return typed_state(process,log,base,"before");
    }
    typed_symbol=parsed_global_symbol(process,base,"container_result");
    typed_callback=parsed_global_symbol(process,base,"ContainerCallback");
    name=parsed_global_symbol(process,base,"container_sink");
    if(!typed_symbol||!typed_callback||!name||
       !typed_read(process,typed_symbol+12,&container_type,4)||
       !typed_read(process,typed_callback+12,&typed_callback_type,4)||
       !typed_read(process,name+12,&typed_sink_type,4)||
       !typed_read(process,typed_callback_type+4,&typed_shape,4)||!typed_shape||
       !typed_state(process,log,base,"before"))return 0;
    sprintf(line,"typed-owners symbol %08lx type %08lx callback %08lx callbacktype %08lx shape %08lx sinktype %08lx\n",typed_symbol,container_type,typed_callback,typed_callback_type,typed_shape,typed_sink_type);
    WriteFile(log,line,strlen(line),&written,0);
    if(container_unit==17)direct_arguments[0]=37;
    else if(container_unit==18||container_unit==22||container_unit==23) {
        container_stage=1;return 1;
    } else if(container_unit==19||container_unit==21) {
        direct_arguments[0]=typed_shape;
        if(!container_type_state(process,log,base,typed_callback_type,"before"))return 0;
    } else if(container_unit==20)direct_arguments[0]=container_type;
    else if(container_unit==24||container_unit==25) {
        if(!typed_read(process,base+0x4ff14,&parameters,4)||parameters||
           !typed_read(process,typed_symbol+8,&name,4)||
           !typed_read(process,typed_callback+8,&callback_name,4)||
           !typed_text(process,log,name,"function")||
           !typed_text(process,log,callback_name,"callback"))return 0;
        if(container_unit==24) {
            if(!typed_read(process,base+0x4b598,&stream,4)||!stream)return 0;
            direct_arguments[0]=typed_symbol;direct_arguments[1]=stream;
            sprintf(line,"typed-output stream %08lx\n",stream);WriteFile(log,line,strlen(line),&written,0);
        } else {
            direct_arguments[0]=248;direct_arguments[1]=typed_symbol;direct_arguments[2]=0;
            direct_arguments[3]=name;direct_arguments[4]=callback_name;
            direct_arguments[5]=typed_sink_type;direct_arguments[6]=typed_callback_type;
        }
    } else if(container_unit==30)direct_arguments[0]=typed_symbol;
    return 1;
}
static int typed_constructed(HANDLE process,HANDLE thread,HANDLE log,DWORD base,CONTEXT *current,char *buffers)
{
    DWORD value,type;unsigned short flags;unsigned char kind,node_flags;
    unsigned expected=container_unit==18&&container_stage==1?37:container_unit==22?37:11;
    unsigned expected_type=expected==37?0x64:0x4064;
    if(!typed_read(process,current->Eax,&kind,1)||kind!=7||
       !typed_read(process,current->Eax+1,&node_flags,1)||node_flags!=3||
       !typed_read(process,current->Eax+4,&type,4)||
       !typed_read(process,type,&flags,2)||flags!=expected_type||
       !typed_read(process,current->Eax+12,&value,4)||value!=expected||
       !typed_ir(process,log,current->Eax,container_stage==1?"constructor1":"constructor2",0))return -1;
    if(container_unit==18&&container_stage==1) {
        container_stage=2;return container_call(process,thread,log,base,current,1);
    }
    if(container_unit==18) {
        direct_arguments[0]=80;direct_arguments[1]=container_owned[0];direct_arguments[2]=container_owned[1];
    } else {
        direct_arguments[0]=typed_sink_type;direct_arguments[1]=container_owned[0];
    }
    container_stage=0;
    if(buffers)dump_stream_buffers(process,log,base,buffers);
    return container_call(process,thread,log,base,current,0);
}
static int typed_return(HANDLE process,HANDLE log,DWORD base,DWORD result)
{
    DWORD name;
    if(container_unit==17||container_unit==18) {
        if(!typed_ir(process,log,result,"result",container_unit==18))return 0;
    }
    if(container_unit==19||container_unit==21) {
        if(!container_type_state(process,log,base,typed_callback_type,"after"))return 0;
    }
    if(container_unit>=27&&container_unit<=29) {
        if(!typed_aggregate(process,log,"after"))return 0;
        if(container_unit==28) {
            if(!typed_read(process,typed_tag+8,&name,4)||name!=result||
               !typed_text(process,log,result,"result"))return 0;
        }
    }
    return typed_state(process,log,base,"after")&&container_vector_state(process,log,container_vector,"after");
}
