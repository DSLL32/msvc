#define RECON_SHIM_IMPL
#include <assert.h>
#include <fcntl.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static void visible(const char *expected,size_t size)
{
    struct stat information;char data[1024];
    assert(!fstat(1,&information) && information.st_size==(off_t)size);
    assert(pread(1,data,size,0)==(ssize_t)size && !memcmp(data,expected,size));
}
int main(int argc,char **argv)
{
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,333}};
    unsigned row=0,n;int live=argc>2 && !strcmp(argv[2],"live"),wide=argc>2 && !strcmp(argv[argc-1],"wide"),first,second;char guarded[272],replacement[272],*buffer=guarded+8,*original=buffer,expected[1024],sentinel;
    unsigned char before[108],after[108],lowio[256];ShimFILE others[2];
    ShimFILE *stream=&recon_iob[1];int mode=0x0200;size_t size=wide?256:128;
    assert(argc>=2);memset(guarded,0xa5,sizeof(guarded));memset(replacement,0xa5,sizeof(replacement));
    if(argv[1][0]<'0' || argv[1][0]>'2') {
        if(!strcmp(argv[1],"mode"))mode=0x40;
        else if(!strcmp(argv[1],"size"))size=127;
        else if(!strcmp(argv[1],"null"))buffer=NULL;
        else if(!strcmp(argv[1],"other"))stream=&recon_iob[2];
        else if(!strcmp(argv[1],"pending"))recon_iob[1]._cnt=1;
        else if(!strcmp(argv[1],"device"))recon_osfile[1]|=0x40;
        else if(!strcmp(argv[1],"wrap"))buffer=(char *)0xffffffc0U;
        else if(!strncmp(argv[1],"wide",4)) {
            size=256;assert(!recon_watcom_setvbuf(stream,buffer,mode,size));
            if(!strcmp(argv[1],"widepending")){assert(recon_fputc('X',stream)=='X');buffer=replacement+8;}
            else if(!strcmp(argv[1],"wideoverlap"))++buffer;
            else if(!strcmp(argv[1],"widesize")){size=128;buffer=replacement+8;}
            else assert(!strcmp(argv[1],"widesame"));
        }
        else if(!strcmp(argv[1],"repeat") || !strcmp(argv[1],"changed") || !strcmp(argv[1],"changedsize")) {
            assert(!recon_watcom_setvbuf(stream,buffer,mode,size));
            if(!strcmp(argv[1],"changedsize")){stream->_bufsiz=256;recon_fputc('x',stream);return 42;}
            if(!strcmp(argv[1],"changed")){stream->_base=(char *)1;recon_fputc('x',stream);return 42;}
        } else assert(0);
        recon_watcom_setvbuf(stream,buffer,mode,size);return 42;
    }
    row=argv[1][0]-'0';
    first=open("aux-sentinel",O_RDWR|O_CREAT|O_TRUNC,0600);
    second=open("prn-sentinel",O_RDWR|O_CREAT|O_TRUNC,0600);
    assert(first==3 && second==4 && write(first,"A",1)==1 && write(second,"P",1)==1);
    memcpy(others,recon_iob+3,sizeof(others));memcpy(lowio,recon_osfile,sizeof(lowio));
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
    fp(before);assert(!recon_watcom_setvbuf(stream,buffer,mode,size));fp(after);
    assert(!memcmp(before,after,108) && errno==(int)errors[row][0] && recon_doserrno==errors[row][1] && recon_GetLastError()==errors[row][2]);
    assert(stream->_base==buffer && stream->_ptr==buffer && stream->_bufsiz==(int)size && !stream->_cnt && stream->_flag==0x102);
    if(wide) {
        buffer=replacement+8;
        fp(before);assert(!recon_watcom_setvbuf(stream,buffer,mode,size));fp(after);
        assert(!memcmp(before,after,108) && errno==(int)errors[row][0] && recon_doserrno==errors[row][1] && recon_GetLastError()==errors[row][2]);
        assert(stream->_base==buffer && stream->_ptr==buffer && stream->_bufsiz==256 && !stream->_cnt);
        for(n=0;n<sizeof(guarded);++n)assert((unsigned char)guarded[n]==0xa5);
    }
    assert(recon_fclose(recon_iob+3)==EOF && recon_fclose(recon_iob+4)==EOF);
    assert(!memcmp(others,recon_iob+3,sizeof(others)) && !memcmp(lowio,recon_osfile,sizeof(lowio)));
    assert(pread(first,&sentinel,1,0)==1 && sentinel=='A' && pread(second,&sentinel,1,0)==1 && sentinel=='P');
    assert(!close(first) && !close(second));visible("",0);
    fp(before);assert(recon_fputs("ab",stream)==0);fp(after);
    assert(!memcmp(before,after,108) && stream->_ptr==buffer+2 && stream->_cnt==(int)size-2 && !memcmp(buffer,"ab",2));visible("",0);
    assert(recon_fputc('c',stream)=='c' && recon_fputc('\n',stream)=='\n');
    strcpy(expected,"abc\r\n");visible(expected,5);assert(stream->_ptr==buffer && !stream->_cnt);
    memset(buffer,0xa5,size);
    for(n=0;n<size;++n)assert(recon_fputc('A',stream)=='A');
    visible(expected,5);assert(stream->_ptr==buffer+size && !stream->_cnt);
    assert(recon_fputc('B',stream)=='B');memset(expected+5,'A',size);visible(expected,5+size);
    assert(stream->_ptr==buffer+1 && *buffer=='B' && recon_fputc('\n',stream)=='\n');
    memcpy(expected+5+size,"B\r\n",3);visible(expected,8+size);
    assert(!recon_fputs("tail",stream));visible(expected,8+size);
    assert(!recon_fflush(stream));memcpy(expected+8+size,"tail",4);visible(expected,12+size);
    assert(!recon_fputs("user",stream));
    fp(before);assert(!recon_fclose(stream));fp(after);assert(!memcmp(before,after,108));
    assert(!memcmp(buffer,"user",4) && !stream->_flag);
    for(n=0;n<8;++n)assert((unsigned char)buffer[-8+(int)n]==0xa5 && (unsigned char)buffer[size+n]==0xa5);
    if(wide)for(n=0;n<sizeof(guarded);++n)assert((unsigned char)original[(int)n-8]==0xa5);
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}
