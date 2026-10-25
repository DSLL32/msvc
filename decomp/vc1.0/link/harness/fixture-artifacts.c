/* Native fixture checks for LINK's serialized COFF, PE, archive and MAP clocks.
 * This is test support, outside the reconstructed application graph. */
#define RECON_SHIM_IMPL
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <recon-output.h>
#include <recon.h>

typedef struct { uint32_t offset, size; } ClockField;
typedef struct {
    unsigned char *data;
    uint32_t size, fields_count, expected_count;
    ClockField fields[256], expected[256];
    const char *kind;
} Artifact;

static int bounds(Artifact *a, uint32_t p, uint32_t n)
{ return p <= a->size && n <= a->size-p; }
static uint32_t word(Artifact *a, uint32_t p)
{ return bounds(a,p,2) ? a->data[p] | (uint32_t)a->data[p+1]<<8 : 0xffffffff; }
static uint32_t dword(Artifact *a, uint32_t p)
{ return bounds(a,p,4) ? word(a,p) | word(a,p+2)<<16 : 0xffffffff; }
static int field(Artifact *a, uint32_t p, uint32_t n)
{
    if (!bounds(a,p,n) || a->fields_count==256) return 0;
    a->fields[a->fields_count].offset=p;
    a->fields[a->fields_count++].size=n;
    return 1;
}
static uint32_t raw(Artifact *a, uint32_t optional, uint32_t rva)
{
    uint32_t pe=optional-24, table=optional+word(a,pe+20), i;
    for (i=0;i<word(a,pe+6);++i) {
        uint32_t p=table+40*i, start=dword(a,p+12), bytes=dword(a,p+16);
        if (!bounds(a,p,40)) return 0xffffffff;
        if (rva>=start && rva-start<bytes && bounds(a,dword(a,p+20),bytes))
            return dword(a,p+20)+rva-start;
    }
    return 0xffffffff;
}
static uint32_t image_checksum(Artifact *a, uint32_t size, uint32_t checksum)
{
    uint64_t total=0; uint32_t p;
    for (p=0;p<size;p+=2) {
        uint32_t value=a->data[p];
        if (p+1<size) value|=(uint32_t)a->data[p+1]<<8;
        if (p<checksum || p>=checksum+4) total+=value;
    }
    while (total>>16) total=(total&65535)+(total>>16);
    return (uint32_t)total+size;
}
static int resource_directory(Artifact *a,uint32_t root,uint32_t size,
                              uint32_t relative,unsigned depth)
{
    uint32_t p=root+relative,count,i;
    if (depth>8 || relative>size || size-relative<16 || !bounds(a,p,16)) return 0;
    if (!field(a,p+4,4)) return 0;
    count=word(a,p+12)+word(a,p+14);
    if (count>(size-relative-16)/8 || !bounds(a,p+16,count*8)) return 0;
    for(i=0;i<count;++i) {
        uint32_t next=dword(a,p+20+i*8);
        if(next&0x80000000) {
            if(!resource_directory(a,root,size,next&0x7fffffff,depth+1))return 0;
        } else if(next>size || size-next<16)return 0;
    }
    return 1;
}
static int pe(Artifact *a)
{
    uint32_t p,optional,table,sections,i,debug,size,export_rva,resource;
    int prepack=!strcmp(a->kind,"debug-prepack");
    int kind_debug=!strcmp(a->kind,"debug-pe")||prepack;
    if(!bounds(a,0,64)||memcmp(a->data,"MZ",2))return 0;
    p=dword(a,60);optional=p+24;
    if(!bounds(a,p,24)||memcmp(a->data+p,"PE\0\0",4)||word(a,optional)!=0x10b)return 0;
    if(word(a,p+4)!=(!strcmp(a->kind,"m68k-pe")||!strcmp(a->kind,"mac-debug-pe")?0x268:0x14c))return 0;
    if(!bounds(a,optional,word(a,p+20)))return 0;
    if(!strcmp(a->kind,"compact-release-defect")) {
        return a->size==2097152 && dword(a,optional+64)==image_checksum(a,3072,optional+64)
            && dword(a,optional+64)!=image_checksum(a,a->size,optional+64);
    }
    if((kind_debug&&!prepack)||!strcmp(a->kind,"edited-pe")||!strcmp(a->kind,"section-release"))
        if(dword(a,optional+64)!=image_checksum(a,a->size,optional+64))return 0;
    if(!strcmp(a->kind,"section-release"))return 1;
    if(!field(a,p+8,4))return 0;
    table=optional+word(a,p+20);sections=word(a,p+6);
    for(i=0;i<sections;++i)if(!bounds(a,table+40*i,40))return 0;
    if(!strcmp(a->kind,"pe")||kind_debug) {
        export_rva=dword(a,optional+96);export_rva=raw(a,optional,export_rva);
        if(!bounds(a,export_rva,40)||!field(a,export_rva+4,4))return 0;
    }
    if(kind_debug||!strcmp(a->kind,"resource-pe")) {
        resource=raw(a,optional,dword(a,optional+112));size=dword(a,optional+116);
        if(!size||!bounds(a,resource,size)||!resource_directory(a,resource,size,0,0))return 0;
    }
    if(kind_debug||!strcmp(a->kind,"mac-debug-pe")||!strcmp(a->kind,"pe-coff-fixup")) {
        size=dword(a,optional+148);debug=raw(a,optional,dword(a,optional+144));
        if(!size||size%28||!bounds(a,debug,size))return 0;
        for(i=0;i<size;i+=28) {
            if(dword(a,debug+i+4)!=dword(a,p+8)||!field(a,debug+i+4,4))return 0;
            if(!bounds(a,dword(a,debug+i+24),dword(a,debug+i+16)))return 0;
        }
        if(!strcmp(a->kind,"mac-debug-pe")) {
            int found=0;
            for(i=0;i<sections;++i)if(!memcmp(a->data+table+40*i,".mscv\0\0\0",8)) {
                uint32_t q=dword(a,table+40*i+20),big;
                if(!bounds(a,q,6)||a->data[q]||a->data[q+1]!=2)return 0;
                big=(uint32_t)a->data[q+2]<<24|(uint32_t)a->data[q+3]<<16|
                    (uint32_t)a->data[q+4]<<8|a->data[q+5];
                if(big!=dword(a,p+8)||!field(a,q+2,4))return 0;
                found=1;
            }
            if(!found)return 0;
        }
    }
    return 1;
}
static int coff(Artifact *a,uint32_t base,uint32_t size,int exports)
{
    uint32_t machine=word(a,base),table=base+20+word(a,base+16),i,count=word(a,base+2);
    if(machine!=0x14c&&machine!=0x14d&&machine!=0x162)return 0;
    if(size<20||table<base||table-base>size||count>(size-(table-base))/40)return 0;
    if(!field(a,base+4,4))return 0;
    if(exports)for(i=0;i<count;++i) {
        uint32_t p=table+i*40;
        if(!memcmp(a->data+p,".edata\0\0",8)) {
            uint32_t q=base+dword(a,p+20);
            if(dword(a,p+16)<40||!bounds(a,q,40)||!field(a,q+4,4))return 0;
        }
    }
    return 1;
}
static int archive(Artifact *a)
{
    uint32_t p=8;
    if(!bounds(a,0,8)||memcmp(a->data,"!<arch>\n",8))return 0;
    while(p<a->size) {
        uint32_t i,n=0,start,width=12;int nonzero=0;
        for(i=p;i<a->size;++i)if(a->data[i]){nonzero=1;break;}
        if(!nonzero)return 1;
        if(!bounds(a,p,60)||memcmp(a->data+p+58,"`\n",2))return 0;
        for(i=48;i<58;++i)if(a->data[p+i]>='0'&&a->data[p+i]<='9')n=n*10+a->data[p+i]-'0';
            else if(a->data[p+i]!=' '&&a->data[p+i])return 0;
        start=p+60;if(!bounds(a,start,n))return 0;
        for(i=0;i<a->expected_count;++i)if(a->expected[i].offset==p+16)width=a->expected[i].size;
        if(width==10) {
            for(i=0;i<10;++i)if(a->data[p+16+i]<'0'||a->data[p+16+i]>'9')return 0;
            if(a->data[p+26]!='0'||a->data[p+27]!=' ')return 0;
        } else if(width!=12)return 0;
        if(!field(a,p+16,width))return 0;
        if(strcmp(a->kind,"archive-members")&&n>=20 &&
           (word(a,start)==0x14c||word(a,start)==0x14d||word(a,start)==0x162))
            if(!coff(a,start,n,1))return 0;
        p=start+n+(n&1);
    }
    return p==a->size;
}
static int map(Artifact *a)
{
    uint32_t p,found=0;
    for(p=0;p+49<a->size;++p)if((!p||a->data[p-1]=='\n')&&!memcmp(a->data+p," Timestamp is ",14)) {
        uint32_t i;
        for(i=0;i<8;++i)if(!((a->data[p+14+i]>='0'&&a->data[p+14+i]<='9')||(a->data[p+14+i]>='a'&&a->data[p+14+i]<='f')))return 0;
        if(a->data[p+22]!=' '||a->data[p+23]!='('||a->data[p+48]!=')')return 0;
        if(!field(a,p+14,8)||!field(a,p+24,24))return 0;
        ++found;
    }
    return found==1;
}
static int compare_field(const void *x,const void *y)
{
    const ClockField *a=x,*b=y;
    return a->offset<b->offset?-1:a->offset>b->offset?1:a->size<b->size?-1:a->size>b->size;
}
static int parse(Artifact *a)
{
    if(!strcmp(a->kind,"raw"))return 1;
    if(!strcmp(a->kind,"empty-resource-temporary"))return !a->size;
    if(!strcmp(a->kind,"map"))return map(a);
    if(!strncmp(a->kind,"archive",7))return archive(a);
    if(!strcmp(a->kind,"coff"))return coff(a,0,a->size,1);
    if(!strcmp(a->kind,"i386-rom")) {
        uint32_t count=word(a,2),optional=word(a,16),i;
        if(word(a,0)!=0x14c||!count||optional!=224||word(a,20)!=0x10b||!bounds(a,20+optional,count*40))return 0;
        for(i=0;i<count;++i)if(!bounds(a,dword(a,20+optional+40*i+20),dword(a,20+optional+40*i+16)))return 0;
        return field(a,4,4);
    }
    return pe(a);
}
static int read32(FILE *f,uint32_t *v)
{
    unsigned char b[4];if(fread(b,1,4,f)!=4)return 0;
    *v=b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;return 1;
}
static int read_string(FILE *f,char *s,size_t cap)
{
    uint32_t n;if(!read32(f,&n)||n>=cap||fread(s,1,n,f)!=n)return 0;
    s[n]=0;return 1;
}
int link_check_artifacts(void)
{
    FILE *contract=fopen(".link-artifacts","rb");uint32_t count,index;char magic[8];
    if(!contract)return 0;
    if(fread(magic,1,8,contract)!=8||memcmp(magic,"LART0001",8)||!read32(contract,&count)){fclose(contract);return 0;}
    for(index=0;index<count;++index) {
        Artifact a;char path[256],kind[64];FILE *file;uint32_t i,flags;int okay;
        memset(&a,0,sizeof(a));
        if(!read_string(contract,path,sizeof(path))||!read_string(contract,kind,sizeof(kind))||
           !read32(contract,&a.size)||!read32(contract,&flags)||!read32(contract,&a.expected_count)||a.expected_count>256)goto fail;
        for(i=0;i<a.expected_count;++i)if(!read32(contract,&a.expected[i].offset)||!read32(contract,&a.expected[i].size))goto fail;
        file=fopen(path,"rb");if(!file)goto fail;
        a.data=malloc(a.size?a.size:1);a.kind=kind;
        if(!a.data){fclose(file);goto fail;}
        okay=fread(a.data,1,a.size,file)==a.size&&fgetc(file)==EOF;fclose(file);
        if(okay)okay=parse(&a);
        /* Fixed clock input still validates structure; comparison keeps raw clocks. */
        if(okay) {
            qsort(a.fields,a.fields_count,sizeof(ClockField),compare_field);
            qsort(a.expected,a.expected_count,sizeof(ClockField),compare_field);
            okay=a.fields_count==a.expected_count&&!memcmp(a.fields,a.expected,a.fields_count*sizeof(ClockField));
        }
        free(a.data);
        if(!okay){fprintf(stderr,"LINK artifact contract failed: %s (%s)\n",path,kind);goto fail;}
    }
    if(fgetc(contract)!=EOF)goto fail;
    fclose(contract);return 1;
fail:
    fclose(contract);return 0;
}

