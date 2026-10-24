/* Fixed PCH metadata context; test transport, never application code. */
#ifndef CAPTURE_METADATA_UNIT_H
#define CAPTURE_METADATA_UNIT_H
#include <recon.h>
static DWORD metadata_nodes[3],metadata_node_words[3][5];
static DWORD metadata_pages[511][2],metadata_page_count;
static DWORD metadata_blocks[32],metadata_used[32],metadata_block_count;
static DWORD metadata_call,metadata_symbol,metadata_defines,metadata_directories;
static int metadata_unit=-1;
static DWORD metadata_teb;
static unsigned char *metadata_mark_snapshots[511];
static DWORD metadata_vector_fields[3],metadata_vector_items[16];
static int metadata_stage;
static unsigned metadata_index,metadata_setup_count;
static DWORD metadata_owned_vector,metadata_encoded_fields[3],metadata_encoded_items[16];
static DWORD metadata_current_reference;
static char *metadata_streams;
static DWORD metadata_copy_next,metadata_copy_available,metadata_copy_capacity,metadata_copy_bytes;
static unsigned char metadata_copy_before[64];
static DWORD metadata_name_buffer,metadata_name_length,metadata_qualifier_symbol,metadata_qualifier_type,metadata_qualifier_words[2];
static char metadata_name[260];
static DWORD metadata_browser_stream,metadata_browser_words[4],metadata_browser_template[6],metadata_browser_bytes;
static unsigned char metadata_browser_prefix[4096];
static unsigned metadata_browser_calls;
static DWORD metadata_work_slot,metadata_work_owner,metadata_work_storage,metadata_original_work,metadata_format_name,metadata_format_bytes;
static int metadata_read(HANDLE process,DWORD address,void *value,DWORD bytes)
{
    DWORD got;
    return ReadProcessMemory(process,(void *)address,value,bytes,&got)&&got==bytes;
}
static int metadata_text(HANDLE process,HANDLE log,const char *label,DWORD address)
{
    char text[260],line[340];unsigned i;DWORD written;
    for(i=0;i<sizeof(text);++i) {
        if(!metadata_read(process,address+i,text+i,1))return 0;
        if(!text[i])break;
    }
    if(i==sizeof(text))return 0;
    sprintf(line,"metadata-string %s %08lx %s\n",label,address,text);
    return WriteFile(log,line,strlen(line),&written,0)&&written==strlen(line);
}
static int metadata_permanent(DWORD address,DWORD bytes)
{
    unsigned i;
    for(i=0;i<metadata_page_count;++i)
        if(address>=metadata_pages[i][0]&&address-metadata_pages[i][0]<=0xeffc&&
           bytes<=0xeffc-(address-metadata_pages[i][0]))return 1;
    return 0;
}
static int metadata_temporary(DWORD address,DWORD bytes)
{
    unsigned i;int owned=0;
    if(!address||address>0xffffffffUL-bytes)return 0;
    for(i=0;i<metadata_block_count;++i)
        if(address>=metadata_blocks[i]+8&&address-metadata_blocks[i]-8<=metadata_used[i]&&
           bytes<=metadata_used[i]-(address-metadata_blocks[i]-8))owned=1;
    if(!owned)return 0;
    for(i=0;i<metadata_page_count;++i)
        if(address<metadata_pages[i][0]+0xeffc&&address+bytes>metadata_pages[i][0])return 0;
    return 1;
}
static int metadata_vector(HANDLE process,HANDLE log,DWORD pointer,const char *label)
{
    DWORD words[3],item,written;short count,capacity;unsigned i;char line[180],name[80];
    if(!metadata_permanent(pointer,12)||!metadata_read(process,pointer,words,12))return 0;
    count=(short)words[0];capacity=(short)(words[0]>>16);
    if(count<3||count>16||capacity<count||capacity>256||words[1]||
       !metadata_permanent(words[2],count*4))return 0;
    sprintf(line,"metadata-vector %s %08lx count %d capacity %d arena %lu items %08lx\n",label,pointer,count,capacity,words[1],words[2]);
    WriteFile(log,line,strlen(line),&written,0);
    for(i=0;i<(unsigned)count;++i) {
        if(!metadata_read(process,words[2]+4*i,&item,4)||!metadata_permanent(item,1))return 0;
        sprintf(name,"%s-%u",label,i);if(!metadata_text(process,log,name,item))return 0;
    }
    return 1;
}
static int metadata_context(HANDLE process,HANDLE thread,HANDLE log,DWORD base,DWORD teb)
{
    CONTEXT state;DWORD frame[2],context,context_words[8],call[5],pool[5],arena_bytes;
    DWORD page_count,block,block_words[2],bit_count,words[5],node,written,stack_limits[2];
    DWORD browser,stream[4],template_words[6];
    unsigned i,j;char line[360];
    memset(&state,0,sizeof(state));state.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
    if(!GetThreadContext(thread,&state)||
       !metadata_read(process,teb+4,stack_limits,8)||state.Esp<stack_limits[1]||state.Esp+8>stack_limits[0]||
       !metadata_read(process,state.Esp,frame,8)||
       !metadata_read(process,base+0x5b5d4,&context,4)||
       !metadata_read(process,context,context_words,32)||context_words[6]!=3||
       !metadata_text(process,log,"callee",context_words[3])||
       !metadata_read(process,frame[1],call,20)||((call[0]>>8)&255)!=2||(call[0]&255)!=114||!call[4])return 0;
    metadata_call=frame[1];metadata_symbol=context_words[2];
    if(parsed_global_symbol(process,base,"metadata_sink")!=metadata_symbol)return 0;
    if(!metadata_read(process,base+0x5de00,&page_count,4)||page_count<1||page_count>511||
       !metadata_read(process,base+0x56370,metadata_pages,page_count*8))return 0;
    metadata_page_count=page_count;
    sprintf(line,"metadata-context marker %08lx esp %08lx call %08lx context %08lx symbol %08lx arguments3 pages %lu\n",base+0x1c220,state.Esp,metadata_call,context,metadata_symbol,page_count);WriteFile(log,line,strlen(line),&written,0);
    for(i=0;i<page_count;++i) {
        if(!metadata_pages[i][0]||(metadata_pages[i][0]&4095)||metadata_pages[i][0]>0xffffffffUL-0xeffc||
           !metadata_pages[i][1]||!metadata_read(process,metadata_pages[i][1],&bit_count,4)||bit_count!=0xeffc)return 0;
        for(j=0;j<i;++j)if(metadata_pages[i][0]<metadata_pages[j][0]+0xeffc&&metadata_pages[i][0]+0xeffc>metadata_pages[j][0])return 0;
        sprintf(line,"metadata-page %u start %08lx bytes0000effc marks %08lx bits %lu\n",i,metadata_pages[i][0],metadata_pages[i][1],bit_count);WriteFile(log,line,strlen(line),&written,0);
    }
    if(!metadata_read(process,base+0x5e1c8,&arena_bytes,4)||arena_bytes!=0x3ff4||
       !metadata_read(process,base+0x5e1f8,pool,20)||!pool[0]||!pool[1]||pool[3]>arena_bytes)return 0;
    metadata_block_count=0;block=pool[0];
    while(block) {
        if(metadata_block_count>=32||block>0xffffffffUL-arena_bytes-12||
           !metadata_read(process,block,block_words,8)||block_words[1]>arena_bytes)return 0;
        for(i=0;i<metadata_block_count;++i)if(metadata_blocks[i]==block)return 0;
        metadata_blocks[metadata_block_count]=block;
        metadata_used[metadata_block_count]=block==pool[1]?pool[2]-block-8:block_words[1];
        if(metadata_used[metadata_block_count]>arena_bytes||
           (block==pool[1]&&(block_words[0]||metadata_used[metadata_block_count]+pool[3]!=arena_bytes)))return 0;
        sprintf(line,"metadata-arena-block %lu address %08lx allocation %lu data %08lx used %lu next %08lx last %u\n",metadata_block_count,block,arena_bytes+12,block+8,metadata_used[metadata_block_count],block_words[0],block==pool[1]);WriteFile(log,line,strlen(line),&written,0);
        ++metadata_block_count;block=block_words[0];
    }
    if(!metadata_block_count||metadata_blocks[metadata_block_count-1]!=pool[1]||
       !metadata_temporary(metadata_call,20)||!metadata_temporary(context,36))return 0;
    node=call[4];
    for(i=0;i<3;++i) {
        if(!metadata_temporary(node,20)||!metadata_read(process,node,words,20)||
           (words[0]&65535)!=0x025f||!words[1]||!metadata_permanent(words[1],8)||!words[3])return 0;
        metadata_nodes[i]=node;memcpy(metadata_node_words[i],words,20);
        sprintf(line,"metadata-argument %u address %08lx type %08lx next %08lx expression %08lx previous %08lx owned20 outside-all-pages\n",i,node,words[1],words[2],words[3],words[4]);WriteFile(log,line,strlen(line),&written,0);node=words[4];
    }
    if(node)return 0;
    for(i=0;i<3;++i)if(metadata_node_words[i][2]!=metadata_nodes[(i+2)%3])return 0;
    if(!metadata_read(process,base+0x5b664,&metadata_defines,4)||
       !metadata_read(process,base+0x5b668,&metadata_directories,4)||
       !metadata_vector(process,log,metadata_defines,"defines")||
       !metadata_vector(process,log,metadata_directories,"directories")||
       !metadata_read(process,base+0x4d5a0,&browser,4)||!browser||
       !metadata_read(process,browser,stream,16)||!stream[2]||stream[0]<stream[2]||stream[0]-stream[2]>4096||!stream[3]||
       !metadata_read(process,base+0x5387c,template_words,24))return 0;
    sprintf(line,"metadata-browser stream %08lx pending %lu flags %08lx template",browser,stream[0]-stream[2],stream[3]);WriteFile(log,line,strlen(line),&written,0);
    for(i=0;i<6;++i){sprintf(line," %08lx",template_words[i]);WriteFile(log,line,strlen(line),&written,0);}WriteFile(log,"\n",1,&written,0);
    return 1;
}
static int metadata_cache(HANDLE process,HANDLE log,DWORD base,const char *when)
{
    DWORD reference,cache_base,written;unsigned short page;unsigned i,j;char line[180];
    if(!metadata_read(process,base+0x5ddbc,&reference,4)||
       !metadata_read(process,base+0x5ddc0,&cache_base,4)||
       !metadata_read(process,base+0x5ddc4,&page,2)||
       (cache_base&&(page>=metadata_page_count||cache_base!=metadata_pages[page][0])))return 0;
    /* A NULL cache forces a fresh lookup for every owned arena slot. The
       inactive page word is overwritten before mark_pch_pointer uses it. */
    if(!strcmp(when,"before"))for(i=0;i<3;++i)for(j=1;j<5;++j)
        if(metadata_node_words[i][j]&&(DWORD)(metadata_nodes[i]+4*j-cache_base)<=0xeffc)return 0;
    sprintf(line,"metadata-cache %s reference %08lx base %08lx page %u\n",when,reference,cache_base,page);
    if(!WriteFile(log,line,strlen(line),&written,0)||written!=strlen(line))return 0;
    if(!strcmp(when,"before")) {
        strcpy(line,"metadata-cache-path before lookup-for-all-nonnull-slots\n");
        if(!WriteFile(log,line,strlen(line),&written,0)||written!=strlen(line))return 0;
    }
    return 1;
}
static int metadata_prepare(HANDLE process,HANDLE thread,HANDLE log,DWORD base,DWORD teb)
{
    unsigned i;
    if(metadata_unit<0||metadata_unit>9||!metadata_context(process,thread,log,base,teb)||
       !metadata_cache(process,log,base,"before"))return 0;
    for(i=0;i<metadata_page_count;++i) {
        metadata_mark_snapshots[i]=malloc(7680);
        if(!metadata_mark_snapshots[i]||!metadata_read(process,metadata_pages[i][1]+4,metadata_mark_snapshots[i],7680))return 0;
    }
    direct_arguments[0]=metadata_nodes[0];
    if(metadata_unit>=1) {
        if(!metadata_read(process,metadata_defines,metadata_vector_fields,12)||
           !metadata_read(process,metadata_vector_fields[2],metadata_vector_items,4*(metadata_vector_fields[0]&65535)))return 0;
        direct_arguments[0]=metadata_defines;
        if(metadata_unit==2){metadata_stage=1;metadata_index=0;}
        if(metadata_unit==3||metadata_unit==4) {
            char line[220];DWORD written;
            metadata_copy_bytes=4*(metadata_vector_fields[0]&65535);
            metadata_copy_capacity=metadata_unit==3?metadata_copy_bytes+8:10;
            if(metadata_copy_capacity+16>sizeof(metadata_copy_before)||
               !metadata_read(process,base+0x5b650,&metadata_copy_next,4)||
               !metadata_read(process,base+0x5b7dc,&metadata_copy_available,4)||metadata_copy_available<metadata_copy_capacity+16||
               !metadata_permanent(metadata_copy_next,metadata_copy_capacity+16)||
               (metadata_copy_next<metadata_vector_fields[2]+metadata_copy_bytes&&metadata_copy_next+metadata_copy_capacity>metadata_vector_fields[2])||
               !metadata_read(process,metadata_copy_next,metadata_copy_before,metadata_copy_capacity+16))return 0;
            direct_arguments[0]=metadata_vector_fields[2];direct_arguments[1]=metadata_copy_bytes;direct_arguments[2]=metadata_copy_capacity;
            sprintf(line,"metadata-copy-input cursor %08lx available %lu source %08lx bytes%lu allocation%lu\n",metadata_copy_next,metadata_copy_available,direct_arguments[0],metadata_copy_bytes,metadata_copy_capacity);WriteFile(log,line,strlen(line),&written,0);
        }
    }
    if(metadata_unit==5) {
        DWORD name,written;unsigned char output[256];char line[220];
        if(!metadata_read(process,metadata_symbol+8,&name,4))return 0;
        for(metadata_name_length=0;metadata_name_length<128;++metadata_name_length) {
            if(!metadata_permanent(name+metadata_name_length,1)||!metadata_read(process,name+metadata_name_length,metadata_name+metadata_name_length,1))return 0;
            if(!metadata_name[metadata_name_length])break;
        }
        if(metadata_name_length>=128||strcmp(metadata_name,"metadata_sink"))return 0;
        metadata_name_buffer=direct_stack-4096;memset(output,0xa5,sizeof(output));
        if(!WriteProcessMemory(process,(void *)metadata_name_buffer,output,sizeof(output),&written)||written!=sizeof(output))return 0;
        direct_arguments[0]=metadata_name_buffer;direct_arguments[1]=metadata_symbol;
        sprintf(line,"metadata-pascal-input buffer %08lx allocation256 symbol %08lx name %08lx length%lu text %s\n",metadata_name_buffer,metadata_symbol,name,metadata_name_length,metadata_name);WriteFile(log,line,strlen(line),&written,0);
    }
    if(metadata_unit==6||metadata_unit==7) {
        const char *name=metadata_unit==6?"metadata_qualified":"metadata_mutable";DWORD written;char line[240];
        metadata_qualifier_symbol=parsed_global_symbol(process,base,name);
        if(!metadata_permanent(metadata_qualifier_symbol,16)||!metadata_read(process,metadata_qualifier_symbol+12,&metadata_qualifier_type,4)||
           !metadata_permanent(metadata_qualifier_type,8)||!metadata_read(process,metadata_qualifier_type,metadata_qualifier_words,8)||metadata_qualifier_words[1]||
           !!(metadata_qualifier_words[0]&0x30000000)!=(metadata_unit==6))return 0;
        direct_arguments[0]=metadata_qualifier_symbol;
        sprintf(line,"metadata-qualifier-input symbol %08lx name %s type %08lx flags%04x attributes%04x related%08lx\n",metadata_qualifier_symbol,name,metadata_qualifier_type,(unsigned short)metadata_qualifier_words[0],(unsigned short)(metadata_qualifier_words[0]>>16),metadata_qualifier_words[1]);WriteFile(log,line,strlen(line),&written,0);
    }
    if(metadata_unit==8) {
        DWORD suppressed,written;unsigned char hook;char line[260];
        if(!metadata_read(process,base+0x5b58c,&suppressed,4)||suppressed||
           !metadata_read(process,base+0x4d5a0,&metadata_browser_stream,4)||!metadata_browser_stream||
           !metadata_read(process,metadata_browser_stream,metadata_browser_words,16)||(long)metadata_browser_words[1]<2||
           !(metadata_browser_words[3]&2)||metadata_browser_words[0]<metadata_browser_words[2]||
           !metadata_read(process,base+0x5387c,metadata_browser_template,24)||
           !metadata_read(process,base+0x17190,&hook,1)||hook!=0xc3)return 0;
        metadata_browser_bytes=metadata_browser_words[0]-metadata_browser_words[2];
        if(metadata_browser_bytes>4094||!metadata_read(process,metadata_browser_words[2],metadata_browser_prefix,metadata_browser_bytes))return 0;
        for(i=0;i<6;++i)if(metadata_browser_template[i])return 0;
        metadata_browser_calls=0;
        sprintf(line,"metadata-browser-input stream %08lx pointer %08lx base %08lx count %lu flags %08lx pending%lu suppressed0 template-zero24 hook-known-empty1\n",metadata_browser_stream,metadata_browser_words[0],metadata_browser_words[2],metadata_browser_words[1],metadata_browser_words[3],metadata_browser_bytes);WriteFile(log,line,strlen(line),&written,0);
    }
    if(metadata_unit==9) {
        DWORD zero=0,written;unsigned char output[256];char line[240];
        metadata_work_slot=direct_stack-4096;metadata_format_bytes=15;
        if(!metadata_read(process,metadata_symbol+8,&metadata_format_name,4)||
           !metadata_text(process,log,"format-name",metadata_format_name)||
           !metadata_read(process,base+0x4f4f4,&metadata_original_work,4)||!metadata_original_work)return 0;
        memset(output,0xa5,sizeof(output));
        if(!WriteProcessMemory(process,(void *)metadata_work_slot,&zero,4,&written)||written!=4||
           !WriteProcessMemory(process,(void *)(metadata_work_slot+16),"%s:%d",6,&written)||written!=6||
           !WriteProcessMemory(process,(void *)(metadata_work_slot+64),output,sizeof(output),&written)||written!=sizeof(output))return 0;
        sprintf(line,"metadata-format-setup slot %08lx initial-null1 format %08lx output %08lx allocation256 original-work %08lx\n",metadata_work_slot,metadata_work_slot+16,metadata_work_slot+64,metadata_original_work);WriteFile(log,line,strlen(line),&written,0);
        metadata_stage=4;
    }
    return 1;
}
static int metadata_return(HANDLE process,HANDLE log,DWORD base,DWORD result)
{
    DWORD words[5],written;unsigned i;unsigned char marks[7680];char line[360];
    if(metadata_unit<0||metadata_unit>9)return 0;
    for(i=0;i<3;++i)if(!metadata_read(process,metadata_nodes[i],words,20)||
       memcmp(words,metadata_node_words[i],20))return 0;
    for(i=0;i<metadata_page_count;++i) {
        if(!metadata_read(process,metadata_pages[i][1]+4,marks,7680)||
           memcmp(marks,metadata_mark_snapshots[i],7680))return 0;
        if(metadata_unit!=8||metadata_browser_calls){free(metadata_mark_snapshots[i]);metadata_mark_snapshots[i]=0;}
    }
    sprintf(line,"metadata-after nodes-unchanged3 page-marks-unchanged%lu no-recursion\n",metadata_page_count);
    WriteFile(log,line,strlen(line),&written,0);
    if(metadata_unit==1||metadata_unit==2) {
        DWORD fields[3],items[16],count;
        count=metadata_vector_fields[0]&65535;
        if(result!=(metadata_unit==1?metadata_defines:metadata_owned_vector)||!metadata_read(process,metadata_defines,fields,12)||memcmp(fields,metadata_vector_fields,12)||
           !metadata_read(process,fields[2],items,4*count)||memcmp(items,metadata_vector_items,4*count))return 0;
        if(metadata_unit==2&&(!metadata_read(process,result,fields,12)||memcmp(fields,metadata_encoded_fields,12)||
           !metadata_read(process,fields[2],items,4*count)||memcmp(items,metadata_encoded_items,4*count)))return 0;
        sprintf(line,"metadata-vector-after unit%d owner %08lx items-unchanged%lu returned-owner1\n",metadata_unit,result,count);WriteFile(log,line,strlen(line),&written,0);
    }
    if(metadata_unit==3||metadata_unit==4) {
        unsigned char after[64];DWORD cursor,available,page_count,copied,items[16],fields[3];
        copied=metadata_copy_bytes<metadata_copy_capacity?metadata_copy_bytes:metadata_copy_capacity;
        if(result!=metadata_copy_next||!metadata_read(process,result,after,metadata_copy_capacity+16)||
           memcmp(after,metadata_vector_items,copied)||memcmp(after+copied,metadata_copy_before+copied,metadata_copy_capacity+16-copied)||
           !metadata_read(process,base+0x5b650,&cursor,4)||cursor!=metadata_copy_next+metadata_copy_capacity||
           !metadata_read(process,base+0x5b7dc,&available,4)||available!=metadata_copy_available-metadata_copy_capacity||
           !metadata_read(process,base+0x5de00,&page_count,4)||page_count!=metadata_page_count||
           !metadata_read(process,metadata_defines,fields,12)||memcmp(fields,metadata_vector_fields,12)||
           !metadata_read(process,fields[2],items,metadata_copy_bytes)||memcmp(items,metadata_vector_items,metadata_copy_bytes))return 0;
        sprintf(line,"metadata-copy-after returned-owner1 copied%lu source-prefix-equal1 tail-guard-unchanged%lu cursor-advance%lu available-drop%lu\n",copied,metadata_copy_capacity+16-copied,metadata_copy_capacity,metadata_copy_capacity);WriteFile(log,line,strlen(line),&written,0);
    }
    if(metadata_unit==5) {
        unsigned char output[256];
        if((result&65535)!=(unsigned short)(short)(signed char)metadata_name_length||
           !metadata_read(process,metadata_name_buffer,output,sizeof(output))||output[0]!=metadata_name_length||
           memcmp(output+1,metadata_name,metadata_name_length+1))return 0;
        for(i=metadata_name_length+2;i<sizeof(output);++i)if(output[i]!=0xa5)return 0;
        sprintf(line,"metadata-pascal-after ax%04x prefix-count%lu text %s nul1 tail-a5%lu\n",(unsigned)(result&65535),metadata_name_length,metadata_name,254-metadata_name_length);WriteFile(log,line,strlen(line),&written,0);
    }
    if(metadata_unit==6||metadata_unit==7) {
        DWORD words[2];
        if(result!=(DWORD)(metadata_unit==6)||!metadata_read(process,metadata_qualifier_type,words,8)||memcmp(words,metadata_qualifier_words,8))return 0;
        sprintf(line,"metadata-qualifier-after eax%lu type-unchanged8\n",result);WriteFile(log,line,strlen(line),&written,0);
    }
    if(metadata_unit==8) {
        DWORD fields[4],template_words[6],suppressed,auxiliary;unsigned char prefix[4096];
        if(metadata_browser_calls>=2||!metadata_read(process,metadata_browser_stream,fields,16)||
           fields[0]!=metadata_browser_words[0]+1||fields[1]!=metadata_browser_words[1]-1||
           fields[2]!=metadata_browser_words[2]||fields[3]!=metadata_browser_words[3]||
           !metadata_read(process,fields[2],prefix,metadata_browser_bytes+1)||memcmp(prefix,metadata_browser_prefix,metadata_browser_bytes)||prefix[metadata_browser_bytes]!=14||
           !metadata_read(process,base+0x5387c,template_words,24)||memcmp(template_words,metadata_browser_template,24)||
           !metadata_read(process,base+0x5b58c,&suppressed,4)||suppressed!=1||
           !metadata_read(process,base+0x5e434,&auxiliary,4)||auxiliary!=metadata_browser_stream)return 0;
        ++metadata_browser_calls;
        sprintf(line,"metadata-browser-after call%u suppressed1 appended%s pending%lu template-unchanged24 aux-browser1\n",metadata_browser_calls,metadata_browser_calls==1?"0e":"none",metadata_browser_bytes+1);WriteFile(log,line,strlen(line),&written,0);
    }
    if(metadata_unit==9) {
        DWORD fields[4],trailer[2],old_work;char text[16];
        if(result!=metadata_format_bytes||!metadata_read(process,metadata_work_owner,fields,16)||
           fields[0]!=metadata_format_bytes||fields[1]!=metadata_work_storage+metadata_format_bytes||fields[2]!=metadata_work_storage||fields[3]!=metadata_work_storage||
           !metadata_read(process,metadata_work_storage+2040,trailer,8)||trailer[0]||trailer[1]||
           !metadata_read(process,metadata_work_storage,text,metadata_format_bytes)||memcmp(text,"metadata_sink:6",metadata_format_bytes)||
           !metadata_read(process,base+0x4f4f4,&old_work,4)||old_work!=metadata_original_work)return 0;
        sprintf(line,"metadata-format-result eax%lu count%lu next-written-prefix%lu source metadata_sink:6 original-work-unchanged1\n",result,fields[0],metadata_format_bytes);WriteFile(log,line,strlen(line),&written,0);
        metadata_stage=5;
    }
    return metadata_cache(process,log,base,"after");
}
/* Fixed accepted vector operations and the WorkingStack constructor/read
   workflow only. No target compiler globals, token state or pointer-valued
   source slots are changed by this test transport. */
