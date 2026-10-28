#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <float.h>
#include <setjmp.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <limits.h>
#include <malloc.h>
#include <pthread.h>
#include <recon.h>

/* BC4.0 BCC32 VA d4026..d403b and EXCEPT.C: process-global pointer swap.
 * Native defaults preserve distinct initial/GP identities, but do not implement
 * unwinding, register diagnostics, signals or Windows exception dispatch. */
#ifdef RECON_BORLAND_400_USER_HANDLER
static ULONG WINAPI borland_initial_user_handler(EXCEPTION_RECORD *record,void *frame,CONTEXT *context,void *dispatch)
{
    (void)record;(void)frame;(void)context;(void)dispatch;
    recon_unsupported("Borland400 initial exception handler invocation");
    __builtin_unreachable();
}
static ULONG WINAPI borland_gp_user_handler(EXCEPTION_RECORD *record,void *frame,CONTEXT *context,void *dispatch)
{
    (void)record;(void)frame;(void)context;(void)dispatch;
    recon_unsupported("Borland400 GP exception handler invocation");
    __builtin_unreachable();
}
static ReconBorland400UserHandler borland_user_handler=borland_initial_user_handler;
#endif
ReconBorland400UserHandler __cdecl recon_borland_400_SetUserHandler(ReconBorland400UserHandler handler)
{
#ifdef RECON_BORLAND_400_USER_HANDLER
    ReconBorland400UserHandler previous=borland_user_handler;
    borland_user_handler=handler;
    return previous;
#else
    (void)handler;recon_unsupported("Borland400 user handler requires explicit opt-in");
    __builtin_unreachable();
#endif
}
/* Host startup boundary for the actual later priority100 callback at db319:
 * its entire effect is _SetUserHandler(GP default), before App main. */
void recon_borland_400_initialize_gp_handler(void)
{
#ifdef RECON_BORLAND_400_USER_HANDLER
    recon_borland_400_SetUserHandler(borland_gp_user_handler);
#else
    recon_unsupported("Borland400 GP startup requires explicit opt-in");
#endif
}

/* Compiler-generated abstract vtables may reference this boundary during
 * otherwise inlined construction. Invocation remains unsupported; this does
 * not model an original CRT purecall diagnostic or successful return. */
#ifdef __cplusplus
extern "C"
#endif
__attribute__((noreturn)) void __cxa_pure_virtual(void)
{
    recon_unsupported("pure virtual invocation");
    __builtin_unreachable();
}

/* Qualified MSPDB50 raw reflected CRC signature. The polynomial generates
 * every canonical table entry; no provider table or private state is copied. */
__attribute__((weak)) unsigned long __cdecl recon_mspdb_SigForPbCb(
    const void *bytes,long byte_count,unsigned long signature)
{
    const unsigned char *cursor=bytes;long remaining=byte_count;unsigned bit;
    if(byte_count<0 || (byte_count && (!bytes ||
       (uintptr_t)bytes>UINTPTR_MAX-(unsigned long)byte_count)))
        recon_unsupported("unreviewed MSPDB signature input/span");
    /* Negative counts stay denied, including the canonical DEC/JS INT_MIN
     * wrap case. Zero reads nothing. By-value parameter storage is private;
     * the original caller discards its slots and consumes only the result. */
    while(remaining-- >0) {
        signature^=*cursor++;
        for(bit=0;bit<8;++bit)
            signature=(signature>>1)^((signature&1)?0xedb88320UL:0);
    }
    return signature;
}

/* Cold MSPDB public: caller/import and published header establish typing;
 * no truncation, destination access or successful provider behavior is granted. */
void __cdecl recon_mspdb_TruncStFromSz(char *destination,const char *source,size_t count)
{
    (void)destination;(void)source;(void)count;
    recon_unsupported("unreviewed MSPDB TruncStFromSz");
}

int __cdecl recon_mspdb_NameMap_open(struct PDB *database,int writable,struct NameMap **result)
{
    (void)database;(void)writable;(void)result;
    recon_unsupported("unreviewed MSPDB NameMap::open");return 0;
}

/* Cold VC5 imports: linkability only. Neither implicit x87 operands nor PDB
 * input/output pointers are inspected; successful provider behavior is absent. */
void __cdecl recon_adj_fdiv_r(void)
{
    recon_unsupported("unreviewed MSVCRT _adj_fdiv_r");
}
int __cdecl recon_mspdb_PDB_OpenValidate(char *path,char *search,char *mode,
    DWORD signature,DWORD age,LONG *error,char *error_text,struct PDB **result)
{
    (void)path;(void)search;(void)mode;(void)signature;(void)age;
    (void)error;(void)error_text;(void)result;
    recon_unsupported("unreviewed MSPDB PDB::OpenValidate");return 0;
}

/* Distinct Watcom CRT boundaries. Only the explicit first-WLIB-main empty
 * target namespace heap check below has an established success contract. */
ReconWatcomFirstMainHeap recon_watcom_first_main_heap;
/* Actual WCC initialized DWORD is16; native malloc policy is independent. */
unsigned recon_watcom_amblksiz=16;
/* Qualified loader AH2 is stored into this startup-owned byte before App. */
char recon_watcom_os=2;
static volatile int watcom_errno;
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static void watcom_position_scope(ShimFILE *stream,const ReconWatcomFilePosition *position)
{
    unsigned index;uintptr_t address=(uintptr_t)position,base;
    for(index=0;index<20 && stream!=&recon_iob[index];++index){}
    if(index==20 || !position || address%4 || address>UINTPTR_MAX-4 ||
       (address<(uintptr_t)stream+sizeof(*stream) && (uintptr_t)stream<address+4))
        recon_unsupported("Watcom file position stream/storage");
    if(!(stream->_flag&0x83) || (stream->_flag&0x40) ||
       (unsigned)stream->_file>=recon_nhandle || !(recon_osfile[stream->_file]&1))
        recon_unsupported("Watcom file position inactive stream");
    if(stream->_base && stream->_bufsiz>0) {
        base=(uintptr_t)stream->_base;
        if(base>UINTPTR_MAX-(unsigned)stream->_bufsiz ||
           (address<base+(unsigned)stream->_bufsiz && base<address+4))
            recon_unsupported("Watcom file position aliases stream buffer");
    }
}
#endif
int __cdecl recon_watcom_fgetpos(ShimFILE *stream,ReconWatcomFilePosition *position)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    long result;
    watcom_position_scope(stream,position);
    if((recon_osfile[stream->_file]&0x80) && (stream->_base || stream->_ptr || stream->_cnt))
        recon_unsupported("Watcom file position unqualified text buffering");
    result=recon_ftell(stream);
    /* The original stores-1/returns1 on ftell error; that errno/failure domain
     * remains unsupported. Qualified normal calls store the exact DWORD. */
    if(result<0) recon_unsupported("Watcom fgetpos backend failure");
    *position=result;return 0;
#else
    (void)stream;(void)position;
    recon_unsupported("Watcom fgetpos requires explicit profile");return 1;
