/* Explicit97 progression evidence: actual next block callback consumes no heap
   on its concretely observed suppression==0 path. Context bytes are retained. */
static int dump_progress_input(HANDLE child,HANDLE log,DWORD frontier,DWORD *words)
{
    DWORD fields[4],base,capacity,module,cvbase,read_offset,entry[3],bytes,written,i;WORD number;int used;
    unsigned char input[43],header[4];
    if(!entry_progress_only || scalar_checkpoint || (scalar_progress && !scalar_completion))return 1;
    if(frontier!=0x25450 || words[0]!=0x25172 ||
       !ReadProcessMemory(child,(void *)0x335e4,fields,16,&bytes) || bytes!=16 || fields[1] || fields[2] ||
       !ReadProcessMemory(child,(void *)0x3305c,&base,4,&bytes) || bytes!=4 ||
       !ReadProcessMemory(child,(void *)0x33084,&capacity,4,&bytes) || bytes!=4 || capacity!=4746 || fields[0]!=base+4 ||
       !ReadProcessMemory(child,(void *)0x33050,&module,4,&bytes) || bytes!=4 || module!=words[10] ||
       !ReadProcessMemory(child,(void *)module,&number,2,&bytes) || bytes!=2 || number!=10 || words[8]!=base ||
       !ReadProcessMemory(child,(void *)words[9],entry,12,&bytes) || bytes!=12 || entry[0]!=0xa0124 || entry[1]!=4754 || entry[2]!=43 ||
       !ReadProcessMemory(child,(void *)0x33004,&cvbase,4,&bytes) || bytes!=4 ||
       !ReadProcessMemory(child,(void *)0x332a8,&read_offset,4,&bytes) || bytes!=4 || read_offset!=4754+43 ||
       !ReadProcessMemory(child,(void *)base,input,43,&bytes) || bytes!=43 ||
       input[4]!=33 || input[5] || input[6]!=6 || input[7]!=2 ||
       words[5]!=words[6] || fields[3]!=words[5]+4 || !ReadProcessMemory(child,(void *)words[5],header,4,&bytes) || bytes!=4 || memcmp(input,header,4))return 0;
    used=sprintf(object_line,"progress-input module %lu module_address %08lx input %08lx cursor %08lx capacity %lu directory %08lx offset %lu size %lu cvbase %lu read_offset %lu output %08lx output_cursor %08lx bytes ",
        (DWORD)number,module,base,fields[0],capacity,words[9],entry[1],entry[2],cvbase,read_offset,words[5],fields[3]);
    for(i=0;i<43;++i)used+=sprintf(object_line+used,"%02x",input[i]);object_line[used++]='\n';
    return WriteFile(log,object_line,used,&written,0) && written==(DWORD)used;
}
