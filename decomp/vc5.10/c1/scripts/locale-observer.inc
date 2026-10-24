/* Original-only isolated canonical CRT observation; no child query calls.
 * Transport adapted from C1XX locale-observer.inc, SHA256 937791bdc4096b31fb59bcafb668c025805f91ed49259a0c99140ccf3afd9578.
 * All target addresses/extents and captured state below belong to C1. */
static DWORD locale_export(const char *wanted)
{
    /* Only authenticated, immutable export identities are cached. Every
     * locale/global/table value is read afresh at every checkpoint. */
    static struct LocaleExport { const char *name; DWORD rva,address; } known[] = {
        {"__mb_cur_max",0x3fba0,0},{"__lc_codepage",0x3f8f8,0},{"__lc_handle",0x3f1a8,0},
        {"_pctype",0x3fb9c,0},{"_mbctype",0x3f1c8,0},{"_ctype",0x3fba8,0},{"setlocale",0x5729,0},
        {"__argc",0x3fa84,0},{"__argv",0x3fa8c,0},{"_environ",0x3f8fc,0},
        {"_errno",0x6436,0},{"__doserrno",0x642d,0},{"_getmbcp",0xa573,0},{"_setmbcp",0x2257,0}
    };
    static DWORD module;
    unsigned item; DWORD index,name,address,rva; WORD slot; char text[128];
    IMAGE_DOS_HEADER dos; IMAGE_NT_HEADERS nt; IMAGE_EXPORT_DIRECTORY directory;
    if(!module) {
        readmem(crt_base,&dos,sizeof(dos));readmem(crt_base+dos.e_lfanew,&nt,sizeof(nt));
        rva=nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        readmem(crt_base+rva,&directory,sizeof(directory));if(directory.NumberOfNames>4096)fail("locale export bounds");
        for(index=0;index<directory.NumberOfNames;++index) {
            readmem(crt_base+(DWORD)directory.AddressOfNames+index*4,&name,4);
            if(name+sizeof(text)<name || name+sizeof(text)>nt.OptionalHeader.SizeOfImage)fail("locale export name bounds");
            readmem(crt_base+name,text,sizeof(text));if(!memchr(text,0,sizeof(text)))fail("locale export name length");
            for(item=0;item<sizeof(known)/sizeof(known[0]);++item)if(!strcmp(text,known[item].name)) {
                readmem(crt_base+(DWORD)directory.AddressOfNameOrdinals+index*2,&slot,2);
                if(slot>=directory.NumberOfFunctions || known[item].address)fail("locale export ordinal/duplicate");
                readmem(crt_base+(DWORD)directory.AddressOfFunctions+slot*4,&address,4);
                if(address!=known[item].rva)fail("canonical locale export identity");known[item].address=crt_base+address;
            }
        }
        for(item=0;item<sizeof(known)/sizeof(known[0]);++item)if(!known[item].address)fail("missing canonical locale export");module=crt_base;
    }
    if(module!=crt_base)fail("locale provider identity changed");
    for(item=0;item<sizeof(known)/sizeof(known[0]);++item)if(!strcmp(wanted,known[item].name))return known[item].address;
    fail("unknown locale export request");return 0;
}

static void locale_vector(const char *phase,const char *kind,const char *baseline,DWORD address,DWORD count)
{
    FILE *input,*output;DWORD root,expected_count,index,pointer,length,end;BYTE *expected,*actual;char name[128];
    input=fopen(baseline,"rb");if(!input)fail("locale vector baseline");
    if(fread(&root,4,1,input)!=1 || fread(&expected_count,4,1,input)!=1 || root!=address || expected_count!=count || count>4096)fail("locale vector root/count changed");
    sprintf(name,"locale-%s-%s.bin",phase,kind);output=fopen(name,"wb");if(!output)fail("locale vector output");
    fwrite(&address,4,1,output);fwrite(&count,4,1,output);
    for(index=0;index<count;++index) {
        DWORD original;
        if(fread(&original,4,1,input)!=1 || fread(&length,4,1,input)!=1 || !length || length>65536)fail("locale vector length");
        expected=(BYTE *)malloc(length);actual=(BYTE *)malloc(length);if(!expected || !actual)fail("locale vector observer allocation");
        if(fread(expected,1,length,input)!=length)fail("locale vector baseline truncated");
        readmem(address+index*4,&pointer,4);readmem(pointer,actual,length);
        if(pointer!=original || memcmp(actual,expected,length) || actual[length-1])fail("locale vector content/identity changed");
        fwrite(&pointer,4,1,output);fwrite(&length,4,1,output);fwrite(actual,1,length,output);free(actual);free(expected);
    }
    readmem(address+count*4,&end,4);if(end || fgetc(input)!=EOF)fail("locale vector terminator/trailing bytes");
    if(fclose(input) || fclose(output))fail("locale vector close");
}

static void locale_region(const char *phase,const char *kind,DWORD address,DWORD bytes)
{
    MEMORY_BASIC_INFORMATION region;DWORD received;char name[128];
    memset(&region,0,sizeof(region));
    if(address) {
        received=VirtualQueryEx(child,(void *)address,&region,sizeof(region));
        if(received!=sizeof(region) || region.State!=MEM_COMMIT || address<(DWORD)region.BaseAddress || address+bytes<address || address+bytes>(DWORD)region.BaseAddress+region.RegionSize)fail("locale observed mapped region");
    }
    sprintf(name,"locale-%s-%s-region.bin",phase,kind);save_blob(name,&region,sizeof(region));
    /* A mapped region is not a malloc object extent. */
}