#endif
}
int __cdecl recon_watcom_fsetpos(ShimFILE *stream,const ReconWatcomFilePosition *position)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    watcom_position_scope(stream,position);
    if(*position<0) recon_unsupported("Watcom fsetpos negative position");
    if(recon_fseek(stream,*position,SEEK_SET))
        recon_unsupported("Watcom fsetpos backend failure");
    return 0;
#else
    (void)stream;(void)position;
    recon_unsupported("Watcom fsetpos requires explicit profile");return -1;
#endif
}
volatile int *__cdecl recon_watcom_errno(void)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    return &watcom_errno;
#else
    recon_unsupported("Watcom errno requires explicit profile");return NULL;
#endif
}
/* Exact dyadic subset of the old decimal conversion role. Caller supplies a
 * readable terminated string; bounds do not prove arbitrary storage ownership.
 * The result uses the ordinary native typed-double ABI, not Watcom registers. */
double __cdecl recon_watcom_strtod(const char *text,char **end)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    uint64_t coefficient=0,bits,odd;unsigned n,digits=0,fraction=0,width;
    int dot=0,power,exponent;double result;
    if(end || (uintptr_t)text<4096 || (uintptr_t)text>UINTPTR_MAX-19)
        recon_unsupported("Watcom strtod input/end span");
    if(recon_mb_codepage || recon_mb_codepage_gate || recon_lc_codepage || recon_mb_cur_max!=1)
        recon_unsupported("Watcom strtod default locale");
    for(n=0;n<6;++n)if(recon_lc_handle[n])
        recon_unsupported("Watcom strtod locale handles");
    for(n=0;n<19 && text[n];++n) {
        if(text[n]=='.' && !dot){dot=1;continue;}
        if(text[n]<'0' || text[n]>'9' || ++digits>17)
            recon_unsupported("Watcom strtod plain decimal grammar");
        coefficient=coefficient*10+(unsigned)(text[n]-'0');
        if(dot)++fraction;
    }
    if(n==19 || !digits)recon_unsupported("Watcom strtod length/digits");
    bits=0;
    if(coefficient) {
        power=-(int)fraction;
        while(fraction--) {
            if(coefficient%5)recon_unsupported("Watcom strtod non-dyadic decimal");
            coefficient/=5;
        }
        while(!(coefficient&1)){coefficient>>=1;++power;}
        odd=coefficient;for(width=0;odd;odd>>=1)++width;
        if(width>53)recon_unsupported("Watcom strtod inexact significand");
        exponent=(int)width-1+power;
        if(exponent<-1022 || exponent>1023)
            recon_unsupported("Watcom strtod nonnormal result");
        bits=((uint64_t)(exponent+1023)<<52)|
             ((coefficient<<(53-width))&UINT64_C(0xfffffffffffff));
    }
    memcpy(&result,&bits,sizeof(result));return result;
#else
    (void)text;(void)end;
    recon_unsupported("strtod requires explicit Watcom profile");return 0;
#endif
}
/* The selected software-runtime path forms an exact integer coefficient,
 * then divides by an exact power of ten for exponents -17..0. These native
 * typed boundaries reuse the exact-decimal domain; they do not reproduce the
 * old register ABI or its general rounding/overflow implementation. */
void __cdecl recon_watcom_zbuf2f(char *digits,double *output)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    unsigned length,i;double result;
    uintptr_t input=(uintptr_t)digits,destination=(uintptr_t)output;
    if(input<4096 || input>UINTPTR_MAX-19 || destination<4096 ||
       destination>UINTPTR_MAX-sizeof(*output) ||
       (destination&3))
        recon_unsupported("Watcom ZBuf2F input/output span");
    for(length=0;length<18 && digits[length];++length)
        if(digits[length]<'0' || digits[length]>'9')
            recon_unsupported("Watcom ZBuf2F unsigned digits");
    if(!length || length>17)
        recon_unsupported("Watcom ZBuf2F digit count");
    if(destination<input+length+1 && input<destination+sizeof(*output))
        recon_unsupported("Watcom ZBuf2F overlapping output");
    /* The existing conversion validates default-C and exact representability
     * before the only output write. Caller owns both complete live spans. */
    result=recon_watcom_strtod(digits,NULL);
    for(i=0;i<sizeof(result);++i)
        ((unsigned char *)output)[i]=((const unsigned char *)&result)[i];
#else
    (void)digits;(void)output;
    recon_unsupported("ZBuf2F requires explicit Watcom profile");
#endif
}
double __cdecl recon_watcom_scale10v(double value,int exponent)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    uint64_t bits,coefficient,significand;unsigned encoded,n=0,i,fraction,used=0;
    int power,shift;char digits[18],text[19];
    if(exponent>0 || exponent< -17)
        recon_unsupported("Watcom Scale10V decimal exponent");
    memcpy(&bits,&value,sizeof(bits));
    encoded=(unsigned)((bits>>52)&0x7ff);
    if(bits>>63)recon_unsupported("Watcom Scale10V negative input");
    coefficient=0;
    if(bits) {
        power=(int)encoded-1023;
        if(!encoded || power<0 || power>56)
            recon_unsupported("Watcom Scale10V integral coefficient");
        significand=(bits&UINT64_C(0xfffffffffffff))|UINT64_C(0x10000000000000);
        shift=power-52;
        if(shift>=0)coefficient=significand<<shift;
        else {
            if(significand&((UINT64_C(1)<<(-shift))-1))
                recon_unsupported("Watcom Scale10V fractional coefficient");
            coefficient=significand>>(-shift);
        }
        if(coefficient>UINT64_C(99999999999999999))
            recon_unsupported("Watcom Scale10V coefficient digit count");
    }
    do {digits[n++]=(char)('0'+coefficient%10);coefficient/=10;}while(coefficient);
    fraction=(unsigned)-exponent;
    if(fraction>=n && fraction) {
        text[used++]='.';
        for(i=0;i<fraction-n;++i)text[used++]='0';
        for(i=0;i<n;++i)text[used++]=digits[n-1-i];
    } else {
        for(i=0;i<n;++i) {
            if(fraction && i==n-fraction)text[used++]='.';
            text[used++]=digits[n-1-i];
        }
    }
    text[used]=0;
    return recon_watcom_strtod(text,NULL);
#else
    (void)value;(void)exponent;
    recon_unsupported("Scale10V requires explicit Watcom profile");return 0;
#endif
}
/* Opted-in %.17e uses host formatting only when the exact decimal needs
 * no rounding at eighteen significant digits, then supplies Watcom's minimum
 * three exponent digits. Caller owns at least26 writable output bytes. */
