#include <stdio.h>
#include <io.h>
#include <recon.h>
static void show(const char *where,FILE *file)
{
    long logical=ftell(file),physical=_tell(_fileno(file));
    printf("%s flag=%x ptr=%d cnt=%d size=%d logical=%ld physical=%ld\n",where,file->_flag,file->_base?(int)(file->_ptr-file->_base):0,file->_cnt,file->_bufsiz,logical,physical);
}
int main(void)
{
    FILE *file=fopen("input.bin","rb");char data[8];if(!file)return 3;
    show("open",file);if(fread(data,1,8,file)!=8)return 4;show("read",file);
    if(fseek(file,40,SEEK_SET))return 5;show("seek",file);
    if(fread(data,1,8,file)!=8)return 6;show("refill",file);
    if(fseek(file,2,SEEK_CUR))return 7;show("current",file);
    if(fgetc(file)!='a')return 8;show("byte",file);
    if(setvbuf(file,NULL,_IOFBF,1024))return 9;show("explicit",file);
    if(fseek(file,100,SEEK_SET))return 10;show("explicit-seek",file);
    if(fread(data,1,8,file)!=8)return 11;show("explicit-read",file);
    return fclose(file)!=0;
}
