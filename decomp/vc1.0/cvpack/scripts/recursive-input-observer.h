/* Only the actual recursive comparator's two published, initialized inputs. */
static int recursive_read(HANDLE child,DWORD address,void *data,DWORD size)
{
    DWORD bytes;
    return ReadProcessMemory(child,(void *)address,data,size,&bytes) && bytes==size;
}
static int recursive_line(HANDLE log,int used,const BYTE *data,DWORD size)
{
    DWORD i,written;
    for(i=0;i<size;++i)used+=sprintf(object_line+used,"%02x",data[i]);
    object_line[used++]='\n';return WriteFile(log,object_line,used,&written,0) && written==(DWORD)used;
}
static int dump_recursive_inputs(HANDLE child,HANDLE log,DWORD frontier,CONTEXT *context,DWORD *words)
{
    DWORD table,base,count,capacity,first,relative,record,header[7],module,directory,entries,entry[3];
    DWORD input,requested,blocks,block0,span=0,subsection=0,offset=0,i,size,page,next,storage[6],block[3],node,position;
    DWORD bucket,buckets,counts,head,cursor,available,arena,hash_count,link[3];WORD number,length;
    int used,found=0;
    if(frontier!=0x1e780)return 1;
    first=count=capacity=0;
    if(words[0]!=0x1ad0d || !recursive_read(child,0x373bc,&table,4) ||
       !recursive_read(child,0x335d8,&first,2) || !recursive_read(child,0x335e0,&count,2) ||
       !recursive_read(child,0x373c0,&capacity,2))return 0;
    relative=(context->Edx&0xffff)-first;
    if(!table || capacity!=12000 || count>capacity || relative>=count || context->Ecx!=table+28*relative ||
       !recursive_read(child,context->Ecx,header,28))return 0;
    used=sprintf(object_line,"recursive-local table %08lx capacity %lu count %lu first %lu type %lu entry %08lx bytes ",
        table,capacity,count,first,context->Edx&0xffff,context->Ecx);
    if(!recursive_line(log,used,(BYTE *)header,28) || (((WORD *)header)[12]&0x100))return 0;
    record=header[0];
    if(!recursive_read(child,0x33050,&module,4) || !module || !recursive_read(child,module,&number,2) ||
       !recursive_read(child,0x33018,&directory,4) || !recursive_read(child,0x3329c,&entries,4) || entries>4096 ||
       !recursive_read(child,0x33064,&input,4) || !recursive_read(child,0x33078,&requested,4) ||
       !recursive_read(child,0x3308c,&blocks,4) || !blocks || !recursive_read(child,blocks,&block0,4) || block0!=input)return 0;
    for(i=0;i<entries;++i) {
        if(!recursive_read(child,directory+i*12,entry,12))return 0;
        if((entry[0]&0xffff)==0x121 && (entry[0]>>16)==number) {
            if(span || entry[2]<4)return 0;span=entry[2]-4;offset=entry[1];subsection=i;
        }
    }
    if(!span || requested>=0xffe8 || span>requested || record<input+4 || record-input>span-4 ||
       !recursive_read(child,record,&length,2))return 0;
    size=(DWORD)length+2;
    if(size<4 || size>4096 || size>span-(record-input) || !recursive_read(child,record,array_bytes,size))return 0;
    used=sprintf(object_line,"recursive-local-data module %lu input %08lx requested %lu defined %lu directory %lu offset %lu record %08lx bytes ",
        (DWORD)number,input,requested,span,subsection,offset,record);
    if(!recursive_line(log,used,array_bytes,size))return 0;
    next=0;if(!recursive_read(child,0x37398,&next,2))return 0;
    relative=(words[2]&0xffff)-0x1000;
    if((words[2]&0xffff)<0x1000 || (words[2]&0xffff)>=next || relative/512>=120 ||
       !recursive_read(child,0x37198+(relative/512)*4,&page,4) || !page ||
       !recursive_read(child,page+(relative%512)*4,&record,4) || record!=words[1] ||
       !recursive_read(child,0x3301c,storage,24) || !storage[0] || storage[1]>64)return 0;
    node=storage[5];
    for(i=0;i<storage[1];++i) {
        if(!node || !recursive_read(child,node,block,12) || !block[1] || block[0]>storage[0])return 0;
        if(record>=block[1] && record-block[1]<block[0]) {
            if(found || block[0]-(record-block[1])<4 || !recursive_read(child,record,&length,2))return 0;
            size=(DWORD)length+2;
            if(size<4 || size>4096 || size>block[0]-(record-block[1]) || !recursive_read(child,record,array_bytes,size))return 0;
            used=sprintf(object_line,"recursive-published index %lu next %lu page %08lx record %08lx block %08lx data %08lx used %lu capacity %lu bytes ",
                words[2]&0xffff,next,page,record,node,block[1],block[0],storage[0]);
            if(!recursive_line(log,used,array_bytes,size))return 0;found=1;
        }
        node=block[2];
    }
    if(!found || node)return 0;
    bucket=((WORD *)header)[5];hash_count=0;
    if(bucket>=4096 || !recursive_read(child,0x37168,&buckets,4) || !recursive_read(child,0x3716c,&counts,4) ||
       !buckets || !counts || !recursive_read(child,buckets+bucket*4,&head,4) ||
       !recursive_read(child,counts+bucket*2,&hash_count,2) || !hash_count || hash_count>1024 ||
       !recursive_read(child,0x352b8,&cursor,4) || !recursive_read(child,0x352b4,&available,4) || available>=1024)return 0;
    arena=cursor-(1024-available)*12;node=head;position=0;
    while(node) {
        if(position>=hash_count || node<arena || node>=cursor || (node-arena)%12 || !recursive_read(child,node,link,12))return 0;
        if(node==context->Ebx)break;
        node=link[2];++position;
    }
    if(!node || link[0]!=words[1] || (link[1]&0xffff)!=(words[2]&0xffff))return 0;
    used=sprintf(object_line,"recursive-hash bucket %lu head %08lx count %lu position %lu cursor %08lx available %lu arena %08lx node %08lx bytes ",
        bucket,head,hash_count,position,cursor,available,arena,node);
    return recursive_line(log,used,(BYTE *)link,12);
}