int __cdecl recon_watcom_sprintf_17e(char *output,const char *format,...)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    va_list arguments;double value;uint64_t bits,magnitude,coefficient;
    unsigned encoded,i,negative;int power,count,saved_errno;char text[32];
    uintptr_t destination=(uintptr_t)output,input=(uintptr_t)format;
    if(destination<4096 || destination>UINTPTR_MAX-26 ||
       input<4096 || input>UINTPTR_MAX-6 || memcmp(format,"%.17e",6))
        recon_unsupported("Watcom sprintf17e format/output span");
    if(destination<input+6 && input<destination+26)
        recon_unsupported("Watcom sprintf17e format/output overlap");
    if(recon_mb_codepage || recon_mb_codepage_gate || recon_lc_codepage || recon_mb_cur_max!=1)
        recon_unsupported("Watcom sprintf17e default locale");
    for(i=0;i<6;++i)if(recon_lc_handle[i])
        recon_unsupported("Watcom sprintf17e locale handles");
    va_start(arguments,format);value=va_arg(arguments,double);va_end(arguments);
    memcpy(&bits,&value,sizeof(bits));negative=(unsigned)(bits>>63);
    magnitude=bits&UINT64_C(0x7fffffffffffffff);
    coefficient=0;
    if(!magnitude && negative)recon_unsupported("Watcom sprintf17e negative zero");
    if(magnitude) {
        encoded=(unsigned)(magnitude>>52);
        if(!encoded || encoded==0x7ff)
            recon_unsupported("Watcom sprintf17e nonnormal value");
        coefficient=(magnitude&UINT64_C(0xfffffffffffff))|UINT64_C(0x10000000000000);
        power=(int)encoded-1023-52;
        while(!(coefficient&1)){coefficient>>=1;++power;}
        if(power>=0) {
            if(power>59 || coefficient>(UINT64_C(999999999999999999)>>power))
                recon_unsupported("Watcom sprintf17e decimal precision");
            coefficient<<=power;
        } else {
            if(power< -17)recon_unsupported("Watcom sprintf17e fractional precision");
            while(power++<0) {
                if(coefficient>UINT64_C(999999999999999999)/5)
                    recon_unsupported("Watcom sprintf17e decimal precision");
                coefficient*=5;
            }
        }
    }
    saved_errno=errno;count=snprintf(text,sizeof(text),"%.17e",value);errno=saved_errno;
    /* Admitted values have a two-digit host exponent. Check that result
     * before inserting its leading zero; output is still untouched. */
    if(count!=23+(int)negative || text[count-4]!='e' ||
       (text[count-3]!='+' && text[count-3]!='-') ||
       text[count-2]<'0' || text[count-2]>'9' ||
       text[count-1]<'0' || text[count-1]>'9')
        recon_unsupported("Watcom sprintf17e host spelling");
    text[count+1]=0;text[count]=text[count-1];text[count-1]=text[count-2];
    text[count-2]='0';++count;
    memcpy(output,text,(size_t)count+1);return count;
#else
    (void)output;(void)format;
    recon_unsupported("sprintf17e requires explicit Watcom profile");return 0;
#endif
}
char *__cdecl recon_watcom_strrev(char *text)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    size_t length,left,right;char byte;
    if((uintptr_t)text<4096 || (uintptr_t)text>UINTPTR_MAX-256)
        recon_unsupported("Watcom strrev input span");
    /* The reached byte-sized initializer domain supplies mutable storage
     * through its NUL. This guard does not establish general memory ownership. */
    length=strnlen(text,256);
    if(length==256)recon_unsupported("Watcom strrev length/terminator");
    if(length>1)for(left=0,right=length-1;left<right;++left,--right) {
        byte=text[left];text[left]=text[right];text[right]=byte;
    }
    return text;
#else
    (void)text;recon_unsupported("strrev requires explicit Watcom profile");return NULL;
#endif
}
void __cdecl recon_watcom_splitpath_dos(const char *path,char *drive,char *directory,
                                      char *name,char *extension)
{
#if defined(RECON_CRT_PROFILE_WATCOM_100A) && defined(RECON_SPLITPATH_WATCOM_DOS)
    char *outputs[4]={drive,directory,name,extension};
    const size_t spans[4]={1,1,9,5};
    const char *parts[4];size_t lengths[4],length,dot,n,prior;
    uintptr_t output;
    if((uintptr_t)path<4096 || (uintptr_t)path>UINTPTR_MAX-260)
        recon_unsupported("Watcom DOS splitpath input span");
    for(length=0;length<260 && path[length];++length)
        if((unsigned char)path[length]>=128 || path[length]==':' ||
           path[length]=='/' || path[length]=='\\')
            recon_unsupported("Watcom DOS splitpath relative ASCII leaf");
    if(length==260)recon_unsupported("Watcom DOS splitpath long leaf");
    dot=length;for(n=0;n<length;++n)if(path[n]=='.')dot=n;
    parts[0]=parts[1]=parts[2]=path;parts[3]=path+dot;
    lengths[0]=lengths[1]=0;lengths[2]=dot>8?8:dot;
    lengths[3]=length-dot>4?4:length-dot;
    for(n=0;n<4;++n)if(outputs[n]) {
        output=(uintptr_t)outputs[n];
        if(output<4096 || output>UINTPTR_MAX-spans[n] ||
           (output<(uintptr_t)path+length+1 && (uintptr_t)path<output+spans[n]))
            recon_unsupported("Watcom DOS splitpath output/input span");
        for(prior=0;prior<n;++prior)if(outputs[prior] &&
           output<(uintptr_t)outputs[prior]+spans[prior] &&
           (uintptr_t)outputs[prior]<output+spans[n])
            recon_unsupported("Watcom DOS splitpath overlapping outputs");
    }
    /* DOS provider bounds include the terminator: filename9/extension5.
     * Empty requested drive/directory and every guard precede any output. */
    for(n=0;n<4;++n)if(outputs[n]) {
        if(lengths[n])memcpy(outputs[n],parts[n],lengths[n]);
        outputs[n][lengths[n]]=0;
    }
#else
    (void)path;(void)drive;(void)directory;(void)name;(void)extension;
    recon_unsupported("Watcom DOS splitpath requires qualified variant");
#endif
}
void __cdecl recon_watcom_splitpath2(const char *path,char *buffer,char **drive,
                                   char **directory,char **name,char **extension)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    char **outputs[4]={drive,directory,name,extension};
    const char *parts[4];size_t lengths[4],length,dot,total=0,n,prior;
    char *cursor=buffer;uintptr_t output;
    if(!path || !buffer || (uintptr_t)path>UINTPTR_MAX-260)
        recon_unsupported("Watcom splitpath2 input/workspace");
    for(length=0;length<260 && path[length];++length)
        if((unsigned char)path[length]>=128 || path[length]==':' ||
           path[length]=='/' || path[length]=='\\')
            recon_unsupported("Watcom splitpath2 outside relative ASCII leaf");
    if(length==260) recon_unsupported("Watcom splitpath2 long leaf");
    dot=length;for(n=0;n<length;++n)if(path[n]=='.')dot=n;
    parts[0]=parts[1]=parts[2]=path;parts[3]=path+dot;
    lengths[0]=lengths[1]=0;lengths[2]=dot;lengths[3]=length-dot;
    for(n=0;n<4;++n)if(outputs[n])total+=lengths[n]+1;
    if((uintptr_t)buffer>UINTPTR_MAX-total ||
       ((uintptr_t)buffer<(uintptr_t)path+length+1 &&
        (uintptr_t)path<(uintptr_t)buffer+total))
        recon_unsupported("Watcom splitpath2 workspace wrap/overlap");
    for(n=0;n<4;++n)if(outputs[n]) {
        output=(uintptr_t)outputs[n];
        if(output%sizeof(char *) || output>UINTPTR_MAX-sizeof(char *) ||
           (output<(uintptr_t)path+length+1 && (uintptr_t)path<output+sizeof(char *)) ||
           (output<(uintptr_t)buffer+total && (uintptr_t)buffer<output+sizeof(char *)))
            recon_unsupported("Watcom splitpath2 output-location overlap/span");
        for(prior=0;prior<n;++prior)if(outputs[prior] &&
           output<(uintptr_t)outputs[prior]+sizeof(char *) &&
           (uintptr_t)outputs[prior]<output+sizeof(char *))
            recon_unsupported("Watcom splitpath2 aliased output locations");
    }
    /* Only requested components occupy workspace. Empty drive/directory each
     * receive their own NUL; no allocation or host path interpretation occurs. */
    for(n=0;n<4;++n)if(outputs[n]) {
        *outputs[n]=cursor;
        if(lengths[n])memcpy(cursor,parts[n],lengths[n]);
        cursor[lengths[n]]=0;cursor+=lengths[n]+1;
    }