static void locale_snapshot(const char *phase,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD values[18],teb,index,ptd,slot,errors[3],root[2],count;BYTE raw_teb[4096],raw_ptd[16],mb[257],ctype[514],old[514],code[9];
    LDT_ENTRY selector;char name[128],result[256];DWORD length=0;
    if(!GetThreadSelectorEntry(thread,context->SegFs,&selector))fail("locale TEB selector");
    teb=selector.BaseLow|((DWORD)selector.HighWord.Bytes.BaseMid<<16)|((DWORD)selector.HighWord.Bytes.BaseHi<<24);
    readmem(teb,raw_teb,sizeof(raw_teb));
    if(*(DWORD *)(raw_teb+0x18)!=teb || *(DWORD *)(raw_teb+0x24)!=tid)fail("locale TEB identity");
    readmem(crt_base+0x6436,code,9);if(memcmp(code,"\xe8\x88\xb1\xff\xff\x83\xc0\x08\xc3",9))fail("locale errno instruction identity");
    readmem(crt_base+0x642d,code,9);if(memcmp(code,"\xe8\x91\xb1\xff\xff\x83\xc0\x0c\xc3",9))fail("locale doserrno instruction identity");
    readmem(crt_base+0x15ce,&slot,4);if(slot!=crt_base+0x3d004)fail("locale getptd TLS literal");readmem(slot,&index,4);
    if(index>=64)fail("locale TLS index");memcpy(&ptd,raw_teb+0xe10+4*index,4);if(!ptd)fail("locale actual PTD");readmem(ptd,raw_ptd,sizeof(raw_ptd));
    memcpy(errors,raw_ptd+8,8);memcpy(errors+2,raw_teb+0x34,4);
    sprintf(name,"locale-%s-teb.bin",phase);save_blob(name,raw_teb,sizeof(raw_teb));sprintf(name,"locale-%s-ptd.bin",phase);save_blob(name,raw_ptd,sizeof(raw_ptd));
    sprintf(name,"locale-%s-errors.bin",phase);save_blob(name,errors,sizeof(errors));sprintf(name,"locale-%s-fp.bin",phase);save_blob(name,&context->FloatSave,sizeof(context->FloatSave));
    fprintf(events,"locale-state phase=%s teb=%08lx tid=%lu tls-index=%lu ptd=%08lx errno=%ld doserrno=%lu LastError=%lu\n",phase,teb,tid,index,ptd,(LONG)errors[0],errors[1],errors[2]);
    memset(values,0,sizeof(values));values[0]=crt_base;
    readmem(locale_export("__mb_cur_max"),values+1,4);readmem(locale_export("__lc_codepage"),values+2,4);readmem(locale_export("__lc_handle"),values+3,24);
    readmem(crt_base+0x3f97c,values+9,4);readmem(crt_base+0x3f1c0,values+10,4);readmem(locale_export("_pctype"),values+11,4);values[12]=locale_export("_ctype");
    readmem(locale_export("__argc"),values+13,4);readmem(locale_export("__argv"),values+14,4);readmem(locale_export("_environ"),values+15,4);
    if(!locale_old_pctype)locale_old_pctype=values[11];values[16]=locale_old_pctype;values[17]=locale_result;
    sprintf(name,"locale-%s-words.bin",phase);save_blob(name,values,sizeof(values));
    readmem(locale_export("_mbctype"),mb,sizeof(mb));readmem(values[11]-2,ctype,sizeof(ctype));readmem(locale_old_pctype-2,old,sizeof(old));
    sprintf(name,"locale-%s-mbctype.bin",phase);save_blob(name,mb,sizeof(mb));sprintf(name,"locale-%s-ctype.bin",phase);save_blob(name,ctype,sizeof(ctype));sprintf(name,"locale-%s-old-ctype.bin",phase);save_blob(name,old,sizeof(old));
    if(locale_result){remote_string(locale_result,result,sizeof(result));length=strlen(result)+1;}
    sprintf(name,"locale-%s-result.bin",phase);save_blob(name,result,length);
    locale_region(phase,"ctype",values[11]-2,sizeof(ctype));locale_region(phase,"old-ctype",locale_old_pctype-2,sizeof(old));locale_region(phase,"result",locale_result,length);
    locale_vector(phase,"CRT-argv","original-crt-argv.bin",values[14],values[13]);
    {FILE *file=fopen("original-crt-environment.bin","rb");if(!file || fread(root,4,2,file)!=2 || fclose(file))fail("locale environment baseline count");count=root[1];}
    locale_vector(phase,"CRT-environment","original-crt-environment.bin",values[15],count);
    {FILE *file=fopen("invoke-entry-argv.bin","rb");if(!file || fread(root,4,2,file)!=2 || fclose(file))fail("locale Invoke baseline root");}
    locale_vector(phase,"Invoke-argv","invoke-entry-argv.bin",root[0],root[1]);
    sprintf(name,"locale-%s",phase);save_frontend_state(name);
}

static void prepare_isolated_locale_observer(void)
{
    FILE *marker=fopen("isolated-locale-observers.txt","r");DWORD actual;BYTE first;
    if(!marker)return;
    fclose(marker);if(baseline || !checksum_tables_probe)fail("isolated locale capture role");
    isolated_locale_probe=1;scanner_tables_probe=1;
    if(locale_export("setlocale")!=locale_entry_address)fail("isolated locale entry identity");
    mb_entry_address=locale_export("_setmbcp");readmem(base+0x5d148,&actual,4);
    if(actual!=mb_entry_address)fail("actual _setmbcp IAT identity");
    readmem(mb_entry_address,&first,1);if(first!=0x83)fail("canonical _setmbcp entry byte");
    mb_entry_saved=first;patch(mb_entry_address,0xcc);mb_entry_active=1;
    fprintf(events,"isolated-locale-observer setlocale-iat=0005d120 setmbcp-iat=0005d148 exports=14 no-query-calls=1 no-seeds=1\n");
}