static int dump_numeric_input(HANDLE child,HANDLE log,DWORD frontier,DWORD *words)
{
    DWORD module,directory,entries,input,requested,blocks,block0,entry[3],span=0,offset=0,selected=0,i;
    WORD number;BYTE tag[2];int used;
    if(frontier!=0x1f1d0)return 1;
    if(words[0]!=0x27766 || !recursive_read(child,0x33050,&module,4) || !module ||
       !recursive_read(child,module,&number,2) || !recursive_read(child,0x33018,&directory,4) ||
       !recursive_read(child,0x3329c,&entries,4) || entries>4096 || !recursive_read(child,0x33064,&input,4) ||
       !recursive_read(child,0x33078,&requested,4) || !recursive_read(child,0x3308c,&blocks,4) ||
       !blocks || !recursive_read(child,blocks,&block0,4) || block0!=input)return 0;
    for(i=0;i<entries;++i) {
        if(!recursive_read(child,directory+i*12,entry,12))return 0;
        if((entry[0]&0xffff)==0x121 && (entry[0]>>16)==number) {
            if(span)return 0;if(entry[2]<4)return 0;span=entry[2]-4;offset=entry[1];selected=i;
        }
    }
    if(!span || span>requested || requested>=0xffe8 || words[1]<input+4 ||
       words[1]-input>span-2 || !recursive_read(child,words[1],tag,2))return 0;
    used=sprintf(object_line,"numeric-type-input module %lu input %08lx requested %lu defined %lu directory %lu source %lu argument %08lx size 2 bytes ",
        (DWORD)number,input,requested,span,selected,offset,words[1]);
    return recursive_line(log,used,tag,2);
}