#else
    (void)path;(void)buffer;(void)drive;(void)directory;(void)name;(void)extension;
    recon_unsupported("Watcom splitpath2 requires explicit profile");
#endif
}
void *__cdecl recon_watcom_fill_dwords(void *destination,unsigned pattern,unsigned count)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    unsigned char *cursor=destination;size_t bytes;
    if(count>SIZE_MAX/4) recon_unsupported("Watcom DWORD fill count overflow");
    bytes=(size_t)count*4;
    if(bytes && (!destination || (uintptr_t)destination>UINTPTR_MAX-bytes))
        recon_unsupported("Watcom DWORD fill destination span");
    /* Original register ABI is expressed by the period header. Native uses
     * ordinary C and permits valid unaligned storage without aliasing UB. */
    while(count--) { memcpy(cursor,&pattern,4);cursor+=4; }
    return cursor;
#else
    (void)destination;(void)pattern;(void)count;
    recon_unsupported("Watcom DWORD fill requires explicit profile");return NULL;
#endif
}
char *__cdecl recon_watcom_strerror(int error)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    if(error==1) return "No such file or directory";
#else
    (void)error;
#endif
    recon_unsupported("unreviewed Watcom strerror error/profile");return NULL;
}
int __cdecl recon_watcom_open(const char *name,int flags,...)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    char path[PATH_MAX];struct stat information;int saved_errno=errno,fd,error;
    int exclusive=flags==0x0621,temporary=flags==0x0261,existing_rw=flags==0x0202;
    int writing=flags==0x0162 || flags==0x0262 || exclusive || temporary,permission=0,mode=0x8000;
    va_list arguments;
    const unsigned char *cursor=(const unsigned char *)name;
    if((flags!=0x0200 && !writing && !existing_rw) || !name || !*name || strpbrk(name,"*?"))
        recon_unsupported("Watcom open outside qualified tuple");
    for(;*cursor;++cursor)if(*cursor>=128) recon_unsupported("Watcom open nonASCII path");
    if(writing || existing_rw) {
        size_t length=strlen(name),component_size;
        const char *component=name;char prefix[MAX_PATH];
        if(length>=MAX_PATH || strpbrk(name,":") || name[0]=='/' || name[0]=='\\' ||
           ((exclusive || temporary) && strpbrk(name,"/\\")))
            recon_unsupported("Watcom writable open outside relative path");
        for(cursor=(const unsigned char *)name;;++cursor) {
            if(*cursor && *cursor!='/' && *cursor!='\\')continue;
            component_size=(const char *)cursor-component;
            if(!component_size || component[component_size-1]=='.' ||
               component[component_size-1]==' ')
                recon_unsupported("Watcom writable open invalid relative component");
            if(!*cursor)break;
            memcpy(prefix,name,(const char *)cursor-name);
            prefix[(const char *)cursor-name]=0;
            if(recon_host_path(prefix,path,sizeof(path)) || lstat(path,&information) ||
               !S_ISDIR(information.st_mode)) {
                errno=saved_errno;
                recon_unsupported("Watcom writable open requires existing directory");
            }
            component=(const char *)cursor+1;
        }
    }
    if(writing) {
        /* Only creating tuples consume a permission argument. Readonly and
         * existing-RW two-argument calls must never fetch one. */
        va_start(arguments,flags);permission=va_arg(arguments,int);va_end(arguments);
        /* Watcom's zero permission selects0600 under the reached default mask. */
        if(flags==0x0262 && !permission)permission=0600;
        if((flags==0x0162 && permission!=0666) ||
           (flags==0x0262 && permission!=0600 && permission!=0666) ||
           ((exclusive || temporary) && permission!=0666))
            recon_unsupported("Watcom writable open permission tuple");
        mode=exclusive?0x8501:temporary?0x8301:flags==0x0162?0x4302:0x8302;
    } else if(existing_rw) mode=0x8002;
    if(recon_host_path(name,path,sizeof(path)))
        recon_unsupported("Watcom open translated path");
    if(lstat(path,&information)) {
        error=errno;errno=saved_errno;
        if(!writing && !existing_rw && (error==ENOENT || error==ENOTDIR)) {
            watcom_errno=1;return -1;
        }
        if(!writing || error!=ENOENT)
            recon_unsupported("Watcom open unknown filesystem error");
    } else if(exclusive || temporary || !S_ISREG(information.st_mode))
        recon_unsupported("Watcom open regular existing path");
    /* Preflight is host-only. Existing descriptor ownership handles the real
     * readonly, existing-RW or create/truncate open and its lowio state. */
    errno=saved_errno;
    fd=writing?recon_open(name,mode,permission):recon_open(name,mode);
    if(fd<0) recon_unsupported("Watcom qualified open failure");
    return fd;
#else
    /* The actual caller supplies only two arguments. Do not fetch an unused
     * variadic permission word, or reinterpret these flags as Microsoft/Linux. */
    (void)name;(void)flags;
    recon_unsupported("unreviewed Watcom open");return -1;
#endif
}
long __cdecl recon_watcom_lseek(int fd,long offset,int origin)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    long handle,result;struct stat information;int saved_errno=errno;
    if((unsigned)fd>=recon_nhandle || !(recon_osfile[fd]&1) || origin<0 || origin>2)
        recon_unsupported("Watcom lseek descriptor/origin");
    handle=recon_get_osfhandle(fd);
    if(handle<=0 || fstat((int)handle-1,&information) || !S_ISREG(information.st_mode))
        recon_unsupported("Watcom lseek regular descriptor");
    errno=saved_errno;
    result=recon_lseek(fd,offset,origin);
    if(result<0) recon_unsupported("Watcom lseek unsupported failure/range");
    return result;
#else
    (void)fd;(void)offset;(void)origin;
    recon_unsupported("Watcom lseek requires explicit profile");return -1;
