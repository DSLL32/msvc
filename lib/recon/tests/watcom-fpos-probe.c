#include <assert.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    typedef char Position4[(sizeof(fpos_t)==4)?1:-1];
    int (*get)(FILE *,fpos_t *)=fgetpos;
    int (*set)(FILE *,const fpos_t *)=fsetpos;
    struct { unsigned before;fpos_t value;unsigned after; } position;
    FILE *stream;char bytes[8];unsigned row;unsigned char before[108],after[108];
    int text=argc>1 && !strcmp(argv[1],"text");
    (void)sizeof(Position4);assert(sizeof(position)==12 && get==recon_watcom_fgetpos && set==recon_watcom_fsetpos);
    stream=fopen("input.dat",text?"rt":"rb");assert(stream);
    position.before=0x12345678;position.after=0xabcdef01;position.value=0;
    if(argc>1 && !text && strcmp(argv[1],"live")) {
        if(!strcmp(argv[1],"null"))get(stream,NULL);
        if(!strcmp(argv[1],"stream"))get((FILE *)1,&position.value);
        if(!strcmp(argv[1],"wrap"))get(stream,(fpos_t *)0xfffffffcU);
        if(!strcmp(argv[1],"unaligned"))get(stream,(fpos_t *)((char *)&position.value+1));
        if(!strcmp(argv[1],"alias"))get(stream,(fpos_t *)stream);
        if(!strcmp(argv[1],"buffer")){assert(fgetc(stream)=='A');get(stream,(fpos_t *)stream->_base);}
        if(!strcmp(argv[1],"negative")){position.value=-1;set(stream,&position.value);}
        if(!strcmp(argv[1],"closed")){assert(!fclose(stream));get(stream,&position.value);}
        if(!strcmp(argv[1],"text-buffered")){assert(!fclose(stream));stream=fopen("input.dat","rt");assert(stream && fgetc(stream)=='A');get(stream,&position.value);}
        return 42;
    }
    if(argc>1 && !text)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    errno=111;fp(before);assert(!get(stream,&position.value));fp(after);
    assert(position.value==0 && !memcmp(before,after,108) && errno==111);
    for(row=0;row<3;++row) {
        assert(!memcmp(fgets(bytes,6,stream),"ABCDE",5));
        if(!text) {
            fp(before);assert(!get(stream,&position.value));fp(after);
            assert(position.value==5 && !memcmp(before,after,108));
        }
        position.value=0;fp(before);assert(!set(stream,&position.value));fp(after);
        assert(!memcmp(before,after,108) && errno==111 && position.value==0);
        assert(position.before==0x12345678 && position.after==0xabcdef01);
    }
    assert(!fclose(stream));
    if(argc>1 && !text)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
#else
#ifdef fgetpos
#error Watcom fgetpos alias leaked
#endif
#ifdef fpos_t
#error Watcom fpos type alias leaked
#endif
    (void)argc;(void)argv;recon_watcom_fgetpos(NULL,NULL);return 42;
#endif
}