/* LF_PRECOMP leaf within the actual first initialized type-block payload. */
static int dump_precompiled_input(HANDLE child,HANDLE log,DWORD frontier,DWORD *words)
{
    DWORD module,directory,entries,input,requested,blocks,block0,entry[3],span=0,offset=0,selected=0,i,capacity,size;
    WORD number,length;BYTE record[1024];int used;
    if(frontier!=0x1a2e0 && frontier!=0x1abf0)return 1;
    if(words[0]!=(frontier==0x1a2e0?0x1a0e3:0x1a2f1) || !recursive_read(child,0x33050,&module,4) || !module ||
       !recursive_read(child,module,&number,2) || !recursive_read(child,0x33018,&directory,4) ||
       !recursive_read(child,0x3329c,&entries,4) || entries>4096 || !recursive_read(child,0x33064,&input,4) ||
       !recursive_read(child,0x33078,&requested,4) || !recursive_read(child,0x3308c,&blocks,4) ||
       !blocks || !recursive_read(child,blocks,&block0,4) || block0!=input)return 0;
    for(i=0;i<entries;++i) {
        if(!recursive_read(child,directory+i*12,entry,12))return 0;
        if((entry[0]&0xffff)==0x121 && (entry[0]>>16)==number) {
            if(span || entry[2]<4)return 0;span=entry[2]-4;offset=entry[1];selected=i;
        }
    }
    capacity=requested<0xffe8?requested:0xffe8;
    if(!span || span>capacity || words[1]!=input+(frontier==0x1a2e0?2:12) ||
       !recursive_read(child,input,&length,2))return 0;
    size=(DWORD)length+2;
    if(size<14 || size>sizeof(record) || size>span || !recursive_read(child,input,record,size) ||
       *(WORD *)(record+2)!=0x13)return 0;
    used=sprintf(object_line,"precompiled-type-input module %lu input %08lx capacity %lu defined %lu directory %lu source %lu argument %08lx size %lu bytes ",
        (DWORD)number,input,capacity,span,selected,offset,words[1],size);
    return recursive_line(log,used,record,size);
}

/* Actual published symbol prefix consumed by write_subsection's buffered writer. */
static int dump_symbol_prefix(HANDLE child,HANDLE log,DWORD frontier,CONTEXT *context,DWORD *words)
{
    DWORD directory,count,index,entry[3],module,fields[12],size,requested=0,i,steps=0;int used;
    if(frontier!=0x280f0)return 1;
    if(!symbol_prefix_enabled || words[0]!=0x20d27 || context->Ebx%12 ||
       !recursive_read(child,0x33018,&directory,4) || !recursive_read(child,0x3329c,&count,4))return 0;
    index=context->Ebx/12;size=words[2]&0xffff;
    if(index>=count || !recursive_read(child,directory+index*12,entry,12) ||
       (entry[0]&0xffff)!=0x125 || entry[1]!=words[1] || entry[2]!=size || !size ||
       !recursive_read(child,0x37890,&module,4))return 0;
    while(module) {
        if(++steps>256 || !recursive_read(child,module,fields,48))return 0;
        if((fields[0]&0xffff)==(entry[0]>>16))break;
        module=fields[11];
    }
    if(!module || fields[5]!=words[1] || fields[6]!=size)return 0;
    for(i=0;i<symbol_prefix_count;++i)if(symbol_prefix_allocations[i][0]==words[1] && symbol_prefix_allocations[i][2]==module) {
        if(requested)return 0;requested=symbol_prefix_allocations[i][1];
    }
    if(!requested || size>requested || size>sizeof(walk_bytes) || !recursive_read(child,words[1],walk_bytes,size))return 0;
    used=sprintf(object_line,"packed-symbol-input directory %lu subtype %lu module %lu owner %08lx input %08lx requested %lu defined %lu bytes ",
        index,entry[0]&0xffff,entry[0]>>16,module,words[1],requested,size);
    return recursive_line(log,used,walk_bytes,size);
}

