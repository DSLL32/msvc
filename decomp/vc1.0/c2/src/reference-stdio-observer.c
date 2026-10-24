/* Small debugger used only by the differential harness. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <recon.h>

/* Observation-only breakpoints in otherwise untouched original CRT code. */
typedef struct ObservedCall {
    DWORD rva, nargs, return_address, args[8];
    unsigned char first, return_byte;
    int entered, returned;
    unsigned entry_order;
    int fastcall;
    char name[80];
} ObservedCall;
static ObservedCall observed_calls[32];
static unsigned observed_count, entered_count;
static HANDLE observation_log;
static DWORD image_base;
static char printf_format[64],stop_entry[80],stop_return[80];
static int observe_input;
static int observe_entry;
static DWORD rearm_address;
static DWORD read_word(HANDLE process,DWORD address)
{
    DWORD value,got;
    if(!ReadProcessMemory(process,(void*)address,&value,4,&got)||got!=4)exit(241);
    return value;
}
static DWORD read_byte(HANDLE process,DWORD address)
{
    unsigned char value;DWORD got;
    if(!ReadProcessMemory(process,(void*)address,&value,1,&got)||got!=1)exit(295);
    return value;
}
static void write_byte(HANDLE process,DWORD address,unsigned char value)
{
    DWORD wrote;
    if(!WriteProcessMemory(process,(void*)address,&value,1,&wrote)||wrote!=1)exit(242);
    FlushInstructionCache(process,(void*)address,1);
}
static void log_line(char *line)
{
    DWORD wrote;
    if(!WriteFile(observation_log,line,strlen(line),&wrote,0)||wrote!=strlen(line))exit(243);
}
static void dump_bytes(HANDLE process,char *name,DWORD address,unsigned length)
{
    unsigned char bytes[4096];char line[8300];DWORD got,i;
    if(length>sizeof(bytes))exit(258);
    if(length&&(!ReadProcessMemory(process,(void*)address,bytes,length,&got)||got!=length))exit(259);
    sprintf(line,"%s ",name);log_line(line);
    for(i=0;i<length;++i)sprintf(line+2*i,"%02x",bytes[i]);line[2*length]=0;
    log_line(line);log_line("\n");
}
static unsigned string_bytes(HANDLE process,DWORD address)
{
    DWORD got;unsigned i;unsigned char byte;
    for(i=0;i<512;++i){if(!ReadProcessMemory(process,(void*)(address+i),&byte,1,&got)||got!=1)exit(260);if(!byte)return i+1;}
    exit(261);return 0;
}
/* Listing-input observation only. The caller owns256 destination bytes;
   buffered input bytes are limited to the FILE's actually initialized span. */