#endif
}
long __cdecl recon_watcom_tell(int fd)
{
    return recon_watcom_lseek(fd,0,SEEK_CUR);
}
int __cdecl recon_watcom_setmode(int fd,int mode)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    int previous;
    if(mode!=0x0200 || (unsigned)fd>=recon_nhandle || !(recon_osfile[fd]&1))
        recon_unsupported("Watcom setmode binary descriptor domain");
    previous=recon_setmode(fd,0x8000);
    if(previous==0x8000) return 0x0200;
    if(previous==0x4000) return 0x0100;
    recon_unsupported("Watcom setmode unsupported previous mode");return -1;
#else
    (void)fd;(void)mode;
    recon_unsupported("Watcom setmode requires explicit profile");return -1;
#endif
}
int __cdecl recon_watcom_memicmp(const void *left,const void *right,size_t count)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    const unsigned char *a=left,*b=right;
    unsigned first,second;
    if(!count)return 0;
    if((uintptr_t)left<4096 || (uintptr_t)right<4096 ||
       (uintptr_t)left>UINTPTR_MAX-count || (uintptr_t)right>UINTPTR_MAX-count)
        recon_unsupported("Watcom memicmp caller spans");
    while(count--) {
        first=*a++;second=*b++;
        if(first>='A' && first<='Z')first+='a'-'A';
        if(second>='A' && second<='Z')second+='a'-'A';
        if(first!=second)return (int)first-(int)second;
    }
    return 0;
#else
    (void)left;(void)right;(void)count;
    recon_unsupported("unreviewed Watcom memicmp");return 0;
#endif
}
struct ReconWatcomDirent *__cdecl recon_watcom_opendir(const char *path)
{
    (void)path;recon_unsupported("unreviewed Watcom opendir");return NULL;
}
struct ReconWatcomDirent *__cdecl recon_watcom_readdir(struct ReconWatcomDirent *directory)
{
    (void)directory;recon_unsupported("unreviewed Watcom readdir");return NULL;
}
int __cdecl recon_watcom_closedir(struct ReconWatcomDirent *directory)
{
    (void)directory;recon_unsupported("unreviewed Watcom closedir");return -1;
}
unsigned int __cdecl recon_watcom_rotl(unsigned int value,unsigned int count)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    count&=31;
    return count?(value<<count)|(value>>(32-count)):value;
#else
    (void)value;(void)count;
    recon_unsupported("Watcom _rotl requires explicit profile");return 0;
#endif
}
char *__cdecl recon_watcom_ultoa(unsigned long value,char *output,int radix)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    if((radix!=10 && radix!=31 && radix!=36) || !output || (uintptr_t)output>UINTPTR_MAX-12)
        recon_unsupported("Watcom ultoa decimal/base31/base36 output domain");
    /* Caller owns11B for decimal or8B for base31/base36; unsigned long is32-bit.
     * The conservative12B nonwrap check above does not detect capacity.
     * At most ten decimal or seven base31/base36 digits plus NUL are written. */
    return recon_ultoa(value,output,radix);
#else
    (void)value;(void)output;(void)radix;
    recon_unsupported("Watcom ultoa requires explicit profile");return NULL;
#endif
}
char *__cdecl recon_watcom_utoa(unsigned int value,char *output,int radix)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    if((radix!=10 && radix!=16) || !output || (uintptr_t)output>UINTPTR_MAX-12)
        recon_unsupported("Watcom utoa decimal/hexadecimal output domain");
    /* The public API has no capacity formal. The reached caller owns12B;
     * unsigned32 needs at most ten decimal or eight hexadecimal digits plus NUL. */
    return recon_ultoa(value,output,radix);
#else
    (void)value;(void)output;(void)radix;
    recon_unsupported("Watcom utoa requires explicit profile");return NULL;
#endif
}
int __cdecl recon_watcom_getch(void)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    struct stat input,null_device;int saved_errno=errno;
    unsigned char character=0;DWORD count=0;
    if(!recon_nhandle || (recon_osfile[0]!=0xc1 && recon_osfile[0]!=0x41) ||
       recon_get_osfhandle(0)!=1 || recon_GetStdHandle(STD_INPUT_HANDLE)!=(HANDLE)1)
        recon_unsupported("Watcom getch unchanged stdin ownership");
    if(fstat(0,&input) || stat("/dev/null",&null_device) ||
       !S_ISCHR(input.st_mode) || !S_ISCHR(null_device.st_mode) ||
       input.st_rdev!=null_device.st_rdev) {
        errno=saved_errno;
        recon_unsupported("Watcom getch requires null-device stdin");
    }
    errno=saved_errno;
    /* The old CRT initializes this byte before its direct Win32 read. Its
     * ignored console-mode failures do not supply an input character. Use
     * ReadFile rather than lowio read, which would add a CRT EOF flag. */
    if(!recon_ReadFile((HANDLE)1,&character,1,&count,NULL) || count || character)
        recon_unsupported("Watcom getch unsupported read result");
    return character;
#else
    recon_unsupported("Watcom getch requires explicit profile");return 0;
#endif
}
void recon_bind_watcom_wlib_first_main_heap(const char *reference)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    unsigned n;
    if (recon_watcom_first_main_heap.bound || recon_watcom_first_main_heap.calls ||
        recon_watcom_first_main_heap.clean || recon_watcom_first_main_heap.target_activity ||
        !reference || strlen(reference)!=64)
        recon_unsupported("Watcom first-main heap binding prestate");
    for(n=0;n<64;++n)
        if(!((reference[n]>='0' && reference[n]<='9') || (reference[n]>='a' && reference[n]<='f')))
            recon_unsupported("Watcom first-main heap reference SHA256");
    memcpy(recon_watcom_first_main_heap.reference_sha256,reference,65);
    recon_watcom_first_main_heap.bound=1;
#else
    (void)reference;recon_unsupported("Watcom first-main heap requires explicit profile");
#endif
}
static void watcom_target_heap_activity(void)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    /* Sticky, including activity before binding: it cannot wrap back to empty.
     * Host/internal allocations use libc directly and never enter here. */
    recon_watcom_first_main_heap.target_activity=1;
#else
    recon_unsupported("Watcom target allocation requires explicit profile");
#endif
}
/* The caller supplies the still-live base of a genuine host malloc block;
 * arithmetic guards do not establish ownership. This finite policy reports an
 * actual capacity shortfall without relocating or modifying the allocation. */
void *__cdecl recon_watcom_expand(void *pointer,size_t size)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    size_t capacity;int saved_errno=errno;
    if((uintptr_t)pointer<4096 || !size || (uintptr_t)pointer>UINTPTR_MAX-size)
        recon_unsupported("Watcom expansion pointer/request span");
    capacity=malloc_usable_size(pointer);errno=saved_errno;
    if(!capacity || size<=capacity)
        recon_unsupported("unreviewed Watcom expansion success/shrink");
    return NULL;
#else
    (void)pointer;(void)size;
    recon_unsupported("Watcom expansion requires explicit profile");return NULL;