static int metadata_launch(HANDLE process,HANDLE thread,HANDLE log,DWORD base,CONTEXT *current)
{
    DWORD frame[12],target,written;unsigned count,i;char line[220];
    for(i=0;i<12;++i)frame[i]=0xa5a5a5a5;
    frame[0]=direct_marker;
    if(metadata_stage==1) {target=base+0x224c0;count=2;frame[1]=metadata_vector_fields[0]&65535;frame[2]=0;}
    else if(metadata_stage==2) {target=base+0x345f0;count=1;frame[1]=metadata_vector_items[metadata_index];}
    else if(metadata_stage==3) {target=base+0x22780;count=2;frame[1]=metadata_owned_vector;frame[2]=metadata_current_reference;}
    else if(metadata_stage==4) {target=base+0x19cf0;count=1;frame[1]=metadata_work_slot;}
    else if(metadata_stage==5) {target=base+0x19eb0;count=1;frame[1]=metadata_work_owner;}
    else if(metadata_stage==6) {target=base+0x19cd0;count=1;frame[1]=metadata_work_owner;}
    else if(metadata_stage==7) {target=base+0x19e00;count=3;frame[1]=metadata_work_owner;frame[2]=metadata_work_slot+64;frame[3]=metadata_format_bytes;}
    else if(metadata_stage==0) {
        target=base+direct_target_rva;count=metadata_unit==8?0:metadata_unit==9?4:1;
        if(metadata_unit==9)for(i=0;i<4;++i)frame[i+1]=direct_arguments[i];
        else if(count)frame[1]=metadata_owned_vector;
    }
    else return -1;
    metadata_setup_count=count;
    sprintf(line,"metadata-setup-start stage%d index%u target %08lx stack %08lx args%u %08lx %08lx\n",metadata_stage,metadata_index,target,direct_stack,count,count?frame[1]:0,count>=2?frame[2]:0);WriteFile(log,line,strlen(line),&written,0);
    for(i=2;i<count;++i){sprintf(line,"metadata-setup-argument %u %08lx\n",i+1,frame[i+1]);WriteFile(log,line,strlen(line),&written,0);}
    if(!metadata_stage&&metadata_streams)dump_stream_buffers(process,log,base,metadata_streams);
    if(!WriteProcessMemory(process,(void *)direct_stack,frame,sizeof(frame),&written)||written!=sizeof(frame))return -1;
    current->Eip=target;current->Esp=direct_stack;direct_active=1;
    return SetThreadContext(thread,current)?1:-1;
}
static int metadata_constructed(HANDLE process,HANDLE thread,HANDLE log,DWORD base,CONTEXT *current)
{
    DWORD fields[3],items[16],canary[4],page_count,written;unsigned count,i,page;char line[240];
    count=metadata_vector_fields[0]&65535;
    if(current->Esp!=direct_stack+4+4*metadata_setup_count||current->Ebx!=direct_saved.Ebx||current->Esi!=direct_saved.Esi||
       current->Edi!=direct_saved.Edi||current->Ebp!=direct_saved.Ebp||current->FloatSave.ControlWord!=direct_saved.FloatSave.ControlWord||
       !metadata_read(process,current->Esp,canary,16)||!metadata_read(process,base+0x5de00,&page_count,4)||page_count!=metadata_page_count)return -1;
    for(i=0;i<4;++i)if(canary[i]!=0xa5a5a5a5)return -1;
    sprintf(line,"metadata-setup-return stage%d index%u eax %08lx esp %08lx nonvolatile1 canary-a5x16\n",metadata_stage,metadata_index,current->Eax,current->Esp);WriteFile(log,line,strlen(line),&written,0);
    if(metadata_stage==1) {
        metadata_owned_vector=current->Eax;
        if(!metadata_permanent(metadata_owned_vector,12)||!metadata_read(process,metadata_owned_vector,fields,12)||
           fields[0]!=(count<<16)||fields[1]||!metadata_permanent(fields[2],count*4))return -1;
        metadata_stage=2;
    } else if(metadata_stage==2) {
        if(!metadata_read(process,base+0x5ddbc,&metadata_current_reference,4))return -1;
        page=metadata_current_reference>>16;
        if(page>=metadata_page_count||(metadata_current_reference&65535)>=0xeffc||
           metadata_pages[page][0]+(metadata_current_reference&65535)!=metadata_vector_items[metadata_index])return -1;
        metadata_encoded_items[metadata_index]=metadata_current_reference;
        sprintf(line,"metadata-reference %u original %08lx encoded %08lx\n",metadata_index,metadata_vector_items[metadata_index],metadata_current_reference);WriteFile(log,line,strlen(line),&written,0);metadata_stage=3;
    } else if(metadata_stage==3) {
        if(!metadata_read(process,metadata_owned_vector,fields,12)||
           fields[0]!=((count<<16)|(metadata_index+1))||fields[1]||!metadata_permanent(fields[2],count*4)||
           !metadata_read(process,fields[2],items,4*(metadata_index+1))||memcmp(items,metadata_encoded_items,4*(metadata_index+1)))return -1;
        if(++metadata_index<count)metadata_stage=2;
        else {
            memcpy(metadata_encoded_fields,fields,12);metadata_stage=0;
            sprintf(line,"metadata-encoded-vector before owner %08lx count%u capacity%u arena0 items %08lx\n",metadata_owned_vector,count,count,fields[2]);WriteFile(log,line,strlen(line),&written,0);
            if(!metadata_cache(process,log,base,"prepared"))return -1;
        }
    } else if(metadata_stage==4) {
        DWORD work[4],trailer[2],old_work;
        if(!metadata_read(process,metadata_work_slot,&metadata_work_owner,4)||!metadata_work_owner||metadata_work_owner==metadata_original_work||
           !metadata_read(process,metadata_work_owner,work,16)||work[0]||!work[3]||work[1]!=work[3]||work[2]!=work[3]||
           !metadata_read(process,work[3]+2040,trailer,8)||trailer[0]||trailer[1]||
           !metadata_read(process,base+0x4f4f4,&old_work,4)||old_work!=metadata_original_work)return -1;
        metadata_work_storage=work[3];
        direct_arguments[0]=metadata_work_owner;direct_arguments[1]=metadata_work_slot+16;direct_arguments[2]=metadata_format_name;direct_arguments[3]=count;
        sprintf(line,"metadata-format-input buffer %08lx owner16 storage %08lx block2048 format %08lx name %08lx number%u count0 trailer-zero8 original-work-distinct1\n",metadata_work_owner,metadata_work_storage,direct_arguments[1],metadata_format_name,count);WriteFile(log,line,strlen(line),&written,0);metadata_stage=0;
    } else if(metadata_stage==5) {
        DWORD trailer[2];
        if(!metadata_read(process,metadata_work_storage+2040,trailer,8)||trailer[0]||trailer[1]!=metadata_format_bytes)return -1;
        metadata_stage=6;
    } else if(metadata_stage==6) {
        DWORD work[4];
        if(!metadata_read(process,metadata_work_owner,work,16)||work[0]||work[1]!=metadata_work_storage||work[2]!=metadata_work_storage||work[3]!=metadata_work_storage)return -1;
        metadata_stage=7;
    } else if(metadata_stage==7) {
        unsigned char output[256];DWORD work[4],trailer[2],old_work;
        if(current->Eax!=metadata_format_bytes||!metadata_read(process,metadata_work_slot+64,output,256)||memcmp(output,"metadata_sink:6",metadata_format_bytes)||
           !metadata_read(process,metadata_work_owner,work,16)||work[0]!=metadata_format_bytes||work[1]!=metadata_work_storage+metadata_format_bytes||work[2]!=metadata_work_storage||work[3]!=metadata_work_storage||
           !metadata_read(process,metadata_work_storage+2040,trailer,8)||trailer[0]||trailer[1]!=metadata_format_bytes||
           !metadata_read(process,base+0x4f4f4,&old_work,4)||old_work!=metadata_original_work)return -1;
        for(i=metadata_format_bytes;i<256;++i)if(output[i]!=0xa5)return -1;
        sprintf(line,"metadata-format-after bytes%lu read%lu text metadata_sink:6 tail-a5%lu original-work-unchanged1\n",metadata_format_bytes,current->Eax,256-metadata_format_bytes);WriteFile(log,line,strlen(line),&written,0);
        if(metadata_streams)dump_stream_buffers(process,log,base,metadata_streams);
        TerminateProcess(process,99);return 1;
    } else return -1;
    return metadata_launch(process,thread,log,base,current);
}
#endif
