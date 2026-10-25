/* Original-only tooling. Merge an ASCII target overlay without changing the
 * broker's environment. Sets are NAME=VALUE, removals NAME, double-NUL end. */
#ifndef RECON_ORIGINAL_ENVIRONMENT_H
#define RECON_ORIGINAL_ENVIRONMENT_H
#include <stdlib.h>
#include <string.h>
#define ORIGINAL_ENVIRONMENT_LIMIT 32767U

static size_t original_environment_name_size(const char *entry)
{
    const char *equal=strchr(entry+(*entry=='='), '=');
    return equal ? (size_t)(equal-entry) : strlen(entry);
}
static int original_environment_names(const char *a,const char *b)
{
    size_t i,an=original_environment_name_size(a),bn=original_environment_name_size(b);
    for(i=0;i<an && i<bn;++i) {
        unsigned char ac=(unsigned char)a[i],bc=(unsigned char)b[i];
        if(ac>='a' && ac<='z')ac-='a'-'A';
        if(bc>='a' && bc<='z')bc-='a'-'A';
        if(ac!=bc)return ac<bc?-1:1;
    }
    return an==bn?0:an<bn?-1:1;
}
static int original_environment_compare(const void *a,const void *b)
{
    return original_environment_names(*(const char *const *)a,*(const char *const *)b);
}
static int original_environment_valid(const char *bytes,size_t size,int overlay)
{
    size_t at=0,n,end;
    if(size<2 || size>ORIGINAL_ENVIRONMENT_LIMIT || bytes[size-1] || bytes[size-2])return 0;
    for(n=0;n<size;++n)if((unsigned char)bytes[n]>127)return 0;
    while(at<size-1 && bytes[at]) {
        end=at;while(end<size && bytes[end])++end;
        if(end>=size || (overlay && bytes[at]=='=') ||
           (!overlay && !strchr(bytes+at+(bytes[at]=='='),'=')))return 0;
        at=end+1;
    }
    /* An empty block has two terminators; populated blocks have one extra. */
    return at==(size==2 && !bytes[0]?0:size-1);
}
static char *original_environment_merge(const char *base,size_t base_size,
                                        const char *overlay,size_t overlay_size)
{
    const char *p,*q,**entries;
    char *result,*out;
    size_t count=0,n,total=1;
    int replaced;
    if(!original_environment_valid(base,base_size,0) ||
       !original_environment_valid(overlay,overlay_size,1))return NULL;
    /* Duplicate case-folded overlay names would make the input ambiguous. */
    for(p=overlay;*p;p+=strlen(p)+1)
        for(q=p+strlen(p)+1;*q;q+=strlen(q)+1)
            if(!original_environment_names(p,q))return NULL;
    entries=malloc((base_size+overlay_size)*sizeof(*entries));
    if(!entries)return NULL;
    for(p=base;*p;p+=strlen(p)+1) {
        replaced=0;
        for(q=overlay;*q;q+=strlen(q)+1)
            if(!original_environment_names(p,q)){replaced=1;break;}
        if(!replaced)entries[count++]=p;
    }
    for(p=overlay;*p;p+=strlen(p)+1)if(strchr(p,'='))entries[count++]=p;
    qsort(entries,count,sizeof(*entries),original_environment_compare);
    for(n=0;n<count;++n) {
        if(n && !original_environment_names(entries[n-1],entries[n]))goto invalid;
        total+=strlen(entries[n])+1;
        if(total>ORIGINAL_ENVIRONMENT_LIMIT)goto invalid;
    }
    if(!count)total=2;
    result=malloc(total);
    if(!result)goto invalid;
    out=result;
    for(n=0;n<count;++n) {
        size_t length=strlen(entries[n])+1;
        memcpy(out,entries[n],length);out+=length;
    }
    *out=0;if(!count)out[1]=0;
    free(entries);
    return result;
invalid:
    free(entries);
    return NULL;
}
#endif
