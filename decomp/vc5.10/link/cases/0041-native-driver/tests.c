/* Native test glue: validate raw output, then derive a clock-only comparison.
 * Durable qualification bytes are copied by the external GDB workflow before
 * this callback. Its supervisor-root raw copy is ephemeral. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <recon-output.h>

static uint32_t image_u32(const unsigned char *p)
{
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void image_put32(unsigned char *p,uint32_t value)
{
    p[0]=(unsigned char)value;p[1]=(unsigned char)(value>>8);
    p[2]=(unsigned char)(value>>16);p[3]=(unsigned char)(value>>24);
}
static uint32_t image_checksum(const unsigned char *bytes)
{
    uint32_t sum=0;
    unsigned i;
    for(i=0;i<4096;i+=2) {
        if(i==264 || i==266)continue;
        sum+=(uint32_t)bytes[i]|(uint32_t)bytes[i+1]<<8;
        sum=(sum&0xffff)+(sum>>16);
    }
    sum=(sum&0xffff)+(sum>>16);
    return sum+4096;
}
int check_native_driver_outputs(const ReconOutputContext *context)
{
    unsigned char bytes[4096];
    char path[4096],raw[4096];
    const char *leaf;
    FILE *file;
    size_t prefix;
    int length,okay,closed;
    if(!context || context->struct_size!=sizeof(*context) ||
        context->terminal_kind!=RECON_OUTPUT_EXIT || context->terminal_value ||
        !context->workspace_cwd || !context->stdout_raw)return 0;
    length=snprintf(path,sizeof(path),"%s/native.sys",context->workspace_cwd);
    if(length<0 || length>=(int)sizeof(path))return 0;
    file=fopen(path,"rb");
    if(!file)return 0;
    okay=fread(bytes,1,sizeof(bytes),file)==sizeof(bytes) && fgetc(file)==EOF && !ferror(file);
    closed=fclose(file);
    if(!okay || closed)return 0;
    if(memcmp(bytes,"MZ",2) || image_u32(bytes+60)!=176 ||
        memcmp(bytes+176,"PE\0\0",4) || image_u32(bytes+264)!=image_checksum(bytes) ||
        image_u32(bytes+184)!=image_u32(bytes+1540) ||
        image_u32(bytes+184)!=image_u32(bytes+1568))return 0;
    leaf=strrchr(context->stdout_raw,'/');
    if(!leaf || strcmp(leaf+1,"stdout"))return 0;
    prefix=(size_t)(leaf+1-context->stdout_raw);
    if(prefix+sizeof("native-driver-image.raw")>sizeof(raw))return 0;
    memcpy(raw,context->stdout_raw,prefix);
    memcpy(raw+prefix,"native-driver-image.raw",sizeof("native-driver-image.raw"));
    file=fopen(raw,"wbx");
    if(!file)return 0;
    okay=fwrite(bytes,1,sizeof(bytes),file)==sizeof(bytes);
    closed=fclose(file);
    if(!okay || closed)return 0;
    /* Only the identified COFF/debug clocks and their computed checksum. */
    memset(bytes+184,0,4);memset(bytes+1540,0,4);memset(bytes+1568,0,4);
    image_put32(bytes+264,image_checksum(bytes));
    file=fopen(path,"wb");
    if(!file)return 0;
    okay=fwrite(bytes,1,sizeof(bytes),file)==sizeof(bytes);
    closed=fclose(file);
    return okay && !closed;
}