#endif
}
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static __thread uintptr_t watcom_stack_low,watcom_stack_high;
static void watcom_stack_bounds(void)
{
    pthread_attr_t attributes;void *base=NULL;size_t length=0,guard=0;
    int status,saved_errno=errno;
    status=pthread_getattr_np(pthread_self(),&attributes);
    if(status) {errno=saved_errno;recon_unsupported("Watcom native stack metadata");}
    status=pthread_attr_getstack(&attributes,&base,&length);
    if(!status) status=pthread_attr_getguardsize(&attributes,&guard);
    if(pthread_attr_destroy(&attributes) && !status) status=1;
    errno=saved_errno;
    if(status || !base || !length || guard>=length ||
       (uintptr_t)base>UINTPTR_MAX-length)
        recon_unsupported("Watcom native stack bounds/guard");
    /* Conservatively exclude the reported guard even on hosts whose returned
     * usable span already omits it. Thread-stack/guard changes are unsupported. */
    watcom_stack_low=(uintptr_t)base+guard;
    watcom_stack_high=(uintptr_t)base+length;
}
#endif
int __cdecl recon_watcom_alloca_available(size_t size)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    unsigned char marker[32];uintptr_t current=(uintptr_t)marker;
    if(size!=400) recon_unsupported("Watcom unqualified alloca size");
    /* Query once with adequate ordinary call stack. Later low-space checks
     * avoid entering the deeper pthread metadata implementation. The local
     * probe lies below the caller and covers native alloca alignment padding. */
    if(!watcom_stack_high) watcom_stack_bounds();
    if(current<watcom_stack_low || current>=watcom_stack_high)
        recon_unsupported("Watcom current address outside native stack");
    return current-watcom_stack_low>size;
#else
    (void)size;recon_unsupported("Watcom alloca requires explicit profile");return 0;
#endif
}
void *recon_watcom_malloc(size_t size)
{ watcom_target_heap_activity();return malloc(size); }
void *recon_watcom_calloc(size_t count,size_t size)
{ watcom_target_heap_activity();return calloc(count,size); }
void *recon_watcom_realloc(void *pointer,size_t size)
{ watcom_target_heap_activity();return recon_realloc(pointer,size); }
void recon_watcom_free(void *pointer)
{ watcom_target_heap_activity();recon_free(pointer); }
char *recon_watcom_strdup(const char *text)
{ watcom_target_heap_activity();return strdup(text); }
int recon_watcom_heapchk(void)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    if (recon_watcom_first_main_heap.bound!=1 || recon_watcom_first_main_heap.calls ||
        recon_watcom_first_main_heap.clean || recon_watcom_first_main_heap.target_activity)
        recon_unsupported("Watcom heap check outside first-main empty target namespace");
    /* No target-owned allocation has entered this namespace. An empty heap
     * satisfies the supported integrity predicate; this diagnoses no glibc
     * corruption and represents no original CRT free-ring bytes. */
    recon_watcom_first_main_heap.clean=1;
    recon_watcom_first_main_heap.calls=1;
    return 0; /* Watcom _HEAPOK */
#else
    recon_unsupported("unreviewed Watcom _heapchk");return 0;
#endif
}
int recon_watcom_nheapchk(void)
{ recon_unsupported("unreviewed Watcom _nheapchk"); return 0; }
#ifdef RECON_CRT_PROFILE_WATCOM_100A
/* Reached single-threaded registration only. These are Watcom table tokens,
 * not Linux dispositions; raise and delivered-event behavior stay denied. */
static ReconSignalHandler watcom_signal_handlers[4]={
    (ReconSignalHandler)2,(ReconSignalHandler)1,(ReconSignalHandler)1,(ReconSignalHandler)1
};
#endif
ReconSignalHandler recon_watcom_signal(int number,ReconSignalHandler handler)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    unsigned slot;ReconSignalHandler previous;
    switch(number) {
    case 1:slot=0;break;
    case 4:slot=1;break;
    case 6:slot=2;break;
    case 7:slot=3;break;
    default:recon_unsupported("Watcom signal outside registration domain");return NULL;
    }
    if(!handler || handler==(ReconSignalHandler)3)
        recon_unsupported("Watcom signal unsupported handler");
    previous=watcom_signal_handlers[slot];watcom_signal_handlers[slot]=handler;
    return previous;
#else
    (void)number;(void)handler;
    recon_unsupported("Watcom signal requires explicit profile");return NULL;
#endif
}
int recon_watcom_raise(int number)
{ (void)number; recon_unsupported("unreviewed Watcom raise"); return 0; }
int recon_watcom_setjmp(unsigned int *environment)
{ (void)environment; recon_unsupported("unreviewed Watcom _setjmp"); return 0; }

/* These contexts are external to the 32-byte legacy object, whose address is
 * also stored as an int pointer by the C++ frontend. Addresses may be reused:
 * each new setjmp overwrites the previous saved context at that address.
 * Like the existing tracing runtime, this registry serves single-threaded
 * fixtures. Entries cannot be freed because C has no jump-buffer destructor. */
struct JumpContext {
    void *legacy;
    int opaque_setjmp3;
    int watcom_normal_only;
    recon_host_jmp_buf host;
    struct JumpContext *next;
};
static struct JumpContext *jump_contexts;

void recon_watcom_longjmp(unsigned int *environment,int value)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    struct JumpContext *context;
    if(!environment || value!=1)
        recon_unsupported("Watcom longjmp environment/value");
    for(context=jump_contexts;context;context=context->next)
        if(context->legacy==environment) {
            if(!context->watcom_normal_only || context->opaque_setjmp3)
                recon_unsupported("Watcom longjmp requires saved52-byte context");
            /* The reached Spawn frame is still live on this thread, on the
             * same unwind chain. Its opaque target owner is never serialized;
             * libc restores the existing caller-frame context, not x87 state. */
            longjmp(context->host,1);
        }
    recon_unsupported("Watcom longjmp without matching live save");
#else
    (void)environment;(void)value;
    recon_unsupported("Watcom longjmp requires explicit profile");
#endif
    abort();
}

recon_host_jmp_buf *recon_jmp_context(void *legacy)
{
    struct JumpContext *context;
    for (context = jump_contexts; context; context = context->next)
        if (context->legacy == legacy) { context->opaque_setjmp3=0; context->watcom_normal_only=0; return &context->host; }
    context = malloc(sizeof(*context));
    if (!context) recon_unsupported("setjmp context allocation");
    context->legacy = legacy;
    context->opaque_setjmp3=0;
    context->watcom_normal_only=0;
    context->next = jump_contexts;
    jump_contexts = context;
    return &context->host;
}

recon_host_jmp_buf *recon_setjmp3_context(void *legacy,unsigned size,int count)
{
    struct JumpContext *context; recon_host_jmp_buf *host;
    if (!legacy || size!=64 || count) recon_unsupported("unreviewed _setjmp3 storage/count/unwind");
    host=recon_jmp_context(legacy);
    for (context=jump_contexts;context;context=context->next)
        if (context->legacy==legacy) { context->opaque_setjmp3=1; break; }
    return host;
}

