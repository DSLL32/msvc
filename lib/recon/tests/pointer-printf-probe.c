/* Explicit static-CRT probe. Assembly observes x87 state only. */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
#ifdef RECON_NATIVE_TEST
#include <recon.h>
#endif
extern unsigned long _doserrno;
static void snapshot(unsigned char *data)
{
#ifdef RECON_NATIVE_TEST
    __asm__ __volatile__("fnsave (%0); frstor (%0)"::"r"(data):"memory");
#else
    __asm mov eax,data
    __asm fnsave [eax]
    __asm frstor [eax]
#endif
}
static void initialize(void)
{
#ifdef RECON_NATIVE_TEST
    __asm__ __volatile__("fninit; fld1");
#else
    __asm fninit
    __asm fld1
#endif
}
static int emit(FILE *file,const char *format,...)
{
    va_list args;int count;va_start(args,format);count=vfprintf(file,format,args);va_end(args);return count;
}
int main(void)
{
    static const char *formats[]={"|con=0x%p","[%p]","%%p=%p%%","[%12p]","[%012p]","[%-12p]","[%.4p]","[%lp]"};
    static const unsigned long values[]={0,1,0x12,0xabcdef,0x12000100,0xffffffff};
    unsigned char *buffer,prebuffer[4096],prefile[32],postfile[32],before[108],after[108];
    unsigned long header[12],last,dos;unsigned e,v,f,count=8;int result,saved;FILE *states,*file;
#ifdef RECON_NATIVE_TEST
    count=3;
    {
        const char *mode=recon_host_getenv("RECON_POINTER_PRINTF_NEGATIVE"),*format="%p";
        if(mode){
            if(!strcmp(mode,"escaped"))return emit(stdout,"%%p")==2?0:43;
            if(!strcmp(mode,"bad-prefix"))format="%q%p";
            if(!strcmp(mode,"bad-space"))format="%q %p";
            if(!strcmp(mode,"bad-chain"))format="%d%q%p";
            if(!strcmp(mode,"bad-escape-chain"))format="%q%%%p";
            if(!strcmp(mode,"width"))format="%12p";
            if(!strcmp(mode,"zero"))format="%012p";
            if(!strcmp(mode,"left"))format="%-12p";
            if(!strcmp(mode,"precision"))format="%.4p";
            if(!strcmp(mode,"length"))format="%lp";
            if(!strcmp(mode,"mixed"))format="%p:%d";
            if(!strcmp(mode,"multiple"))format="%p:%p";
            if(!strcmp(mode,"star"))format="%*p";
            if(!strcmp(mode,"trailing"))format="%p%";
            if(!strcmp(mode,"nonascii"))format="\200%p";
            if(!strcmp(mode,"locale"))recon_lc_handle[2]=1033;
            if(!strcmp(mode,"codepage"))recon_mb_codepage=1252;
            if(!strcmp(mode,"max"))recon_mb_cur_max=2;
            emit(stdout,format,(void*)0x1234abcd);return 42;
        }
    }
#endif
    if(sizeof(FILE)!=32)return 120;
    states=fopen("states.bin","wb");if(!states)return 121;
    for(e=0;e<4;++e)for(v=0;v<6;++v)for(f=0;f<count;++f){
        file=fopen("value.bin","wb");if(!file)return 122;
        if(fputc('!',file)==EOF || fflush(file) || file->_bufsiz!=4096)return 123;
        buffer=(unsigned char*)file->_base;memset(buffer,0x5a,4096);
        memcpy(prebuffer,buffer,4096);memcpy(prefile,file,32);
        initialize();snapshot(before);
        errno=(e&1)?111:0;_doserrno=77;SetLastError((e&2)?0x12345678:0);
        result=emit(file,formats[f],(void*)values[v]);
        last=GetLastError();saved=errno;dos=_doserrno;snapshot(after);
        memcpy(postfile,file,32);
        header[0]=e;header[1]=v;header[2]=f;header[3]=values[v];header[4]=(unsigned long)buffer;
        header[5]=result;header[6]=(e&1)?111:0;header[7]=saved;header[8]=77;header[9]=dos;
        header[10]=(e&2)?0x12345678:0;header[11]=last;
        fwrite(header,4,12,states);fwrite(prefile,1,32,states);fwrite(postfile,1,32,states);
        fwrite(prebuffer,1,4096,states);fwrite(buffer,1,4096,states);
        fwrite(before,1,108,states);fwrite(after,1,108,states);
        fclose(file);
    }
    fclose(states);return 0;
}