/* Pure bounded control for the migrated checksum oracle, independent of target
 * execution. Per-word folding is retained here as a differently ordered sum. */
int link_fixture_checksum_test(void)
{
    unsigned char bytes[257];Artifact a;uint32_t n,i,seed=17;
    memset(&a,0,sizeof(a));a.data=bytes;
    for(i=0;i<sizeof(bytes);++i){seed=seed*1664525+1013904223;bytes[i]=(unsigned char)(seed>>24);}
    for(n=0;n<=sizeof(bytes);++n) {
        uint32_t sum=0;
        for(i=0;i<n;i+=2) {
            uint32_t v=bytes[i];if(i+1<n)v|=(uint32_t)bytes[i+1]<<8;
            if(i<64||i>=68){sum+=v;sum=(sum&65535)+(sum>>16);}
        }
        sum=(sum&65535)+(sum>>16);
        if(image_checksum(&a,n,64)!=sum+n)return 0;
    }
    return 1;
}

static unsigned char *load_artifact(const char *path,uint32_t *size)
{
    FILE *file=fopen(path,"rb");long length;unsigned char *data;
    if(!file)return NULL;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<0||fseek(file,0,SEEK_SET)) {fclose(file);return NULL;}
    data=malloc(length?length:1);
    if(!data||fread(data,1,length,file)!=(size_t)length||fgetc(file)!=EOF) {free(data);fclose(file);return NULL;}
    fclose(file);*size=(uint32_t)length;return data;
}
static int check_process_clocks(unsigned char *data,uint32_t size,const uint32_t (*clocks)[3],uint32_t count,int packed)
{
    Artifact a;uint32_t i;int okay;
    if(count>256)return 0;
    memset(&a,0,sizeof(a));a.data=data;a.size=size;a.kind=packed?"debug-pe":"debug-prepack";
    a.expected_count=count;
    for(i=0;i<count;++i){a.expected[i].offset=clocks[i][packed?1:0];a.expected[i].size=clocks[i][2];}
    okay=parse(&a);
    qsort(a.fields,a.fields_count,sizeof(ClockField),compare_field);
    qsort(a.expected,a.expected_count,sizeof(ClockField),compare_field);
    return okay&&a.fields_count==count&&!memcmp(a.fields,a.expected,count*sizeof(ClockField));
}
/* Explicit CVPACK test double. The complete actual prepack input must match
 * its independent original capture before the captured packed response is used.
 * Prepack's original sentinel checksum stays strict; packed checksum is validated
 * independently, then recomputed after copying only proven actual clock fields. */