/* The Windows48B context and native out-of-line libc context are private,
 * differing CRT representations. Never serialize captured register/FS bytes
 * into the target's52B owner. The reached same-chain live Spawn may resume1. */
recon_host_jmp_buf *recon_watcom_setjmp_context(void *legacy,unsigned size)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    struct JumpContext *context;recon_host_jmp_buf *host;
    int saved_errno=errno;unsigned long saved_dos=recon_doserrno;
    DWORD saved_error=recon_GetLastError();
    if(!legacy || size!=52)recon_unsupported("Watcom setjmp requires52-byte owner");
    host=recon_jmp_context(legacy);
    for(context=jump_contexts;context;context=context->next)
        if(context->legacy==legacy){context->watcom_normal_only=1;break;}
    errno=saved_errno;recon_doserrno=saved_dos;recon_SetLastError(saved_error);
    return host;
#else
    (void)legacy;(void)size;recon_unsupported("Watcom setjmp requires explicit profile");return NULL;
#endif
}

void recon_longjmp(void *legacy, int value)
{
    struct JumpContext *context;
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    recon_unsupported("unreviewed Watcom nonlocal jump/unwind");
#endif
    for (context = jump_contexts; context; context = context->next)
        if (context->legacy == legacy) {
            if (context->opaque_setjmp3) {
#ifdef RECON_CRT_PROFILE_MSVCRT_510
                if(value!=1 && value!=2 && value!=102)recon_unsupported("MSVCRT510 count0 longjmp value");
                /* The caller guarantees this same-thread frame is still live,
                 * on the same chain with no active intervening SEH/C++ cleanup.
                 * The owner tag proves the64B/count0 save, not those lifetimes. */
                longjmp(context->host,value);
#else
                recon_unsupported("unreviewed _setjmp3 longjmp/SEH unwind");
#endif
            }
            if (context->watcom_normal_only) recon_unsupported("unreviewed Watcom saved-context resume");
            longjmp(context->host, value);
        }
    recon_unsupported("longjmp without matching setjmp");
    abort();
}

static unsigned abstract_control(unsigned short control)
{
    static const unsigned precision[4] = { _PC_24, _MCW_PC, _PC_53, _PC_64 };
    unsigned result = precision[(control >> 8) & 3];
    if (control & 0x01) result |= _EM_INVALID;
    if (control & 0x02) result |= _EM_DENORMAL;
    if (control & 0x04) result |= _EM_ZERODIVIDE;
    if (control & 0x08) result |= _EM_OVERFLOW;
    if (control & 0x10) result |= _EM_UNDERFLOW;
    if (control & 0x20) result |= _EM_INEXACT;
    result |= (control & 0x0c00) >> 2;
    result |= (control & 0x1000) << 6;
    return result;
}

unsigned recon_control87(unsigned value, unsigned mask)
{
    static const unsigned short precision[4] = { 0x300, 0x200, 0, 0x100 };
    unsigned short control, updated;
    unsigned desired;
    /* x87 control state has no ordinary C interface. Only these transfers use
     * assembly; translation of Microsoft's flags remains ordinary C. */
    __asm__ volatile ("fnstcw %0" : "=m" (control));
    desired = (abstract_control(control) & ~mask) | (value & mask);
    updated = control & ~0x1f3f;
    if (desired & _EM_INVALID) updated |= 0x01;
    if (desired & _EM_DENORMAL) updated |= 0x02;
    if (desired & _EM_ZERODIVIDE) updated |= 0x04;
    if (desired & _EM_OVERFLOW) updated |= 0x08;
    if (desired & _EM_UNDERFLOW) updated |= 0x10;
    if (desired & _EM_INEXACT) updated |= 0x20;
    updated |= precision[(desired & _MCW_PC) >> 16];
    updated |= (desired & _MCW_RC) << 2;
    updated |= (desired & _MCW_IC) >> 6;
    if (updated != control) __asm__ volatile ("fldcw %0" : : "m" (updated));
    return abstract_control(updated);
}

unsigned recon_controlfp(unsigned value, unsigned mask)
{
    /* Unlike _control87, _controlfp leaves the denormal mask unchanged. */
    return recon_control87(value, mask & ~_EM_DENORMAL);
}

static char *local_clock_configuration;
static uint64_t local_clock_calls;

uint64_t recon_local_clock_call_count(void)
{ return local_clock_calls; }

static unsigned clock_field(const char **cursor, int separator)
{
    const char *start = *cursor;
    char *end;
    unsigned long value;
    if (*start < '0' || *start > '9') recon_unsupported("RECON_LOCAL_CLOCK field");
    errno = 0;
    value = strtoul(start, &end, 10);
    if (errno || value > 0x7fffffffUL || *end != separator)
        recon_unsupported("RECON_LOCAL_CLOCK syntax");
    *cursor = end + (separator != 0);
    return (unsigned)value;
}

static int calendar_leap(int year)
{
    return !(year % 4) && ((year % 100) || !(year % 400));
}

static unsigned calendar_month_days(int year, unsigned month)
{
    static const unsigned days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    return days[month - 1] + (month == 2 && calendar_leap(year));
}

static int64_t calendar_seconds(int year, unsigned month, unsigned day,
    unsigned hour, unsigned minute, unsigned second)
{
    int current;
    unsigned index;
    int64_t days = day - 1;
    for (current = 1970; current < year; ++current) days += 365 + calendar_leap(current);
    for (current = year; current < 1970; ++current) days -= 365 + calendar_leap(current);
    for (index = 1; index < month; ++index) days += calendar_month_days(year, index);
    return ((days * 24 + hour) * 60 + minute) * 60 + second;
}

static void calendar_breakdown(int64_t seconds, struct tm *calendar)
{
    int64_t days = seconds / 86400, within_day = seconds % 86400;
    int weekday, year = 1970, month = 1;
    unsigned month_days;
    if (within_day < 0) { within_day += 86400; --days; }
    weekday = (int)((days + 4) % 7);
    if (weekday < 0) weekday += 7;
    memset(calendar, 0, sizeof(*calendar));
    while (days < 0) { --year; days += 365 + calendar_leap(year); }
    while (days >= 365 + calendar_leap(year)) {
        days -= 365 + calendar_leap(year);
        ++year;
    }
    calendar->tm_yday = (int)days;
    while (days >= (month_days = calendar_month_days(year, month))) {
        days -= month_days;
        ++month;
    }
    calendar->tm_year = year - 1900;
    calendar->tm_mon = month - 1;
    calendar->tm_mday = (int)days + 1;
    calendar->tm_wday = weekday;
    calendar->tm_hour = within_day / 3600;
    calendar->tm_min = (within_day % 3600) / 60;
    calendar->tm_sec = within_day % 60;
}

static int local_clock_dst(int64_t local_seconds, int64_t utc_seconds)
{
    struct tm calendar, hint;
    time_t host_seconds = (time_t)utc_seconds;
    calendar_breakdown(local_seconds, &calendar);
    /* The weak shared _isindst consumes tm_isdst; give it a host hint.
     * Reconstructed strong versions inspect this calendar themselves, keeping
     * their historical transition rules independent of the host's rules. */
    tzset();
    if (localtime_r(&host_seconds, &hint)) calendar.tm_isdst = hint.tm_isdst;
    return recon_daylight && recon_isindst(&calendar);
}