/* Recycling consumes the pointer published by a live local TypeRecord. */
static int dump_type_recycle_owner(HANDLE child,HANDLE log,DWORD frontier,CONTEXT *context,DWORD *words)
{
    DWORD table,capacity=0,count=0,owner,header[7];
    int used;
    if(frontier!=0x1f120 && frontier!=0x1f0d0)return 1;
    owner=context->Esi;
    if(words[0]!=(frontier==0x1f120?0x1cb6c:0x1cb83) ||
       !recursive_read(child,0x373bc,&table,4) ||
       !recursive_read(child,0x373c0,&capacity,2) ||
       !recursive_read(child,0x335e0,&count,2) || !count || count>capacity || capacity>12000 ||
       owner<table || owner-table>=count*28 || (owner-table)%28 ||
       !recursive_read(child,owner,header,28) || header[0]!=words[1] ||
       (((WORD *)header)[12]&0x38)!=(frontier==0x1f120?0x20:0x10))return 0;
    used=sprintf(object_line,"input-buffer %08lx ",owner);
    return recursive_line(log,used,(BYTE *)header,28);
}

/* The feedback name is a counted string in the current module's read record. */
static int dump_module_name_input(HANDLE child,HANDLE log,DWORD frontier,DWORD *words)
{
    DWORD root,header[12],directory,entries,i,record[3],found=0,index=0,offset;
    BYTE data[1024];int used;
    if(frontier!=0x1eea0)return 1;
    if(!recursive_read(child,0x33050,&root,4) || root!=words[1] ||
       !recursive_read(child,root,header,48) || !header[3] || header[4]<8 || header[4]>sizeof(data) ||
       !recursive_read(child,0x33018,&directory,4) || !recursive_read(child,0x3329c,&entries,4) || entries>4096)return 0;
    for(i=0;i<entries;++i) {
        if(!recursive_read(child,directory+i*12,record,12))return 0;
        if((record[0]&0xffff)==0x120 && (record[0]>>16)==(header[0]&0xffff)) {
            if(record[1]!=header[3] || record[2]!=header[4])return 0;
            ++found;index=i;
        }
    }
    if(found!=1 || !recursive_read(child,header[3],data,header[4]))return 0;
    offset=8+12*(DWORD)*(WORD *)(data+4);
    if(offset>=header[4] || data[offset]>=128 || (DWORD)data[offset]+1>header[4]-offset)return 0;
    used=sprintf(object_line,"module-name-record module %08lx directory %lu record %08lx size %lu bytes ",root,index,header[3],header[4]);
    return recursive_line(log,used,data,header[4]);
}

/* Close consumes allocation identities and arena next links, not payload data. */
static int dump_type_server_close_input(HANDLE child,HANDLE log,DWORD frontier,DWORD *words)
{
    DWORD root,parent[29],block[3],address,index,family;int used;
    if(frontier!=0x2bbe8)return 1;
    if(words[0]!=0x1a687 || !recursive_read(child,0x373d0,&root,4) || root!=words[1] ||
       !recursive_read(child,root,parent,sizeof(parent)))return 0;
    used=sprintf(object_line,"pdb-close-parent %08lx ",root);
    if(!recursive_line(log,used,(BYTE *)parent,sizeof(parent)))return 0;
    for(family=0;family<2;++family) {
        address=parent[family?21:18];index=0;
        while(address) {
            if(index==8 || !recursive_read(child,address,block,sizeof(block)))return 0;
            used=sprintf(object_line,"pdb-close-block %lu %lu %08lx ",family,index,address);
            if(!recursive_line(log,used,(BYTE *)block,sizeof(block)))return 0;
            address=block[2];++index;
        }
    }
    return 1;
}