int link_fixture_cvpack(const char *target,const char *before_path,const char *after_path,
                        const uint32_t (*clocks)[3],uint32_t count)
{
    unsigned char *actual=NULL,*before=NULL,*after=NULL;uint32_t an=0,bn=0,cn=0,i,j,optional,checksum;FILE *file;int okay=0;
    actual=load_artifact(target,&an);before=load_artifact(before_path,&bn);after=load_artifact(after_path,&cn);
    if(!actual||!before||!after||an!=bn||!check_process_clocks(actual,an,clocks,count,0)||
       !check_process_clocks(before,bn,clocks,count,0)||!check_process_clocks(after,cn,clocks,count,1))goto done;
    /* Transfer the actual clock input before clearing the comparison copies. */
    for(i=0;i<count;++i) {
        uint32_t p=clocks[i][0],q=clocks[i][1],n=clocks[i][2];
        memcpy(after+q,actual+p,n);
        memset(actual+p,0,n);memset(before+p,0,n);
    }
    if(memcmp(actual,before,an))goto done;
    optional=after[60]|(uint32_t)after[61]<<8|(uint32_t)after[62]<<16|(uint32_t)after[63]<<24;
    optional+=24;
    {
        Artifact a;memset(&a,0,sizeof(a));a.data=after;a.size=cn;
        checksum=image_checksum(&a,cn,optional+64);
    }
    for(j=0;j<4;++j)after[optional+64+j]=(unsigned char)(checksum>>(8*j));
    file=fopen(target,"wb");if(!file)goto done;
    okay=fwrite(after,1,cn,file)==cn;
    if(fclose(file))okay=0;
done:
    free(actual);free(before);free(after);return okay;
}
/* The two registered LINK_REPRO copy responses have identical input/output
 * bytes. Verify the whole live source against its independent captured input,
 * require the destination absent, then replay precisely that file effect. */
int link_fixture_copy(const char *source,const char *reference,const char *destination)
{
    unsigned char *actual=NULL,*expected=NULL;uint32_t an=0,en=0;FILE *file;int okay=0;struct stat status;
    actual=load_artifact(source,&an);expected=load_artifact(reference,&en);
    if(!actual||!expected||an!=en||memcmp(actual,expected,an))goto done;
    if(!lstat(destination,&status)||errno!=ENOENT)goto done;
    file=fopen(destination,"wb");if(!file)goto done;
    okay=fwrite(expected,1,en,file)==en;if(fclose(file))okay=0;
done:
    free(actual);free(expected);return okay;
}

/* Runs after the actual target/children exit; never reads stale target globals. */
int link_fixture_output(const ReconOutputContext *context)
{
    char cwd[4096];
    return context&&context->struct_size==sizeof(*context)&&context->executor_pid&&
        context->terminal_kind==RECON_OUTPUT_EXIT&&context->workspace_cwd&&
        getcwd(cwd,sizeof(cwd))&&!strcmp(cwd,context->workspace_cwd)&&link_check_artifacts();
}
