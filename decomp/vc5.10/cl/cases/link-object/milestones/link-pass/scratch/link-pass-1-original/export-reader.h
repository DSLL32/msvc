/* Original-debugger analysis only: fresh bounded export-directory reads.
 * The capture receipt separately verifies the actual loaded hFile SHA256.
 * No result or mutable target state is cached. */
#ifndef CL_EXPORT_READER_H
#define CL_EXPORT_READER_H

typedef int (*ExportRead)(void *, DWORD, void *, DWORD);

static DWORD export_word(const BYTE *p)
{
    DWORD value;
    memcpy(&value,p,4);
    return value;
}
static WORD export_short(const BYTE *p)
{
    WORD value;
    memcpy(&value,p,2);
    return value;
}
static int export_range(DWORD start,DWORD size,DWORD address,DWORD bytes)
{
    return address>=start && address-start<=size && bytes<=size-(address-start);
}
static const BYTE *export_part(const BYTE *data,DWORD start,DWORD size,DWORD rva,DWORD bytes)
{
    return export_range(start,size,rva,bytes) ? data+(rva-start) : NULL;
}
static const char *export_string(const BYTE *data,DWORD start,DWORD size,DWORD rva)
{
    const BYTE *p=export_part(data,start,size,rva,1);
    return p && memchr(p,0,size-(rva-start)) ? (const char *)p : NULL;
}

/* 1=found, 0=missing, negative=unsupported/malformed/read failure.
 * A forwarded export retains the old reader's address-of-forwarder result;
 * this observer does not resolve or invoke forwarders. */
static int export_lookup(ExportRead read,void *context,DWORD base,const char *wanted,DWORD *result)
{
    BYTE dos[64],nt[248],*data;
    DWORD offset,image,start,size,stamp,entry,preferred,count,functions;
    DWORD names_rva,ordinals_rva,functions_rva,i,function_rva;
    const BYTE *names,*ordinals,*addresses;
    const char *module,*name,*expected;
    WORD ordinal;
    int answer=0;
    *result=0;
    if (!read(context,base,dos,sizeof(dos))) return -5;
    offset=export_word(dos+60);
    if (export_short(dos)!=0x5a4d || offset<64 || offset>0x100000 ||
        base>0xffffffffUL-offset || base+offset>0xffffffffUL-sizeof(nt)) return -1;
    if (!read(context,base+offset,nt,sizeof(nt))) return -5;
    if (export_word(nt)!=0x4550 || export_short(nt+4)!=0x14c ||
        export_short(nt+20)!=224 || export_short(nt+24)!=0x10b ||
        export_word(nt+24+92)<1) return -1;
    image=export_word(nt+24+56);start=export_word(nt+24+96);size=export_word(nt+24+100);
    stamp=export_word(nt+8);entry=export_word(nt+24+16);preferred=export_word(nt+24+28);
    if (!image || image>0x1000000 || base>0xffffffffUL-image ||
        offset>image || sizeof(nt)>image-offset || size<40 || size>0x100000 ||
        start>=image || size>image-start) return -1;
    /* Exact parsed PE identity of the two hash-bound loaded providers used by
     * this case. Their complete export arrays/names fit these actual extents;
     * other layouts fail closed rather than extending the read speculatively. */
    if (stamp==0x345ebc2cUL && image==0x48000 && entry==0x1000 &&
        preferred==0x78000000UL && start==0x38200 && size==0x3c0e)
        expected="MSVCRT.dll";
    else if (stamp==0x335fbd28UL && image==0x9b000 && entry==0x1202a &&
        preferred==0x10600000UL && start==0x5b560 && size==0x74)
        expected="c1n32_l.dll";
    else return -2;
    data=(BYTE *)malloc(size);
    if (!data) return -4;
    if (!read(context,base+start,data,size)) { free(data); return -5; }
    module=export_string(data,start,size,export_word(data+12));
    functions=export_word(data+20);count=export_word(data+24);
    functions_rva=export_word(data+28);names_rva=export_word(data+32);ordinals_rva=export_word(data+36);
    if (!module || strcmp(module,expected) || !functions || functions>65536 || count>functions) { answer=-3; goto done; }
    addresses=export_part(data,start,size,functions_rva,functions*4);
    names=export_part(data,start,size,names_rva,count*4);
    ordinals=export_part(data,start,size,ordinals_rva,count*2);
    if (!addresses || !names || !ordinals) { answer=-3; goto done; }
    /* Validate every entry, including those after the requested name. This
     * avoids accepting a valid prefix of a malformed directory. */
    for (i=0;i<functions;++i) {
        function_rva=export_word(addresses+4*i);
        if (function_rva>=image) { answer=-3; goto done; }
        if (function_rva && export_range(start,size,function_rva,1) &&
            !export_string(data,start,size,function_rva)) { answer=-3; goto done; }
    }
    for (i=0;i<count;++i) {
        name=export_string(data,start,size,export_word(names+4*i));
        ordinal=export_short(ordinals+2*i);
        if (!name || !*name || ordinal>=functions) { answer=-3; goto done; }
        function_rva=export_word(addresses+4*ordinal);
        if (!function_rva) { answer=-3; goto done; }
        if (!strcmp(name,wanted)) {
            if (answer) { answer=-3; goto done; }
            *result=base+function_rva;answer=1;
        }
    }
 done:
    free(data);
    if (answer!=1) *result=0;
    return answer;
}
#endif