static void input_snapshot(HANDLE process,ObservedCall *call,CONTEXT *context)
{
    DWORD stream,fields[8],got,used,valid,handle,position,high=0,flags;
    HANDLE copy;
    char line[512];
    stream=read_word(process,image_base+0x6e1d8);
    if(!ReadProcessMemory(process,(void*)stream,fields,sizeof(fields),&got)||got!=sizeof(fields))exit(264);
    used=fields[0]&&fields[2]?fields[0]-fields[2]:0;
    valid=used+((long)fields[1]>0?fields[1]:0);
    if(valid>4096||fields[4]>=64)exit(265);
    flags=read_word(process,image_base+0x76778+fields[4])&255;
    handle=read_word(process,image_base+0x767b8+4*fields[4]);
    if(!DuplicateHandle(process,(HANDLE)handle,GetCurrentProcess(),&copy,0,FALSE,DUPLICATE_SAME_ACCESS))exit(266);
    position=SetFilePointer(copy,0,(LONG*)&high,FILE_CURRENT);CloseHandle(copy);
    if(high||position==0xffffffff)exit(267);
    sprintf(line,"input stream %lu cw %lu sw %lu flags %lu count %ld fd %lu size %lu null %lu used %lu valid %lu position %lu osfile %lu\n",
        stream,context->FloatSave.ControlWord&65535,context->FloatSave.StatusWord&65535,fields[3],(long)fields[1],fields[4],fields[6],!fields[0]&&!fields[2],used,valid,position,flags);log_line(line);
    dump_bytes(process,"file_buffer_hex",fields[2],valid);
    dump_bytes(process,"destination_hex",call->args[0],256);
    log_line("input_end\n");
}
static void entry_snapshot(HANDLE process,ObservedCall *call,CONTEXT *context)
{
    char line[256];
    sprintf(line,"entry_inputs column %lu cw %lu sw %lu fastcall %d\n",read_word(process,image_base+0x70d40),context->FloatSave.ControlWord&65535,context->FloatSave.StatusWord&65535,call->fastcall);log_line(line);
    if(!strcmp(call->name,"@write_listing_bytes@12")) {
        if((long)call->args[1]<0||call->args[1]>20)exit(268);
        dump_bytes(process,"bytes_hex",call->args[2],call->args[1]);
    }
    if(!strcmp(call->name,"@write_instruction_text@4") ||
       !strcmp(call->name,"@write_special_instruction@4") ||
       !strcmp(call->name,"@write_instruction_mnemonic@4") ||
       !strcmp(call->name,"@write_instruction_comment@4") ||
       !strcmp(call->name,"@write_instruction_operand@12") ||
       !strcmp(call->name,"@write_operand_prefix@12"))
        dump_bytes(process,"instruction_hex",call->args[0],32);
    if(!strcmp(call->name,"@write_instruction_operand@12") ||
       !strcmp(call->name,"@write_operand_prefix@12"))
        dump_bytes(process,"operand_hex",call->args[1],8);
    if(!strcmp(call->name,"_coff_output_debug_pair") ||
       !strcmp(call->name,"@coff_output_debug_pair@4")) {
        dump_bytes(process,"filename_hex",call->args[0],string_bytes(process,call->args[0]));
        sprintf(line,"coff_state emitted %lu codeview %lu line %lu pending %lu source %08lx listing_source %08lx\n",
            read_word(process,image_base+0x750c8),read_word(process,image_base+0x7313c),
            read_word(process,image_base+0x73140),read_word(process,image_base+0x6e8b4)&65535,
            read_word(process,image_base+0x6901c),read_word(process,image_base+0x6e8a0));
        log_line(line);
    }
    if(!strcmp(call->name,"_end_debug_function") ||
       !strcmp(call->name,"@end_debug_function@4") ||
       !strcmp(call->name,"@write_codeview_function@4") ||
       !strcmp(call->name,"@write_debug_scopes@4") ||
       !strcmp(call->name,"_coff_output_size") ||
       !strcmp(call->name,"@coff_output_size@8")) {
        DWORD symbol=call->args[strstr(call->name,"coff_output_size")?1:0];
        DWORD function=read_word(process,image_base+0x731b4);
        dump_bytes(process,"symbol_hex",symbol,84);
        dump_bytes(process,"symbol_name_hex",read_word(process,symbol+16),
            string_bytes(process,read_word(process,symbol+16)));
        sprintf(line,"symbol_state current_symbol %08lx current_function %08lx scope %08lx\n",
            read_word(process,image_base+0x731ac),function,read_word(process,function+36));
        log_line(line);
        dump_bytes(process,"function_hex",function,44);
        if(!strcmp(call->name,"@write_debug_scopes@4")) {
            DWORD cursor=read_word(process,image_base+0x6e8ac);
            sprintf(line,"scope_cursor %08lx\n",cursor);log_line(line);
            if(cursor)dump_bytes(process,"scope_cursor_hex",cursor,24);
        }
    }
    if(!strcmp(call->name,"@write_debug_symbol@4") ||
       !strcmp(call->name,"@write_codeview_label@4") ||
       !strcmp(call->name,"@write_codeview_frame_variable@4")) {
        DWORD symbol=call->args[0],name=read_word(process,symbol+16);
        DWORD cursor=read_word(process,image_base+0x6e8ac);
        DWORD record=cursor?read_word(process,cursor+12):0,index=0;
        while(record&&read_word(process,record+4)!=symbol&&index<64) {
            record=read_word(process,record);++index;
        }
        if(!record||index==64)exit(269);
        dump_bytes(process,"descriptor_hex",symbol,44);
        if(name)dump_bytes(process,"descriptor_name_hex",name,string_bytes(process,name));
        else log_line("descriptor_name_hex \n");
        sprintf(line,"debug_symbol_state cursor %08lx record %08lx record_symbol %08lx allocation %lu optimize %lu record_index %lu coff %lu byte_first %lu\n",
            cursor,record,record?read_word(process,record+4):0,
            read_word(process,image_base+0x74ff8),read_word(process,image_base+0x70624),index,
            read_word(process,image_base+0x706f4),read_word(process,image_base+0x706f0));
        log_line(line);
        sprintf(line,"function_flags %lu\n",read_word(process,read_word(process,image_base+0x731b4)+40));
        log_line(line);
    }
    if(!strcmp(call->name,"@write_codeview_typedef@4") ||
       !strcmp(call->name,"@write_codeview_local_data@4")) {
        DWORD symbol=call->args[0],identifier=read_word(process,symbol+28);
        DWORD name=read_word(process,symbol+16),bucket=identifier&63,index=0;
        DWORD member=read_word(process,image_base+0x6c158+4*bucket);
        while(member&&member!=symbol&&index<1024) {
            member=read_word(process,member);++index;
        }
        if(!member||index==1024)exit(270);
        dump_bytes(process,"descriptor_hex",symbol,44);
        dump_bytes(process,"descriptor_name_hex",name,string_bytes(process,name));
        sprintf(line,"global_symbol_state bucket %lu index %lu member %08lx coff %lu byte_first %lu\n",
            bucket,index,member,read_word(process,image_base+0x706f4),read_word(process,image_base+0x706f0));
        log_line(line);
    }
    if(!strcmp(call->name,"@read_codeview_initializers@4")) {
        DWORD stream=call->args[0],fields[8],got,used,valid,handle,position,high=0;
        DWORD flags,file_bytes,file_type;HANDLE copy;char state[512];
        if(!ReadProcessMemory(process,(void*)stream,fields,sizeof(fields),&got)||got!=sizeof(fields))exit(271);
        used=fields[0]&&fields[2]?fields[0]-fields[2]:0;
        valid=used+((long)fields[1]>0?fields[1]:0);
        if(valid>4096||fields[4]>=64)exit(272);
        flags=read_word(process,image_base+0x76778+fields[4])&255;
        handle=read_word(process,image_base+0x767b8+4*fields[4]);
        if(!DuplicateHandle(process,(HANDLE)handle,GetCurrentProcess(),&copy,0,FALSE,DUPLICATE_SAME_ACCESS))exit(273);
        position=SetFilePointer(copy,0,(LONG*)&high,FILE_CURRENT);
        if(high||position==0xffffffff)exit(274);
        file_bytes=GetFileSize(copy,&high);file_type=GetFileType(copy);CloseHandle(copy);
        if(high||file_bytes==0xffffffff)exit(275);
        sprintf(state,"codeview_file owner %08lx stream %08lx flags %lu count %ld fd %lu size %lu null %lu used %lu valid %lu position %lu osfile %lu file_bytes %lu file_type %lu temporary_null %lu\n",
            read_word(process,image_base+0x6b8ec),stream,fields[3],(long)fields[1],fields[4],fields[6],
            !fields[0]&&!fields[2],used,valid,position,flags,file_bytes,file_type,!fields[7]);log_line(state);
        dump_bytes(process,"codeview_file_hex",stream,32);
        dump_bytes(process,"codeview_buffer_hex",fields[2],valid);
    }
    if(!strcmp(call->name,"@release_initializer@4")) {
        DWORD node=call->args[0],count=0,free_head=read_word(process,image_base+0x6c144);
        sprintf(line,"initializer_state head %08lx free_head %08lx\n",node,free_head);log_line(line);
        while(node&&count<256) {
            sprintf(line,"initializer_node address %08lx next %08lx link %08lx\n",node,
                read_word(process,node+4),read_word(process,node+8));log_line(line);
            dump_bytes(process,"initializer_hex",node,28);
            node=read_word(process,node+4);++count;
        }
        if(node)exit(276);
        if(free_head)dump_bytes(process,"free_initializer_hex",free_head,28);
    }
    if(!strcmp(call->name,"@coff_write_lines@8")) {
        DWORD contribution=call->args[0],last=read_word(process,contribution+20);
        DWORD block=read_word(process,last+4),seen=0,debug=read_word(process,image_base+0x6905c);
        DWORD vector=read_word(process,image_base+0x751a8),section=read_word(process,contribution+4);
        sprintf(line,"coff_lines_state last %08lx free_head %08lx debug %lu vector %08lx section %08lx owner %08lx file %08lx\n",
            last,read_word(process,image_base+0x751cc),debug,vector,section,read_word(process,section),read_word(process,image_base+0x69064));log_line(line);
        dump_bytes(process,"contribution_hex",contribution,56);
        do {
            DWORD count=read_word(process,block),i;
            if(count>50||++seen>256)exit(277);
            sprintf(line,"coff_lines_block address %08lx count %lu next %08lx\n",block,count,read_word(process,block+4));log_line(line);
            dump_bytes(process,"coff_lines_hex",block+8,count*6);
            for(i=0;i<count;++i)if((debug&1)&&!(read_word(process,block+8+i*6+4)&65535)) {
                DWORD index=read_word(process,block+8+i*6)+2,slot=index-1;
                DWORD capacity=read_word(process,vector+4)&65535,chunk=read_word(process,vector+24),steps=0;
                if(!capacity||!index||index>read_word(process,vector))exit(278);
                while(slot>=capacity){slot-=capacity;chunk=read_word(process,chunk);if(!chunk||++steps>256)exit(279);}
                { DWORD entry=read_word(process,chunk+4+4*slot),auxiliary=read_word(process,entry+4);
                  sprintf(line,"coff_lines_aux index %lu entry %08lx auxiliary %08lx\n",index,entry,auxiliary);log_line(line);
                  dump_bytes(process,"coff_lines_aux_hex",auxiliary,18); }
            }
            if(block==last)break;
            block=read_word(process,block+4);
        }while(block);
        if(block!=last)exit(280);
    }
    if(!strcmp(call->name,"_omf_open_module")) {
        sprintf(line,"omf_entry coff_object %lu\n",read_word(process,image_base+0x706f4));log_line(line);
    }
    if(!strcmp(call->name,"@omf_write_module_header@4") ||
       !strcmp(call->name,"@omf_name_index@4") ||
       !strcmp(call->name,"@omf_append_string@8") ||
       !strcmp(call->name,"@omf_append_bytes@12") ||
       !strcmp(call->name,"@omf_write_comment@8")) {
        unsigned index=(!strcmp(call->name,"@omf_append_string@8") ||
            !strcmp(call->name,"@omf_append_bytes@12") ||
            !strcmp(call->name,"@omf_write_comment@8"))?1:0;
        unsigned bytes=!strcmp(call->name,"@omf_append_bytes@12");
        unsigned comment=!strcmp(call->name,"@omf_write_comment@8");
        sprintf(line,"omf_string source_global %08lx\n",read_word(process,image_base+0x705ec));log_line(line);
        dump_bytes(process,"omf_string_hex",call->args[index],string_bytes(process,call->args[index]));
        if(comment) {
            DWORD directive=read_word(process,image_base+0x74fdc);
            sprintf(line,"omf_comment kind %lu owner %08lx\n",call->args[0]&255,
                read_word(process,directive+8));log_line(line);
        } else if(index) {
            sprintf(line,"omf_record pointer %08lx current %08lx base %08lx\n",
                call->args[0],read_word(process,image_base+0x6b88c),image_base+0x6a8dc);log_line(line);
            dump_bytes(process,"omf_record_hex",call->args[0],bytes?24:23);
        }
    }
    if(!strcmp(call->name,"@omf_output_record@4")) {
        DWORD directive=call->args[0],kind=read_word(process,directive+4);
        DWORD text=read_word(process,directive+8),bytes=0;
        sprintf(line,"omf_directive pointer %08lx head %08lx next %08lx kind %lu text %08lx\n",
            directive,read_word(process,image_base+0x74fdc),read_word(process,directive),kind,text);log_line(line);
        dump_bytes(process,"omf_directive_hex",directive,12);
        if(kind!=1 && text) {
            if(kind==7) {
                unsigned char raw;
                signed char length;
                DWORD got;
                if(!ReadProcessMemory(process,(void*)text,&raw,1,&got)||got!=1)exit(287);
                length=(signed char)raw;
                if(length<0)exit(286);
                bytes=1+length;
            } else bytes=string_bytes(process,text);
        }
        dump_bytes(process,"omf_directive_text_hex",text,bytes);
    }
    if(!strcmp(call->name,"@omf_intern_name@8")) {
        sprintf(line,"omf_name pointer %08lx output %08lx entry_sp %08lx next_index %lu\n",
            call->args[0],call->args[1],context->Esp,read_word(process,image_base+0x75084));log_line(line);
        dump_bytes(process,"omf_name_hex",call->args[0],string_bytes(process,call->args[0]));
        dump_bytes(process,"omf_name_buckets_hex",image_base+0x6b890,16*4);
        /* The caller's four-byte index local is write-only at this boundary.
           Its prior uninitialized bytes are neither read nor compared. */
    }
    if(!strcmp(call->name,"@omf_output_bytes@8")) {
        DWORD symbol=call->args[1],node=context->Ebx;
        DWORD id=read_word(process,node+8),bucket=id&63,index=0;
        DWORD member=read_word(process,image_base+0x6c158+4*bucket);
        while(member&&read_word(process,member+28)!=id&&index<1024){member=read_word(process,member);++index;}
        if(!member||index==1024)exit(297);
        sprintf(line,"omf_absolute displacement %08lx symbol %08lx referenced_owner %08lx return_address %08lx identifier %lu bucket %lu index %lu member %08lx\n",
            call->args[0],symbol,context->Ebp,call->return_address,id,bucket,index,member);log_line(line);
    }
    if(!strcmp(call->name,"@omf_write_datum@8")) {
        DWORD record=call->args[0],node=call->args[1];
        long width=(short)(read_word(process,node+8)>>16);
        if(width!=1&&width!=2&&width!=4&&width!=8&&width!=10)exit(296);
        sprintf(line,"omf_datum record %08lx initializer %08lx current %08lx base %08lx return_address %08lx initializer_owner %08lx record_owner %08lx\n",
            record,node,read_word(process,image_base+0x6b88c),image_base+0x6a8dc,call->return_address,context->Ebx,context->Edi);log_line(line);
        dump_bytes(process,"omf_datum_header_hex",node+8,4);
        dump_bytes(process,"omf_datum_value_hex",node+12,width);
        dump_bytes(process,"omf_datum_count_cursor_hex",record+12,8);
    }
    if(!strcmp(call->name,"@omf_pad_record@8")) {
        DWORD record=call->args[0];
        sprintf(line,"omf_pad record %08lx bytes %lu current %08lx base %08lx return_address %08lx\n",
            record,call->args[1],read_word(process,image_base+0x6b88c),image_base+0x6a8dc,call->return_address);log_line(line);
        /* Only the count/cursor are consumed; old destination bytes are write-only. */
        dump_bytes(process,"omf_pad_count_cursor_hex",record+12,8);
    }
    if(!strcmp(call->name,"@omf_write_byte_array@16")) {
        DWORD segment=call->args[0],bytes=call->args[1],source=call->args[3];
        DWORD initializer=context->Ebx;
        if(bytes>4096)exit(295);
        sprintf(line,"omf_byte_array segment %08lx bytes %lu repeat %lu source %08lx return_address %08lx initializer %08lx owner_source %08lx owner_repeat %lu owner_width %ld combine %lu position %lu data_segment %08lx\n",
            segment,bytes,call->args[2],source,call->return_address,initializer,
            read_word(process,initializer+12),read_word(process,initializer+24),
            (long)(short)(read_word(process,initializer+8)>>16),
            read_byte(process,segment+53),read_word(process,segment+4),read_word(process,image_base+0x6b8d8));log_line(line);
        dump_bytes(process,"omf_byte_array_data_hex",source,source?bytes:0);
    }
    if(!strcmp(call->name,"@omf_output_relocation@12")) {
        DWORD node=call->args[0],symbol=call->args[1],segment=call->args[2];
        DWORD kind=read_word(process,node),id,member,bucket,index,name;
        long width,type;DWORD source;
        sprintf(line,"omf_relocation initializer %08lx symbol %08lx segment %08lx entry_sp %08lx return_address %08lx loop_owner %08lx symbol_owner %08lx segment_owner %08lx combine %lu position %lu\n",
            node,symbol,segment,context->Esp,call->return_address,context->Edi,context->Ebx,context->Ebp,
            read_byte(process,segment+53),read_word(process,segment+4));log_line(line);
        sprintf(line,"omf_segment_owners code %08lx data %08lx constant %08lx bss %08lx debug_symbols %08lx debug_types %08lx\n",
            read_word(process,image_base+0x6b8d4),read_word(process,image_base+0x6b8d8),read_word(process,image_base+0x6b8dc),read_word(process,image_base+0x6b8d0),read_word(process,image_base+0x6914c),read_word(process,image_base+0x6b8e0));log_line(line);
        dump_bytes(process,"omf_initializer_kind_hex",node,4);
        if(kind==1) {
            type=(short)read_word(process,node+8);width=(short)(read_word(process,node+8)>>16);
            if(width<0||width>4096||(type!=6&&width>12))exit(293);
            dump_bytes(process,"omf_initializer_datum_hex",node+8,4);
            dump_bytes(process,"omf_initializer_repeat_hex",node+24,4);
            source=type==6?read_word(process,node+12):node+12;
            sprintf(line,"omf_initializer_data_pointer %08lx\n",source);log_line(line);
            dump_bytes(process,"omf_initializer_data_hex",source,width);
        } else if(kind==2) {
            dump_bytes(process,"omf_initializer_relocation_hex",node+8,9);
            id=read_word(process,node+8);bucket=id&63;index=0;
            member=read_word(process,image_base+0x6c158+4*bucket);
            while(member&&read_word(process,member+28)!=id&&index<1024){member=read_word(process,member);++index;}
            if(!member||index==1024)exit(294);
            kind=(signed char)read_byte(process,member+12);name=read_word(process,member+16);
            sprintf(line,"omf_initializer_reference pointer %08lx identifier %lu kind %ld label_kind %ld bucket %lu index %lu member %08lx\n",
                member,id,kind,kind==3?(long)read_byte(process,member+48):-1L,bucket,index,member);log_line(line);
            dump_bytes(process,"omf_initializer_reference_name_hex",name,string_bytes(process,name));
        } else if(kind==4)dump_bytes(process,"omf_initializer_alignment_hex",node+8,1);
        else if(kind==8)dump_bytes(process,"omf_initializer_padding_hex",node+8,4);
    }
    if(!strcmp(call->name,"@omf_emit_external@4")) {
        DWORD symbol=call->args[0],identifier=read_word(process,symbol+28);
        DWORD name=read_word(process,symbol+16),bucket=identifier&63,index=0;
        DWORD member=read_word(process,image_base+0x6c158+4*bucket);
        while(member&&member!=symbol&&index<1024){member=read_word(process,member);++index;}
        if(!member||index==1024)exit(292);
        sprintf(line,"omf_external symbol %08lx bucket %lu index %lu member %08lx next_external %lu\n",
            symbol,bucket,index,member,read_word(process,image_base+0x75078));log_line(line);
        dump_bytes(process,"omf_external_header_hex",symbol+12,4);
        dump_bytes(process,"omf_external_identifier_hex",symbol+28,4);
        dump_bytes(process,"omf_external_attributes_hex",symbol+36,4);
        dump_bytes(process,"omf_external_name_hex",name,string_bytes(process,name));
        /* Symbol.field14 is write-only in this function; do not inspect it. */
    }
    if(!strcmp(call->name,"@omf_emit_public@8")) {
        DWORD symbol=call->args[0],identifier=read_word(process,symbol+28);
        DWORD name=read_word(process,symbol+16),bucket=identifier&63,index=0;
        DWORD member=read_word(process,image_base+0x6c158+4*bucket);
        while(member&&member!=symbol&&index<1024){member=read_word(process,member);++index;}
        if(!member||index==1024)exit(291);
        sprintf(line,"omf_public symbol %08lx segment %08lx segment_owner %08lx bucket %lu index %lu member %08lx\n",
            symbol,call->args[1],read_word(process,image_base+0x6b8d8),bucket,index,member);log_line(line);
        dump_bytes(process,"omf_public_offset_hex",symbol+4,4);
        dump_bytes(process,"omf_public_header_hex",symbol+12,4);
        dump_bytes(process,"omf_public_identity_hex",symbol+24,8);
        dump_bytes(process,"omf_public_type_hex",symbol+32,8);
        dump_bytes(process,"omf_public_name_hex",name,string_bytes(process,name));
    }
    if(!strcmp(call->name,"@omf_mark_symbol@4")) {
        DWORD symbol=call->args[0],identifier=read_word(process,symbol+28);
        DWORD name=read_word(process,symbol+16),bucket=identifier&63,index=0;
        DWORD member=read_word(process,image_base+0x6c158+4*bucket);
        DWORD flags=read_word(process,symbol+34)&65535,kind=read_word(process,symbol+12)&255;
        while(member&&member!=symbol&&index<1024){member=read_word(process,member);++index;}
        if(!member||index==1024)exit(290);
        sprintf(line,"omf_mark pointer %08lx bucket %lu index %lu member %08lx\n",symbol,bucket,index,member);log_line(line);
        dump_bytes(process,"omf_mark_header_hex",symbol+12,4);
        dump_bytes(process,"omf_mark_identity_hex",symbol+24,8);
        dump_bytes(process,"omf_mark_flags_hex",symbol+34,((flags&5)==4&&(kind==1||kind==4))?6:2);
        dump_bytes(process,"omf_mark_name_hex",name,string_bytes(process,name));
    }
    if(!strcmp(call->name,"@omf_reserve_bytes@8")) {
        DWORD current=read_word(process,image_base+0x6b88c);
        DWORD record=call->args[0],used=read_word(process,record+16)-record;
        if(used<20||used>1004)exit(289);
        sprintf(line,"omf_record pointer %08lx current %08lx base %08lx bytes %lu debug_owner %08lx\n",
            record,current,image_base+0x6a8dc,used,read_word(process,image_base+0x6914c));log_line(line);
        dump_bytes(process,"omf_record_hex",record,used);
    }
    if(!strcmp(call->name,"@omf_output_data@8")) {
        DWORD current=read_word(process,image_base+0x6b88c);
        DWORD used=read_word(process,current+16)-current;
        if(call->args[1]!=4||used<20||used>1004)exit(288);
        sprintf(line,"omf_data source %08lx count %lu entry_sp %08lx return_address %08lx current %08lx base %08lx debug_owner %08lx used %lu\n",
            call->args[0],call->args[1],context->Esp,call->return_address,
            current,image_base+0x6a8dc,read_word(process,image_base+0x6914c),used);log_line(line);
        dump_bytes(process,"omf_data_hex",call->args[0],4);
        dump_bytes(process,"omf_data_record_hex",current,used);
    }
    if(!strcmp(call->name,"@omf_select_segment@4")) {
        sprintf(line,"omf_select segment %08lx owner %08lx\n",
            call->args[0],read_word(process,image_base+0x6914c));log_line(line);
        dump_bytes(process,"omf_select_segment_hex",call->args[0],72);
    }
    if(!strcmp(call->name,"@omf_write_group_definition@8")) {
        sprintf(line,"omf_group segment %08lx segment_owner %08lx group %08lx group_owner %08lx dos %lu\n",
            call->args[0],read_word(process,image_base+0x6b8d8),
            call->args[1],read_word(process,image_base+0x74fc4),
            read_word(process,image_base+0x70684));log_line(line);
        dump_bytes(process,"omf_group_segment_hex",call->args[0],72);
        dump_bytes(process,"omf_group_hex",call->args[1],8);
    }
    if(!strcmp(call->name,"@omf_write_segment_definition@4")) {
        sprintf(line,"omf_segment pointer %08lx owner %08lx next_index %lu empty_name %lu\n",
            call->args[0],read_word(process,image_base+0x6b8d4),
            read_word(process,image_base+0x75080),read_word(process,image_base+0x6a8d8));log_line(line);
        dump_bytes(process,"omf_segment_hex",call->args[0],72);
    }
    if(!strcmp(call->name,"@omf_flush_record_type@4") ||
       !strcmp(call->name,"@omf_find_record@8") ||
       !strcmp(call->name,"@omf_allocate_record@8") ||
       !strcmp(call->name,"@omf_initialize_record@12")) {
        unsigned i;
        sprintf(line,"omf_pool current %08lx base %08lx stride 1004 count 4\n",
            read_word(process,image_base+0x6b88c),image_base+0x6a8dc);log_line(line);
        for(i=0;i<4;++i)dump_bytes(process,"omf_pool_header_hex",image_base+0x6a8dc+i*1004,20);
    }
    if(!strcmp(call->name,"_omf_flush_records") ||
       !strcmp(call->name,"@omf_flush_record@4") ||
       !strcmp(call->name,"@omf_write_record@4") ||
       !strcmp(call->name,"@omf_write_index@8")) {
        DWORD current=read_word(process,image_base+0x6b88c);
        DWORD record=!strcmp(call->name,"_omf_flush_records")?current:call->args[0];
        DWORD cursor=read_word(process,record+16),bytes=cursor-record;
        if(bytes<20||bytes>1004)exit(281);
        sprintf(line,"omf_record pointer %08lx current %08lx base %08lx bytes %lu\n",
            record,current,image_base+0x6a8dc,bytes);log_line(line);
        dump_bytes(process,"omf_record_hex",record,bytes);
        if(!strcmp(call->name,"@omf_write_record@4")) {
            DWORD file=read_word(process,image_base+0x74fb0),fields[8],got,used;
            DWORD handle,position,high=0,file_bytes;
            HANDLE copy;
            if(!ReadProcessMemory(process,(void*)file,fields,sizeof(fields),&got)||got!=sizeof(fields))exit(282);
            used=fields[0]&&fields[2]?fields[0]-fields[2]:0;
            if(used>4096||fields[4]>=64)exit(283);
            handle=read_word(process,image_base+0x767b8+4*fields[4]);
            if(!DuplicateHandle(process,(HANDLE)handle,GetCurrentProcess(),&copy,0,FALSE,DUPLICATE_SAME_ACCESS))exit(284);
            position=SetFilePointer(copy,0,(LONG*)&high,FILE_CURRENT);file_bytes=GetFileSize(copy,0);CloseHandle(copy);
            if(high||position==0xffffffff||file_bytes==0xffffffff)exit(285);
            sprintf(line,"omf_object stream %08lx flags %lu count %ld fd %lu size %lu null %lu used %lu position %lu osfile %lu file_bytes %lu\n",
                file,fields[3],(long)fields[1],fields[4],fields[6],!fields[0]&&!fields[2],used,position,
                read_word(process,image_base+0x76778+fields[4])&255,file_bytes);log_line(line);
            dump_bytes(process,"omf_object_buffer_hex",fields[2],used);
        }
    }
    log_line("entry_end\n");
}
static void numeric_snapshot(HANDLE process,ObservedCall *call,char *stage,CONTEXT *context)
{
    char line[1024];DWORD *args=call->args;int after=!strcmp(stage,"return");
    sprintf(line,"numeric eax %08lx ecx %08lx edx %08lx esp %08lx cw %08lx sw %08lx\n",context->Eax,context->Ecx,context->Edx,context->Esp,context->FloatSave.ControlWord,context->FloatSave.StatusWord);log_line(line);
    if(!strcmp(call->name,"_main")) {
        /* Startup proof stops before any application instruction. */
    } else if(!strcmp(call->name,"_printf")) {
        dump_bytes(process,"format_hex",args[0],string_bytes(process,args[0]));
        sprintf(line,"value_words %08lx%08lx\n",args[1],args[2]);log_line(line);
    } else if(!strcmp(call->name,"__positive")) {
        dump_bytes(process,"input_hex",args[0],8);
    } else if(!strcmp(call->name,"__fltout")) {
        sprintf(line,"value_words %08lx%08lx\n",args[0],args[1]);log_line(line);
        if(after){dump_bytes(process,"float_info_hex",context->Eax,12);dump_bytes(process,"digits_hex",read_word(process,context->Eax+12),string_bytes(process,read_word(process,context->Eax+12)));}
    } else if(!strcmp(call->name,"___dtold")) {
        dump_bytes(process,"input_hex",args[1],8);
        if(after)dump_bytes(process,"extended_hex",args[0],10);
    } else if(!strcmp(call->name,"_$I10_OUTPUT")) {
        sprintf(line,"value_words %08lx%08lx%04lx\n",args[0],args[1],args[2]&65535);log_line(line);
        if(after){dump_bytes(process,"decimal_header_hex",args[5],4);dump_bytes(process,"digits_hex",args[5]+4,string_bytes(process,args[5]+4));}
    } else if(!strcmp(call->name,"__fptostr")) {
        dump_bytes(process,"float_info_hex",args[2],12);dump_bytes(process,"digits_hex",read_word(process,args[2]+12),string_bytes(process,read_word(process,args[2]+12)));
        if(after)dump_bytes(process,"output_hex",args[0],string_bytes(process,args[0]));
    } else if(!strcmp(call->name,"__shift")||!strcmp(call->name,"__cropzeros")||!strcmp(call->name,"__forcdecpt")) {
        dump_bytes(process,"output_hex",args[0],string_bytes(process,args[0]));
    } else if(!strcmp(call->name,"_memcpy")) {
        dump_bytes(process,"input_hex",args[1],args[2]);
        if(after)dump_bytes(process,"output_hex",args[0],args[2]);
    } else if(!strcmp(call->name,"___addl")) {
        if(after)dump_bytes(process,"sum_hex",args[2],4);
    } else if(!strcmp(call->name,"___ld12mul")||!strcmp(call->name,"___add_12")) {
        dump_bytes(process,"left_hex",args[0],12);dump_bytes(process,"right_hex",args[1],12);
    } else if(!strcmp(call->name,"___multtenpow12")||!strcmp(call->name,"___shl_12")||!strcmp(call->name,"___shr_12")) {
        dump_bytes(process,"extended_hex",args[0],12);
    } else {
        dump_bytes(process,"input_hex",args[0],8);
        if(after)dump_bytes(process,"output_hex",args[1],string_bytes(process,args[1]));
    }
    log_line("numeric_end\n");
}
static void snapshot(HANDLE process,ObservedCall *call,char *stage,DWORD eax)
{
    DWORD fields[8],got,i,used,position,high=0,handle,counter=0,cursor=0,value=0;
    HANDLE copy;char line[10000],hex[8193];unsigned char bytes[4096];
    if(!ReadProcessMemory(process,(void*)(image_base+0x761b8),fields,sizeof(fields),&got)||got!=sizeof(fields))exit(244);
    used=fields[0]&&fields[2]?fields[0]-fields[2]:0;
    if(used>4096)exit(245);
    if(used&&(!ReadProcessMemory(process,(void*)fields[2],bytes,used,&got)||got!=used))exit(246);
    for(i=0;i<used;++i)sprintf(hex+2*i,"%02x",bytes[i]);hex[2*used]=0;
    handle=read_word(process,image_base+0x767b8+4);
    if(!DuplicateHandle(process,(HANDLE)handle,GetCurrentProcess(),&copy,0,FALSE,DUPLICATE_SAME_ACCESS))exit(247);
    position=SetFilePointer(copy,0,(LONG*)&high,FILE_CURRENT);CloseHandle(copy);
    if(high||position==0xffffffff)exit(248);
    if(!strcmp(call->name,"_write_char")||!strcmp(call->name,"_write_multi_char")||!strcmp(call->name,"_write_string"))
        counter=read_word(process,call->args[call->nargs-1]);
    if(!strcmp(call->name,"_get_int_arg")||!strcmp(call->name,"_get_short_arg")) {
        cursor=read_word(process,call->args[0]);value=read_word(process,cursor);
    }
    sprintf(line,"%s %s eax %08lx args",stage,call->name,eax);log_line(line);
    for(i=0;i<call->nargs;++i){sprintf(line," %08lx",call->args[i]);log_line(line);}
    sprintf(line,"\nfile flags %lu count %ld fd %lu size %lu null %lu used %lu position %lu counter %lu cursor %08lx value %08lx\nbuffer_hex %s\n",
        fields[3],(long)fields[1],fields[4],fields[6],!fields[0]&&!fields[2],used,position,counter,cursor,value,hex);log_line(line);
}
static void install_plan(HANDLE process,DWORD base)
{
    char path[1024];FILE *plan;unsigned long rva,first,nargs;
    image_base=base;
    GetEnvironmentVariableA("C2_PRINTF_FORMAT",printf_format,sizeof(printf_format));
    GetEnvironmentVariableA("C2_STOP_ENTRY",stop_entry,sizeof(stop_entry));
    GetEnvironmentVariableA("C2_STOP_RETURN",stop_return,sizeof(stop_return));
    observe_input=GetEnvironmentVariableA("C2_STREAM_INPUT",path,sizeof(path))!=0;
    observe_entry=GetEnvironmentVariableA("C2_ENTRY_INPUT",path,sizeof(path))!=0;
    if(!GetEnvironmentVariableA("C2_CRT_CAPTURE",path,sizeof(path)))exit(249);
    observation_log=CreateFileA(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    if(!GetEnvironmentVariableA("C2_CRT_PLAN",path,sizeof(path)))exit(250);
    plan=fopen(path,"r");if(!plan)exit(251);
    while(observed_count<32&&fscanf(plan,"%lx %lx %lu %79s",&rva,&first,&nargs,observed_calls[observed_count].name)==4){
        ObservedCall *call=&observed_calls[observed_count++];
        call->rva=rva;call->first=(unsigned char)first;call->fastcall=(nargs&0x100)!=0;call->nargs=nargs&255;
        if(call->nargs>8||(read_word(process,base+rva)&255)!=first)exit(252);
        write_byte(process,base+rva,0xcc);
    }
    fclose(plan);if(!observed_count)exit(253);
}
static void capture_crt(HANDLE process,HANDLE thread,DWORD base,DWORD address)
{
    unsigned i;ObservedCall *call=0;DWORD got;CONTEXT c;
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
    if(!GetThreadContext(thread,&c))exit(254);
    for(i=0;i<observed_count;++i)if(observed_calls[i].entered&&!observed_calls[i].returned&&observed_calls[i].return_address==address){call=&observed_calls[i];break;}
    if(call){
        unsigned char restored=call->return_byte;int finish=0;
        /* Tail-called CRT wrappers share the same physical return address.
           Observe every active logical boundary there, deepest entry first. */
        for(;;){
            call=0;
            for(i=0;i<observed_count;++i)if(observed_calls[i].entered&&!observed_calls[i].returned&&observed_calls[i].return_address==address&&(!call||observed_calls[i].entry_order>call->entry_order))call=&observed_calls[i];
            if(!call)break;
            snapshot(process,call,"return",c.Eax);if(printf_format[0])numeric_snapshot(process,call,"return",&c);if(observe_input)input_snapshot(process,call,&c);call->returned=1;
            if(call->return_byte!=0xcc)restored=call->return_byte;
            if(!strcmp(call->name,"_printf"))finish=1;
            if(stop_return[0]&&!strcmp(call->name,stop_return))finish=1;
        }
        if(finish){CloseHandle(observation_log);TerminateProcess(process,99);return;}
        write_byte(process,address,restored);
    }else{
        for(i=0;i<observed_count;++i)if(!observed_calls[i].entered&&base+observed_calls[i].rva==address){call=&observed_calls[i];break;}
        if(!call)exit(255);
        if(printf_format[0]&&!strcmp(call->name,"_printf")) {
            DWORD pointer=read_word(process,c.Esp+4),got;char format[64];unsigned n;
            for(n=0;n<sizeof(format)-1;++n){if(!ReadProcessMemory(process,(void*)(pointer+n),format+n,1,&got)||got!=1)exit(262);if(!format[n])break;}format[sizeof(format)-1]=0;
            if(strcmp(format,printf_format)) {
                write_byte(process,address,call->first);rearm_address=address;
                c.Eip=address;c.EFlags|=0x100;if(!SetThreadContext(thread,&c))exit(263);return;
            }
        }
        call->entered=1;call->entry_order=++entered_count;call->return_address=read_word(process,c.Esp);
        if(call->fastcall) {
            if(call->nargs)call->args[0]=c.Ecx;
            if(call->nargs>1)call->args[1]=c.Edx;
            if(call->nargs>2&&(!ReadProcessMemory(process,(void*)(c.Esp+4),call->args+2,(call->nargs-2)*4,&got)||got!=(call->nargs-2)*4))exit(256);
        } else if(!ReadProcessMemory(process,(void*)(c.Esp+4),call->args,call->nargs*4,&got)||got!=call->nargs*4)exit(256);
        call->return_byte=(unsigned char)read_word(process,call->return_address);
        snapshot(process,call,"enter",c.Eax);if(printf_format[0])numeric_snapshot(process,call,"enter",&c);
        if(observe_input)input_snapshot(process,call,&c);
        if(observe_entry)entry_snapshot(process,call,&c);
        if(stop_entry[0]&&!strcmp(stop_entry,call->name)){CloseHandle(observation_log);TerminateProcess(process,99);return;}
        write_byte(process,address,call->first);write_byte(process,call->return_address,0xcc);
    }
    c.Eip=address;if(!SetThreadContext(thread,&c))exit(257);
}

int main(int argc, char **argv)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DEBUG_EVENT ev;
    HANDLE out, err, log;
    DWORD base = 0, written, status, code;
    char command[2048], line[256], environment[4096];
    DWORD environment_length;
    int loader_break = 0;

    if (argc != 6 && argc != 7) return 2;
    memset(&si, 0, sizeof(si));
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    out = CreateFileA(argv[3], GENERIC_WRITE, FILE_SHARE_READ, &sa,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    err = CreateFileA(argv[4], GENERIC_WRITE, FILE_SHARE_READ, &sa,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    log = CreateFileA(argv[5], GENERIC_WRITE, FILE_SHARE_READ, 0,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = out;
    si.hStdError = err;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    sprintf(command, "\"%s\" %s", argv[1], argv[2]);
    strcpy(environment, "MSC_CMD_FLAGS=");
    environment_length = GetEnvironmentVariableA("MSC_CMD_FLAGS", environment + 14, sizeof(environment) - 15);
    if (!environment_length || environment_length >= sizeof(environment)-15) return 5;
    environment[14 + environment_length + 1] = 0;
    if (!CreateProcessA(argv[1], command, 0, 0, TRUE,
                        DEBUG_ONLY_THIS_PROCESS, environment, argc == 7 ? argv[6] : 0, &si, &pi)) {
        sprintf(line, "create-error %lu\n", GetLastError());
        WriteFile(log, line, strlen(line), &written, 0);
        return 3;
    }
    for (;;) {
        if (!WaitForDebugEvent(&ev, 30000)) return 4;
        status = DBG_CONTINUE;
        if (ev.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)ev.u.CreateProcessInfo.lpBaseOfImage;
            install_plan(pi.hProcess,base);
            if (ev.u.CreateProcessInfo.hFile)
                CloseHandle(ev.u.CreateProcessInfo.hFile);
        } else if (ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (ev.u.LoadDll.hFile) CloseHandle(ev.u.LoadDll.hFile);
        } else if (ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = ev.u.Exception.ExceptionRecord.ExceptionCode;
            if(code==EXCEPTION_SINGLE_STEP&&rearm_address) {
                CONTEXT context;memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                if(!GetThreadContext(pi.hThread,&context))return 264;
                context.EFlags&=~0x100;if(!SetThreadContext(pi.hThread,&context))return 265;
                write_byte(pi.hProcess,rearm_address,0xcc);rearm_address=0;
            } else if (code == EXCEPTION_BREAKPOINT && !loader_break) {
                loader_break = 1;
            } else {
                sprintf(line, "exception %08lx rva %08lx base %08lx first %lu\n",
                        code, (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress - base,
                        base, ev.u.Exception.dwFirstChance);
                WriteFile(log, line, strlen(line), &written, 0);
                if(code!=EXCEPTION_BREAKPOINT) return 224;
                capture_crt(pi.hProcess,pi.hThread,base,(DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress);
            }
        } else if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            sprintf(line, "exit %lu\n", ev.u.ExitProcess.dwExitCode);
            WriteFile(log, line, strlen(line), &written, 0);
            ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, status);
            break;
        }
        ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, status);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(out);
    CloseHandle(err);
    CloseHandle(log);
    return 0;
}