static void captured_local_ftime(struct _timeb *result, const char *configuration)
{
    const char *cursor = configuration;
    unsigned year, month, day, hour, minute = 0, second = 0, initial = 900, step = 1225;
    uint64_t elapsed;
    int64_t local_seconds, utc_seconds;
    int dst;
    if (strchr(configuration, ',')) {
        year = clock_field(&cursor, ',');
        month = clock_field(&cursor, ',');
        day = clock_field(&cursor, ',');
        hour = clock_field(&cursor, 0);
    } else {
        year = clock_field(&cursor, '-');
        month = clock_field(&cursor, '-');
        day = clock_field(&cursor, 'T');
        hour = clock_field(&cursor, ':');
        minute = clock_field(&cursor, ':');
        second = clock_field(&cursor, '.');
        initial = clock_field(&cursor, '/');
        step = clock_field(&cursor, 0);
    }
    if (year < 1 || year > 9999 || month < 1 || month > 12 || day < 1 ||
        day > calendar_month_days(year, month) || hour > 23 || minute > 59 ||
        second > 59 || initial > 999)
        recon_unsupported("RECON_LOCAL_CLOCK calendar");
    if (!local_clock_configuration || strcmp(local_clock_configuration, configuration)) {
        free(local_clock_configuration);
        local_clock_configuration = strdup(configuration);
        if (!local_clock_configuration) recon_unsupported("local clock configuration allocation");
        local_clock_calls = 0;
    }
    if (step && local_clock_calls > (UINT64_MAX - initial) / step)
        recon_unsupported("local clock sequence overflow");
    elapsed = initial + local_clock_calls++ * step;
    if (elapsed / 1000 > 0x7fffffffUL)
        recon_unsupported("local clock sequence duration");
    recon_tzset();
    result->timezone = (short)(recon_timezone / 60);
    result->millitm = (unsigned short)(elapsed % 1000);
    result->time = -1;
    result->dstflag = -1;
    /* Shipped _ftime uses mktime with tm_isdst=-1. That CRT rejects years
     * outside 1969..2039 before normalization, and negative/overflowed time_t. */
    if (year < 1969 || year > 2039) return;
    local_seconds = calendar_seconds(year, month, day, hour, minute, second) + (int64_t)(elapsed / 1000);
    utc_seconds = local_seconds + recon_timezone;
    if (utc_seconds < 0 || utc_seconds > 0x7fffffffL) return;
    dst = local_clock_dst(local_seconds, utc_seconds);
    if (dst) {
        utc_seconds -= 3600;
        if (utc_seconds < 0) return;
        /* mktime's second localtime pass classifies the adjusted standard
         * calendar, including the old CRT's ambiguous transition hours. */
        dst = local_clock_dst(local_seconds - 3600, utc_seconds);
    }
    result->time = (long)utc_seconds;
    result->dstflag = (short)(dst != 0);
}

void recon_ftime(struct _timeb *result)
{
    const char *local_clock = getenv("RECON_LOCAL_CLOCK");
    const char *fixed = getenv("RECON_FIXED_TIME");
    const char *milliseconds = getenv("RECON_FIXED_MILLISECONDS");
    struct timeval current;
    struct tm local;
    time_t seconds;
    long millis;
    char *end;
    if (local_clock) {
        captured_local_ftime(result, local_clock);
        return;
    }
    if (local_clock_configuration) {
        free(local_clock_configuration);
        local_clock_configuration = NULL;
        local_clock_calls = 0;
    }
    if (fixed) {
        seconds = (time_t)strtoll(fixed, NULL, 10);
        millis = 0;
    } else {
        if (gettimeofday(&current, NULL)) recon_unsupported("_ftime clock");
        seconds = current.tv_sec;
        millis = current.tv_usec / 1000;
    }
    if (milliseconds) {
        errno = 0;
        millis = strtol(milliseconds, &end, 10);
        if (errno || !*milliseconds || *end || millis < 0 || millis > 999)
            recon_unsupported("RECON_FIXED_MILLISECONDS must be 0..999");
    }
    tzset();
    if (!localtime_r(&seconds, &local)) recon_unsupported("_ftime local time");
    result->time = (long)seconds;
    result->millitm = (unsigned short)millis;
    /* The timeb offset describes standard time, even during daylight time. */
    result->timezone = (short)(timezone / 60);
    result->dstflag = local.tm_isdst > 0;
}

size_t recon_wcslen(const WCHAR *text)
{
    const WCHAR *end = text;
    while (*end) ++end;
    return (size_t)(end - text);
}

__attribute__((weak)) WCHAR *recon_wcscpy(WCHAR *destination, const WCHAR *source)
{
    WCHAR *cursor = destination;
    while ((*cursor++ = *source++) != 0) { }
    return destination;
}

WCHAR *recon_wcsncpy(WCHAR *destination, const WCHAR *source, size_t count)
{
    size_t index = 0;
    while (index < count && source[index]) {
        destination[index] = source[index];
        ++index;
    }
    while (index < count) destination[index++] = 0;
    return destination;
}

/* Captured host input before any target DLL instruction. Only the observed
 * architectural words are supplied; native pointers and empty payload stay. */
static struct {
    char reference_sha256[65];
    unsigned applications;
} native_x87_empty_entry;
void recon_apply_x87_empty_entry_profile(const char *reference_sha256,
                                        unsigned control,unsigned status,unsigned tag)
{
    unsigned char environment[28];
    unsigned short current_control,current_status,current_tag,word;
    unsigned n;int saved_errno=errno;unsigned long saved_dos=recon_doserrno;
    DWORD saved_error=recon_GetLastError();
    if (native_x87_empty_entry.applications || !reference_sha256 ||
        strlen(reference_sha256)!=64 || control!=0x027f || status!=0x0020 || tag!=0xffff)
        recon_unsupported("unreviewed x87 empty-entry profile");
    for(n=0;n<64;++n)if(!((reference_sha256[n]>='0' && reference_sha256[n]<='9') ||
        (reference_sha256[n]>='a' && reference_sha256[n]<='f')))
        recon_unsupported("x87 empty-entry reference SHA256");
    __asm__ __volatile__("fnstenv %0":"=m"(environment));
    memcpy(&current_control,environment,2);memcpy(&current_status,environment+4,2);
    memcpy(&current_tag,environment+8,2);
    if(current_control!=0x027f || current_status || current_tag!=0xffff)
        recon_unsupported("x87 empty-entry native prestate");
    word=(unsigned short)status;memcpy(environment+4,&word,2);
    __asm__ __volatile__("fldenv %0"::"m"(environment));
    memcpy(native_x87_empty_entry.reference_sha256,reference_sha256,65);
    native_x87_empty_entry.applications=1;
    errno=saved_errno;recon_doserrno=saved_dos;recon_SetLastError(saved_error);
}
