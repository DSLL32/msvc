#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stddef.h>
#include "command.h"
#include "resources.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(memcpy)
#endif

DWORD __stdcall rc_mac_long(DWORD value)
{
    return ((value&0x000000ff)<<24)|((value&0x0000ff00)<<8)|
           ((value&0x00ff0000)>>8)|((value&0xff000000)>>24);
}

DWORD __stdcall rc_mac_color_brightness(const WORD *rgb)
{
    return 30*(rgb[0]>>8)+59*(rgb[1]>>8)+11*(rgb[2]>>8);
}

void __stdcall rc_reduce_cursor_width(const BYTE *input,BYTE *output,
                                      int stride,int height)
{
    unsigned groups,group,pair,value;
    BYTE packed=0;
    if(stride<2||height<=0||(unsigned)(stride/2)>65535){ __asm int 3 }
    groups=stride/2;
    do{
        for(group=0;group<groups;++group){
            value=((unsigned)input[0]<<8)|input[1];
            input+=2;
            for(pair=0;pair<8;++pair){
                packed>>=1;
                if((value&3)==3)packed+=0x80;
                value>>=2;
            }
            *output++=packed;
        }
        output+=groups&1;
    }while(--height>0);
}

void __stdcall rc_reduce_cursor_height(const BYTE *input,BYTE *output,
                                       int stride,int height,int scale)
{
    int rows,words,row;
    WORD value;
    const BYTE *next;
    if(stride<2||height<=0||scale<=0){ __asm int 3 }
    if(scale<=1){
        memcpy(output,input,height*stride);
        return;
    }
    rows=height/scale;
    if(!rows){ __asm int 3 }
    do{
        words=(unsigned)stride>>1;
        do{
            value=*(const WORD *)input;
            next=input;
            row=scale-1;
            do{
                next+=stride;
                value&=*(const WORD *)next;
            }while(--row>0);
            *(WORD *)output=value;
            output+=2;input+=2;
        }while(--words>0);
        input+=stride*(scale-1);
    }while(--rows>0);
}

void __stdcall rc_mac_group_icon(RcResourceType *unused_type,RcResource *resource)
{
    RcImageDirectory directory;
    RcImageFileEntry entry;
    RcBitmapInfo bitmap;
    long entries[9],position;
    unsigned available=0,index,kind;
    int i;

    if(rc_read_bytes(rc_resource_data_file,&directory,6)!=6){ __asm int 3 }
    if(directory.reserved||directory.kind!=1)
        rc_resource_file_fatal(2175,rc_resource_name);
    for(i=0;i<9;++i)entries[i]=0;
    position=rc_seek(rc_resource_data_file,0,SEEK_CUR);
    for(index=0;index<directory.count;++index){
        rc_seek(rc_resource_data_file,position,SEEK_SET);
        if(rc_read_bytes(rc_resource_data_file,&entry,16)!=16){ __asm int 3 }
        position=rc_seek(rc_resource_data_file,0,SEEK_CUR);
        rc_seek(rc_resource_data_file,entry.offset,SEEK_SET);
        if(rc_read_bytes(rc_resource_data_file,&bitmap,40)!=40){ __asm int 3 }
        if(bitmap.size!=40)rc_resource_file_fatal(2176,rc_resource_name);
        if(bitmap.planes!=1)continue;
        if(bitmap.bit_count==1)kind=0;
        else if(bitmap.bit_count==4)kind=1;
        else if(bitmap.bit_count==8)kind=2;
        else continue;
        if(bitmap.width==16&&bitmap.height==24)kind+=6;
        else if(bitmap.width==16&&bitmap.height==32)kind+=3;
        else if(bitmap.width!=32||bitmap.height!=64)continue;
        available|=1<<kind;
        entries[kind]=position-16;
    }
    if(!available)rc_resource_file_warning(4508,rc_resource_name);
    else{
        if(available&7)rc_build_mac_icon(resource,0,entries[0],entries[1],entries[2]);
        if(available&0x38)rc_build_mac_icon(resource,3,entries[3],entries[4],entries[5]);
        /* Original small12-high branch reuses the3..5 entries, rather than6..8. */
        if(available&0x1c0)rc_build_mac_icon(resource,6,entries[3],entries[4],entries[5]);
    }
    fclose(rc_resource_data_file);
    rc_resource_data_file=NULL;
}

/* Classic Mac ColorSpec: palette value followed by16-bit RGB channels.
 * These are the bounded2/16/34 rows searched by the image loader. */
const RcMacColor rc_mac_colors_mono[2]={
    {0,65535,65535,65535},{1,0,0,0}
};
const RcMacColor rc_mac_colors4[16]={
    {0,65535,65535,65535},{1,64512,62333,1327},{2,65535,25738,652},
    {3,56683,2242,1698},{4,62167,2134,34028},{5,18147,0,42302},
    {6,0,0,54272},{7,577,43860,60159},{8,7969,46995,5169},
    {9,0,25775,4528},{10,22016,11421,1316},{11,37079,29024,14900},
    {12,49152,49152,49152},{13,32768,32768,32768},
    {14,16384,16384,16384},{15,0,0,0}
};
const RcMacColor rc_mac_colors8[34]={
    {1,65535,65535,52428},{8,65535,52428,39321},{51,52428,39321,26214},
    {22,65535,26214,13107},{146,13107,65535,39321},{227,0,48059,0},
    {159,13107,39321,26214},{165,13107,26214,26214},
    {72,39321,65535,65535},{192,0,39321,65535},{236,0,0,56797},
    {176,13107,0,39321},{42,52428,52428,65535},{84,39321,39321,65535},
    {127,26214,26214,52428},{171,13107,13107,26214},
    {19,65535,26214,52428},{105,39321,0,26214},{92,39321,26214,39321},
    {0,65535,65535,65535},{245,61166,61166,61166},
    {246,56797,56797,56797},{43,52428,52428,52428},
    {247,48059,48059,48059},{248,43690,43690,43690},
    {249,34952,34952,34952},{250,30583,30583,30583},
    {251,21845,21845,21845},{252,17476,17476,17476},
    {253,8738,8738,8738},{254,4369,4369,4369},{255,0,0,0},
    {5,65535,65535,0},{216,56797,0,0}
};
static const DWORD rc_mac_icon_types[9]={
    0x49434e23,0x69636c34,0x69636c38, /* ICN#, icl4, icl8 */
    0x69637323,0x69637334,0x69637338, /* ics#, ics4, ics8 */
    0x69636d23,0x69636d34,0x69636d38  /* icm#, icm4, icm8 */
};

void __stdcall rc_build_mac_icon(RcResource *resource,int kind,
                                 long mono,long color4,long color8)
{
    RcImageFileEntry entry;
    RcBitmapInfo bitmap;
    RcMacRgbQuad *palette;
    BYTE *bits,*input,*output,carry=0,mask;
    int stride,row,column,bytes,mask_stride,width_bytes,i,index;

    if(kind!=0&&kind!=3&&kind!=6){ __asm int 3 }
    if(mono){
        rc_load_mac_icon_image(mono,&entry,&bitmap,&stride,&bits,&palette,1);
        if(palette[0].index)
            for(i=stride*bitmap.height/2-1;i>=0;--i)bits[i]=~bits[i];
    }else if(color4){
        rc_load_mac_icon_image(color4,&entry,&bitmap,&stride,&bits,&palette,1);
        for(i=0;i<(int)bitmap.colors_used;++i){
            index=palette[i].index;
            if(index>=16){ __asm int 3 }
            palette[i].index=rc_mac_color_brightness(&rc_mac_colors4[index].red)<12800;
        }
        carry=(BYTE)color4;
        for(row=0;row<bitmap.height/2;++row){
            input=output=bits+row*stride;
            for(column=0;column<stride;++column){
                mask=*input++;
                carry=(BYTE)((carry<<1)|palette[mask>>4].index);
                carry=(BYTE)((carry<<1)|palette[mask&15].index);
                if(!((column+1)%4))*output++=carry;
            }
        }
    }else{
        rc_load_mac_icon_image(color8,&entry,&bitmap,&stride,&bits,&palette,1);
        for(i=0;i<(int)bitmap.colors_used;++i){
            index=palette[i].index;
            /* The original indexes beyond these34 rows for some mapped values.
             * That adjacent storage is unavailable in this reconstruction. */
            if(index>=34){ __asm int 3 }
            palette[i].index=rc_mac_color_brightness(&rc_mac_colors8[index].red)<12800;
        }
        carry=(BYTE)color4;
        for(row=0;row<bitmap.height/2;++row){
            input=output=bits+row*stride;
            for(column=0;column<stride;++column){
                carry=(BYTE)((carry<<1)|palette[*input++].index);
                if(!((column+1)%8))*output++=carry;
            }
        }
    }
    width_bytes=bitmap.width/8;
    mask_stride=(bitmap.width+31)/32*4;
    rc_repack_mac_icon_bits(bits,stride,mask_stride,width_bytes,width_bytes,
                           bitmap.height/2);
    bytes=width_bytes*bitmap.height/2;
    for(i=0;i<bytes;++i)bits[i]^=(BYTE)~bits[bytes+i];
    rc_write_mac_icon_resource(bits,width_bytes*bitmap.height,resource,
                               rc_mac_icon_types[kind]);
    rc_release(bits);rc_release(palette);
    if(color4){
        rc_load_mac_icon_image(color4,&entry,&bitmap,&stride,&bits,&palette,1);
        bytes=stride*bitmap.height/2;
        for(i=0;i<bytes;++i){
            mask=bits[i];
            bits[i]=(BYTE)((palette[mask>>4].index<<4)|palette[mask&15].index);
        }
        rc_repack_mac_icon_bits(bits,stride,(bitmap.width+31)/32*4,bitmap.width/2,
                               bitmap.width/8,bitmap.height/2);
        bytes=(bitmap.width/2)*(bitmap.height/2);
        input=bits+bytes;
        for(i=0;i<bytes;++i){
            if(!(i%4))mask=*input++;
            if(!(mask&0x80))bits[i]^=0xf0;
            if(!(mask&0x40))bits[i]^=0x0f;
            mask<<=2;
        }
        rc_write_mac_icon_resource(bits,bytes,resource,rc_mac_icon_types[kind+1]);
        rc_release(bits);rc_release(palette);
    }
    if(color8){
        rc_load_mac_icon_image(color8,&entry,&bitmap,&stride,&bits,&palette,1);
        bytes=stride*bitmap.height/2;
        for(i=0;i<bytes;++i)bits[i]=palette[bits[i]].index;
        rc_repack_mac_icon_bits(bits,stride,(bitmap.width+31)/32*4,bitmap.width,
                               bitmap.width/8,bitmap.height/2);
        bytes=bitmap.width*(bitmap.height/2);
        input=bits+bytes;
        for(i=0;i<bytes;++i){
            if(!(i%8))mask=*input++;
            if(!(mask&0x80))bits[i]^=0xff;
            mask<<=1;
        }
        rc_write_mac_icon_resource(bits,bytes,resource,rc_mac_icon_types[kind+2]);
        rc_release(bits);rc_release(palette);
    }
}

unsigned __stdcall rc_load_mac_icon_image(long offset,RcImageFileEntry *entry,
 RcBitmapInfo *bitmap,int *stride,BYTE **bits,RcMacRgbQuad **palette,int has_mask)
{
    WORD bytes;
    int i;
    DWORD count;
    int row_bytes,total;
    rc_seek(rc_resource_data_file,offset,SEEK_SET);
    if(rc_read_bytes(rc_resource_data_file,entry,16)!=16){ __asm int 3 }
    rc_seek(rc_resource_data_file,entry->offset,SEEK_SET);
    if(rc_read_bytes(rc_resource_data_file,bitmap,40)!=40){ __asm int 3 }
    if(!bitmap->colors_used)bitmap->colors_used=1u<<(bitmap->bit_count&31);
    bytes=(WORD)(bitmap->colors_used*4);
    *palette=rc_allocate(bytes);
    if(rc_read_bytes(rc_resource_data_file,*palette,bytes)!=bytes){ __asm int 3 }
    switch(bitmap->bit_count){
    case 8:
        for(i=0;i<(int)bitmap->colors_used;++i)
            rc_map_mac_color(rc_mac_colors8,34,*palette+i);
        break;
    case 4:
        for(i=0;i<(int)bitmap->colors_used;++i)
            rc_map_mac_color(rc_mac_colors4,16,*palette+i);
        break;
    case 1:
        for(i=0;i<(int)bitmap->colors_used;++i)
            rc_map_mac_color(rc_mac_colors_mono,2,*palette+i);
        break;
    }
    row_bytes=(bitmap->width*bitmap->bit_count+31)/32*4;
    *stride=row_bytes;
    if(has_mask)
        total=row_bytes*(bitmap->height/2)+
              (bitmap->width+31)/32*(bitmap->height/2)*4;
    else total=row_bytes*bitmap->height;
    bytes=(WORD)total;
    *bits=rc_allocate(bytes);
    count=rc_read_bytes(rc_resource_data_file,*bits,bytes);
    if(count!=bytes){ __asm int 3 }
    return count;
}

void __stdcall rc_map_mac_color(const RcMacColor *colors,int count,
                                RcMacRgbQuad *color)
{
    int i,closest=-1,distance,best=32767,red,green,blue;
    if(count<=0){ __asm int 3 }
    for(i=0;i<count;++i){
        green=(int)color->green-(colors[i].green>>8);
        blue=(int)color->blue-(colors[i].blue>>8);
        red=(int)color->red-(colors[i].red>>8);
        distance=(green<0?-green:green)+(blue<0?-blue:blue)+
                 (red<0?-red:red);
        if(distance<best){
            best=distance;closest=i;
            if(!distance)break;
        }
    }
    color->index=(BYTE)colors[closest].value;
}

void __stdcall rc_repack_mac_icon_bits(BYTE *bits,int stride,int mask_stride,
 int output_stride,int output_mask,int rows)
{
    BYTE *temporary,*out,*in;
    int row,column,total;
    if(rows<=0||stride<=0||mask_stride<=0||output_stride<0||output_mask<0||
       output_stride>stride||output_mask>mask_stride||
       mask_stride>65535||output_stride>65535-mask_stride||
       rows>65535/(mask_stride+output_stride)){ __asm int 3 }
    total=(mask_stride+output_stride)*rows;
    temporary=rc_allocate((WORD)total);
    out=temporary;
    for(row=rows-1;row>=0;--row){
        memcpy(out,bits+row*stride,output_stride);
        out+=output_stride;
    }
    for(row=rows-1;row>=0;--row){
        in=bits+rows*stride+row*mask_stride;
        for(column=0;column<output_mask;++column)*out++=(BYTE)~*in++;
    }
    /* Original allocation/copy uses mask_stride, although each emitted mask
     * row copies only output_mask bytes. Unwritten temporary bytes stay zero
     * under the actual zeroed allocation contract. */
    memcpy(bits,temporary,total);
    rc_release(temporary);
}

void __stdcall rc_write_mac_icon_resource(const BYTE *bits,int bytes,
                                         RcResource *resource,DWORD code)
{
    WCHAR name[5];
    RcResourceType *type;
    RcResource *copy;
    name[0]=(WCHAR)(signed char)(code>>24);
    name[1]=(WCHAR)(signed char)(code>>16);
    name[2]=(WCHAR)(signed char)(code>>8);
    name[3]=(WCHAR)(signed char)code;
    name[4]=0;
    type=rc_find_resource_type(name,0);
    copy=rc_allocate(sizeof(*copy));
    *copy=*resource;
    copy->size=bytes;
    rc_append_resource(type,copy,bits,(WORD)bytes,0);
}

void __stdcall rc_mac_group_cursor(RcResourceType *type,RcResource *resource)
{
    RcImageDirectory directory;
    RcImageFileEntry entry;
    RcBitmapInfo bitmap;
    RcMacRgbQuad *palette;
    BYTE *bits;
    WORD mask_words[16],data_words[16],word;
    long position,selected=-1;
    int best=32767,index,score,stride,rows,i;
    extern int rc_mac;

    if(rc_read_bytes(rc_resource_data_file,&directory,6)!=6){ __asm int 3 }
    if(directory.reserved||directory.kind!=2)
        rc_resource_file_fatal(2175,rc_resource_name);
    position=rc_seek(rc_resource_data_file,0,SEEK_CUR);
    for(index=0;index<directory.count;++index){
        rc_seek(rc_resource_data_file,position,SEEK_SET);
        if(rc_read_bytes(rc_resource_data_file,&entry,16)!=16){ __asm int 3 }
        position=rc_seek(rc_resource_data_file,0,SEEK_CUR);
        rc_seek(rc_resource_data_file,entry.offset,SEEK_SET);
        if(rc_read_bytes(rc_resource_data_file,&bitmap,40)!=40){ __asm int 3 }
        if(bitmap.size!=40)rc_resource_file_fatal(2176,rc_resource_name);
        if(bitmap.bit_count!=1||bitmap.planes!=1||bitmap.width%16||bitmap.height%32)
            continue;
        if(bitmap.width/16>2)continue;
        score=(bitmap.width/16)*(bitmap.height/32);
        if(score<best){best=score;selected=position-16;}
    }
    if(selected==-1){
        rc_resource_file_warning(4507,rc_resource_name);
        return;
    }
    rc_load_mac_icon_image(selected,&entry,&bitmap,&stride,&bits,&palette,0);
    if(palette[0].red==255&&palette[0].green==255&&palette[0].blue==255){
        /* Original starts this inversion at bits[stride*height], one byte past
         * its initialized allocation. That defective branch is unsupported. */
        __asm int 3
        for(i=stride*bitmap.height;i>0;--i)bits[i]=(BYTE)~bits[i];
    }
    if(best!=1){
        rc_resource_file_warning(4506,rc_resource_name);
        if(bitmap.width>16){
            entry.planes_or_x=(WORD)(entry.planes_or_x/(bitmap.width/16));
            rc_reduce_cursor_width(bits,bits,stride,bitmap.height);
            stride=2;
        }
        if(bitmap.height>32){
            score=bitmap.height/32;
            entry.bits_or_y=(WORD)(entry.bits_or_y/score);
            rc_reduce_cursor_height(bits,bits,stride,bitmap.height,score);
            bitmap.height=32;
        }
    }
    rows=bitmap.height/2;
    if(rows<0||rows>16||stride<=0){ __asm int 3 }
    for(i=0;i<rows;++i){
        if(stride==1){
            mask_words[i]=(WORD)(signed char)bits[rows-i-1];
            data_words[i]=(WORD)((signed char)bits[rows*2-i-1]^~mask_words[i]);
        }else{
            mask_words[i]=(WORD)~*(WORD *)(bits+stride*(rows*2-i-1));
            data_words[i]=*(WORD *)(bits+stride*(rows-i-1))^mask_words[i];
        }
    }
    for(i=rows;i<16;++i){mask_words[i]=0;data_words[i]=0;}
    rc_append_resource_bytes(data_words,32);
    rc_append_resource_bytes(mask_words,32);
    word=rc_mac?(WORD)rc_mac_word(entry.bits_or_y):entry.bits_or_y;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    word=rc_mac?(WORD)rc_mac_word(entry.planes_or_x):entry.planes_or_x;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    resource->size=68;
    rc_release(bits);rc_release(palette);
    fclose(rc_resource_data_file);rc_resource_data_file=NULL;
    rc_append_resource(type,resource,rc_resource_buffer,rc_resource_buffer_used,0);
}

unsigned __stdcall rc_mac_word(WORD value)
{
    return (value>>8)|((value&0xff)<<8);
}

/* The five actual1250..1254 mappings and loader-zero validation flags. */
static const UINT rc_mac_codepages[5]={10029,10007,10000,10006,10081};
DWORD rc_mac_codepage_flags[5];

void __stdcall rc_write_mac_string(const WCHAR *text,int map_page,int counted)
{
    char local[256],expanded[256],*bytes=local,*wide=expanded;
    UINT page;
    int fallback=0,length=0,characters,index,size;
    if(text){
        page=rc_codepage;
        if(map_page&&rc_codepage>=1250&&rc_codepage<=1254){
            index=rc_codepage-1250;
            page=rc_mac_codepages[index];
            if(!(rc_mac_codepage_flags[index]&1)){
                rc_mac_codepage_flags[index]|=1;
                if(IsValidCodePage(page))rc_mac_codepage_flags[index]|=2;
            }
            if(!(rc_mac_codepage_flags[index]&2)){page=rc_codepage;fallback=1;}
        }
        characters=wcslen(text);
        length=WideCharToMultiByte(page,0,text,characters,NULL,0,NULL,NULL);
        if(length>256)bytes=rc_allocate(length);
        WideCharToMultiByte(page,0,text,characters,bytes,length,NULL,NULL);
        if(fallback)rc_map_mac_bytes(bytes,length);
    }
    if(counted){
        /* The original still copies its local buffer when conversion allocated
         * a larger one. Do not read undefined local storage on that branch. */
        if(length>256){ __asm int 3 }
        *(BYTE *)rc_reserve_resource_bytes(1)=(BYTE)length;
        rc_append_resource_bytes(local,length);
    }else{
        if(length<0||length>0x3ffffffe){ __asm int 3 }
        size=length*2+2;
        if(size>256)wide=rc_allocate(size);
        size=rc_expand_mac_bytes(bytes,length,(BYTE *)wide);
        rc_append_resource_bytes(wide,size);
        if(wide!=expanded)rc_release(wide);
    }
    if(bytes!=local)rc_release(bytes);
}

int __stdcall rc_expand_mac_bytes(const char *text,int bytes,BYTE *out)
{
    int total=2;
    while(bytes>0){
        if(IsDBCSLeadByteEx(rc_codepage,(BYTE)*text)){
            if(bytes<2){ __asm int 3 }
            *out++=(BYTE)*text++;--bytes;
        }else *out++=0;
        *out++=(BYTE)*text++;--bytes;
        total+=2;
    }
    out[0]=0;out[1]=0;
    return total;
}


/* Macintosh resource fork header, type list, reference lists and Pascal names.
 * The sixty zero DWORDs after the four header fields fill the data offset256. */
void __cdecl rc_write_mac_resource_file(void)
{
    extern FILE *rc_output_file;
    extern int rc_mac;
    RcResourceType *type,*other;
    RcResource *resource;
    DWORD data_size,map_offset,map_size,value,fourcc;
    int types=0,resources=0,name_bytes=0,reference_offset,name_offset,i;
    WORD word;
    BYTE length;
    const WCHAR *name;
    char first[128],second[128],code[5];

    data_size=(DWORD)rc_seek(rc_output_file,0,SEEK_CUR)-256;
    for(type=rc_resource_types;type;type=type->next){
        if(!type->count)continue;
        ++types;resources+=(short)type->count;
        for(resource=type->resources;resource;resource=resource->next){
            if(!resource->id)resource->id=(WORD)rc_assign_mac_resource_id(type,resource);
            if(resource->name)name_bytes+=wcslen(resource->name)+1;
        }
    }
    rc_seek(rc_output_file,0,SEEK_SET);
    rc_reset_resource_buffer();
    value=rc_mac?rc_mac_long(256):256;
    *(DWORD *)rc_reserve_resource_bytes(4)=value;
    map_offset=data_size+256;
    value=rc_mac?rc_mac_long(map_offset):map_offset;
    *(DWORD *)rc_reserve_resource_bytes(4)=value;
    value=rc_mac?rc_mac_long(data_size):data_size;
    *(DWORD *)rc_reserve_resource_bytes(4)=value;
    map_size=30+types*8+resources*12+name_bytes;
    value=rc_mac?rc_mac_long(map_size):map_size;
    *(DWORD *)rc_reserve_resource_bytes(4)=value;
    for(i=0;i<60;++i){
        value=rc_mac?rc_mac_long(0):0;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
    }
    rc_write_bytes(rc_output_file,rc_resource_buffer,rc_resource_buffer_used);
    rc_seek(rc_output_file,map_offset,SEEK_SET);
    rc_reset_resource_buffer();
    for(i=0;i<6;++i){
        value=rc_mac?rc_mac_long(0):0;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
    }
    word=rc_mac?(WORD)rc_mac_word(28):28;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    word=rc_mac?(WORD)rc_mac_word((WORD)(30+types*8+resources*12)):
                (WORD)(30+types*8+resources*12);
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    word=rc_mac?(WORD)rc_mac_word((WORD)(types-1)):(WORD)(types-1);
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    reference_offset=types*8+2;
    for(type=rc_resource_types;type;type=type->next){
        if(!type->count)continue;
        fourcc=rc_mac_resource_type(type->name,type->id);
        for(other=type->next;other;other=other->next){
            if(fourcc!=rc_mac_resource_type(other->name,other->id))continue;
            code[0]=(char)(fourcc>>24);code[1]=(char)(fourcc>>16);
            code[2]=(char)(fourcc>>8);code[3]=(char)fourcc;code[4]=0;
            if(type->id)wsprintfA(first,"%d",(unsigned)type->id);
            else wsprintfA(first,"%ws",type->name);
            if(other->id)wsprintfA(second,"%d",(unsigned)other->id);
            else wsprintfA(second,"%ws",other->name);
            rc_resource_notice_strings(4509,first,second,(DWORD)code);
        }
        value=rc_mac?rc_mac_long(fourcc):fourcc;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
        word=rc_mac?(WORD)rc_mac_word((WORD)(type->count-1)):(WORD)(type->count-1);
        *(WORD *)rc_reserve_resource_bytes(2)=word;
        word=rc_mac?(WORD)rc_mac_word((WORD)reference_offset):(WORD)reference_offset;
        *(WORD *)rc_reserve_resource_bytes(2)=word;
        reference_offset+=(short)type->count*12;
    }
    name_offset=0;
    for(type=rc_resource_types;type;type=type->next){
        if(!type->count)continue;
        for(resource=type->resources;resource;resource=resource->next){
            word=rc_mac?(WORD)rc_mac_word(resource->id):resource->id;
            *(WORD *)rc_reserve_resource_bytes(2)=word;
            if(!resource->name)word=rc_mac?(WORD)rc_mac_word(0xffff):0xffff;
            else word=rc_mac?(WORD)rc_mac_word((WORD)name_offset):(WORD)name_offset;
            *(WORD *)rc_reserve_resource_bytes(2)=word;
            if(resource->name)name_offset+=wcslen(resource->name)+1;
            value=rc_mac?rc_mac_long(resource->data_offset):resource->data_offset;
            *(DWORD *)rc_reserve_resource_bytes(4)=value;
            value=rc_mac?rc_mac_long(0):0;
            *(DWORD *)rc_reserve_resource_bytes(4)=value;
        }
    }
    for(type=rc_resource_types;type;type=type->next){
        if(!type->count)continue;
        for(resource=type->resources;resource;resource=resource->next){
            if(!resource->name)continue;
            length=(BYTE)wcslen(resource->name);
            *(BYTE *)rc_reserve_resource_bytes(1)=length;
            name=resource->name;
            for(i=length;i;--i) *(BYTE *)rc_reserve_resource_bytes(1)=(BYTE)*name++;
        }
    }
    rc_write_bytes(rc_output_file,rc_resource_buffer,rc_resource_buffer_used);
}


/* /BR's WORD at RVA25054 is the starting Macintosh resource ID;
 * it is the existing rc_language storage, initialized to128 by rc_initialize. */
unsigned __stdcall rc_assign_mac_resource_id(RcResourceType *type,RcResource *resource)
{
    extern WORD rc_language;
    RcResourceType *other;
    RcResource *item;
    unsigned candidate;
    if(rc_is_mac_icon_type(type)){
        for(other=rc_resource_types;other;other=other->next){
            if(!rc_is_mac_icon_type(other))continue;
            for(item=other->resources;item;item=item->next){
                if(item->name&&item->id&&!_wcsicmp(item->name,resource->name))
                    return item->id;
            }
        }
        candidate=rc_language;
        for(;;){
            for(other=rc_resource_types;other;other=other->next){
                if(!rc_is_mac_icon_type(other))continue;
                for(item=other->resources;item;item=item->next)
                    if(item->id==candidate)break;
                if(item)break;
            }
            if(!other)return candidate;
            candidate=(candidate+1)&0xffff;
            if(!candidate)candidate=1;
        }
    }
    candidate=rc_language;
    for(;;){
        for(item=type->resources;item;item=item->next)
            if(item->id==candidate)break;
        if(!item)return candidate;
        candidate=(candidate+1)&0xffff;
        if(!candidate)candidate=1;
    }
}


int __stdcall rc_is_mac_icon_type(const RcResourceType *type)
{
    const WCHAR *name=type->name;
    DWORD value;
    int i;
    if(!name)return 0;
    /* The original consumes four code units unconditionally. A shorter owned
     * spelling cannot supply that initialized span and remains unsupported. */
    if(!name[0]||!name[1]||!name[2]){ __asm int 3 }
    value=((DWORD)(BYTE)name[0]<<24)|((DWORD)(BYTE)name[1]<<16)|
          ((DWORD)(BYTE)name[2]<<8)|(BYTE)name[3];
    for(i=0;i<9;++i)if(value==rc_mac_icon_types[i])return 1;
    return 0;
}


DWORD __stdcall rc_mac_resource_type(const WCHAR *name,WORD id)
{
    DWORD value;
    int i;
    switch((short)id){
    case 0:
        value=0;
        for(i=0;i<4;++i){
            value<<=8;
            if(*name)value|=(BYTE)*name++;
            else value|=' ';
        }
        return value;
    case 1: case 12: return 0x43555253; /* CURS */
    case 2: return 0x57424d50;          /* WBMP */
    case 3: return 0x5749434f;          /* WICO */
    case 4: return 0x574d4e55;          /* WMNU */
    case 5: return 0x57444c47;          /* WDLG */
    case 6: return 0x53545223;          /* STR# */
    case 9: return 0x57414343;          /* WACC */
    case 14: return 0x57474943;         /* WGIC */
    case 16: return 0x57564552;         /* WVER */
    case 240: case 10: return 0x48455841; /* HEXA */
    case 241: return 0x544c4252;        /* TLBR */
    }
    /* Other ordinal types are encoded as M.. plus three hexadecimal digits.
     * The first character is 'M'+the high nibble, just as in the original. */
    return ((DWORD)('M'+((id>>12)&15))<<24)|
           ((DWORD)(BYTE)"0123456789ABCDEF"[(id>>8)&15]<<16)|
           ((DWORD)(BYTE)"0123456789ABCDEF"[(id>>4)&15]<<8)|
           (BYTE)"0123456789ABCDEF"[id&15];
}

/* The contextual128-entry high-byte map selected only for codepage1252.
 * Literal values retain the original legacy substitutions, including '?'. */
static const BYTE rc_windows_to_mac[128]={
    0x3f, /* 80 EURO SIGN */
    0x3f, /* 81 UNDEFINED CP1252 BYTE */
    0xe2, /* 82 SINGLE LOW-9 QUOTATION MARK */
    0xc4, /* 83 LATIN SMALL LETTER F WITH HOOK */
    0xe3, /* 84 DOUBLE LOW-9 QUOTATION MARK */
    0xc9, /* 85 HORIZONTAL ELLIPSIS */
    0xa0, /* 86 DAGGER */
    0xe0, /* 87 DOUBLE DAGGER */
    0xf6, /* 88 MODIFIER LETTER CIRCUMFLEX ACCENT */
    0xe4, /* 89 PER MILLE SIGN */
    0x3f, /* 8A LATIN CAPITAL LETTER S WITH CARON */
    0xdc, /* 8B SINGLE LEFT-POINTING ANGLE QUOTATION MARK */
    0xce, /* 8C LATIN CAPITAL LIGATURE OE */
    0x3f, /* 8D UNDEFINED CP1252 BYTE */
    0x3f, /* 8E LATIN CAPITAL LETTER Z WITH CARON */
    0x3f, /* 8F UNDEFINED CP1252 BYTE */
    0x3f, /* 90 UNDEFINED CP1252 BYTE */
    0xd4, /* 91 LEFT SINGLE QUOTATION MARK */
    0xd5, /* 92 RIGHT SINGLE QUOTATION MARK */
    0xd2, /* 93 LEFT DOUBLE QUOTATION MARK */
    0xd3, /* 94 RIGHT DOUBLE QUOTATION MARK */
    0xa5, /* 95 BULLET */
    0xd0, /* 96 EN DASH */
    0xd1, /* 97 EM DASH */
    0xf7, /* 98 SMALL TILDE */
    0x84, /* 99 TRADE MARK SIGN */
    0x3f, /* 9A LATIN SMALL LETTER S WITH CARON */
    0xdd, /* 9B SINGLE RIGHT-POINTING ANGLE QUOTATION MARK */
    0xcf, /* 9C LATIN SMALL LIGATURE OE */
    0x3f, /* 9D UNDEFINED CP1252 BYTE */
    0x3f, /* 9E LATIN SMALL LETTER Z WITH CARON */
    0xd9, /* 9F LATIN CAPITAL LETTER Y WITH DIAERESIS */
    0xca, /* A0 NO-BREAK SPACE */
    0xc1, /* A1 INVERTED EXCLAMATION MARK */
    0xa2, /* A2 CENT SIGN */
    0xa3, /* A3 POUND SIGN */
    0xdb, /* A4 CURRENCY SIGN */
    0xb4, /* A5 YEN SIGN */
    0x3f, /* A6 BROKEN BAR */
    0xa4, /* A7 SECTION SIGN */
    0xac, /* A8 DIAERESIS */
    0xa9, /* A9 COPYRIGHT SIGN */
    0xbb, /* AA FEMININE ORDINAL INDICATOR */
    0xc7, /* AB LEFT-POINTING DOUBLE ANGLE QUOTATION MARK */
    0xc2, /* AC NOT SIGN */
    0x3f, /* AD SOFT HYPHEN */
    0xa8, /* AE REGISTERED SIGN */
    0x3f, /* AF MACRON */
    0xa1, /* B0 DEGREE SIGN */
    0xb1, /* B1 PLUS-MINUS SIGN */
    0x3f, /* B2 SUPERSCRIPT TWO */
    0x3f, /* B3 SUPERSCRIPT THREE */
    0xab, /* B4 ACUTE ACCENT */
    0xb5, /* B5 MICRO SIGN */
    0xa6, /* B6 PILCROW SIGN */
    0xe1, /* B7 MIDDLE DOT */
    0xfc, /* B8 CEDILLA */
    0x3f, /* B9 SUPERSCRIPT ONE */
    0xbc, /* BA MASCULINE ORDINAL INDICATOR */
    0xc8, /* BB RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK */
    0x3f, /* BC VULGAR FRACTION ONE QUARTER */
    0x3f, /* BD VULGAR FRACTION ONE HALF */
    0x3f, /* BE VULGAR FRACTION THREE QUARTERS */
    0xc0, /* BF INVERTED QUESTION MARK */
    0xcb, /* C0 LATIN CAPITAL LETTER A WITH GRAVE */
    0xe7, /* C1 LATIN CAPITAL LETTER A WITH ACUTE */
    0xe5, /* C2 LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
    0xcc, /* C3 LATIN CAPITAL LETTER A WITH TILDE */
    0x80, /* C4 LATIN CAPITAL LETTER A WITH DIAERESIS */
    0x81, /* C5 LATIN CAPITAL LETTER A WITH RING ABOVE */
    0xae, /* C6 LATIN CAPITAL LETTER AE */
    0x82, /* C7 LATIN CAPITAL LETTER C WITH CEDILLA */
    0xe9, /* C8 LATIN CAPITAL LETTER E WITH GRAVE */
    0x83, /* C9 LATIN CAPITAL LETTER E WITH ACUTE */
    0xe6, /* CA LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
    0xe8, /* CB LATIN CAPITAL LETTER E WITH DIAERESIS */
    0xed, /* CC LATIN CAPITAL LETTER I WITH GRAVE */
    0xea, /* CD LATIN CAPITAL LETTER I WITH ACUTE */
    0xeb, /* CE LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
    0xec, /* CF LATIN CAPITAL LETTER I WITH DIAERESIS */
    0x3f, /* D0 LATIN CAPITAL LETTER ETH */
    0x84, /* D1 LATIN CAPITAL LETTER N WITH TILDE */
    0xf1, /* D2 LATIN CAPITAL LETTER O WITH GRAVE */
    0xee, /* D3 LATIN CAPITAL LETTER O WITH ACUTE */
    0xef, /* D4 LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
    0xcd, /* D5 LATIN CAPITAL LETTER O WITH TILDE */
    0x85, /* D6 LATIN CAPITAL LETTER O WITH DIAERESIS */
    0x3f, /* D7 MULTIPLICATION SIGN */
    0xaf, /* D8 LATIN CAPITAL LETTER O WITH STROKE */
    0x84, /* D9 LATIN CAPITAL LETTER U WITH GRAVE */
    0xf2, /* DA LATIN CAPITAL LETTER U WITH ACUTE */
    0xf3, /* DB LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
    0x86, /* DC LATIN CAPITAL LETTER U WITH DIAERESIS */
    0x3f, /* DD LATIN CAPITAL LETTER Y WITH ACUTE */
    0x3f, /* DE LATIN CAPITAL LETTER THORN */
    0xa7, /* DF LATIN SMALL LETTER SHARP S */
    0x88, /* E0 LATIN SMALL LETTER A WITH GRAVE */
    0x87, /* E1 LATIN SMALL LETTER A WITH ACUTE */
    0x89, /* E2 LATIN SMALL LETTER A WITH CIRCUMFLEX */
    0x8b, /* E3 LATIN SMALL LETTER A WITH TILDE */
    0x8a, /* E4 LATIN SMALL LETTER A WITH DIAERESIS */
    0x8c, /* E5 LATIN SMALL LETTER A WITH RING ABOVE */
    0xbe, /* E6 LATIN SMALL LETTER AE */
    0x8d, /* E7 LATIN SMALL LETTER C WITH CEDILLA */
    0x8f, /* E8 LATIN SMALL LETTER E WITH GRAVE */
    0x8e, /* E9 LATIN SMALL LETTER E WITH ACUTE */
    0x90, /* EA LATIN SMALL LETTER E WITH CIRCUMFLEX */
    0x91, /* EB LATIN SMALL LETTER E WITH DIAERESIS */
    0x93, /* EC LATIN SMALL LETTER I WITH GRAVE */
    0x92, /* ED LATIN SMALL LETTER I WITH ACUTE */
    0x94, /* EE LATIN SMALL LETTER I WITH CIRCUMFLEX */
    0x95, /* EF LATIN SMALL LETTER I WITH DIAERESIS */
    0x3f, /* F0 LATIN SMALL LETTER ETH */
    0x96, /* F1 LATIN SMALL LETTER N WITH TILDE */
    0x98, /* F2 LATIN SMALL LETTER O WITH GRAVE */
    0x97, /* F3 LATIN SMALL LETTER O WITH ACUTE */
    0x99, /* F4 LATIN SMALL LETTER O WITH CIRCUMFLEX */
    0x9b, /* F5 LATIN SMALL LETTER O WITH TILDE */
    0x9a, /* F6 LATIN SMALL LETTER O WITH DIAERESIS */
    0xd6, /* F7 DIVISION SIGN */
    0xbf, /* F8 LATIN SMALL LETTER O WITH STROKE */
    0x9d, /* F9 LATIN SMALL LETTER U WITH GRAVE */
    0x9c, /* FA LATIN SMALL LETTER U WITH ACUTE */
    0x9e, /* FB LATIN SMALL LETTER U WITH CIRCUMFLEX */
    0x9f, /* FC LATIN SMALL LETTER U WITH DIAERESIS */
    0x3f, /* FD LATIN SMALL LETTER Y WITH ACUTE */
    0x3f, /* FE LATIN SMALL LETTER THORN */
    0xd8, /* FF LATIN SMALL LETTER Y WITH DIAERESIS */
};
const BYTE *rc_mac_byte_map;

void __stdcall rc_select_mac_byte_map(UINT page)
{
    rc_mac_byte_map=page==1252?rc_windows_to_mac:NULL;
}

void __stdcall rc_map_mac_bytes(char *text,int bytes)
{
    int index;
    unsigned char value;
    if(!rc_mac_byte_map)rc_select_mac_byte_map(rc_codepage);
    if(rc_mac_byte_map){
        for(index=0;index<bytes;++index){
            value=(unsigned char)text[index];
            if(value&0x80)text[index]=rc_mac_byte_map[value-128];
        }
    }
}

/* The encountered legacy icon record has a two-byte tag followed by these
 * twelve bytes. It is not the modern ICO directory/BITMAPINFOHEADER format. */
typedef struct RcLegacyIconShape {
    short hotspot_x,hotspot_y,width,height,row_bytes;
    BYTE planes,bits_per_pixel;
} RcLegacyIconShape;
typedef char RcLegacyIconShapeSize[sizeof(RcLegacyIconShape)==12?1:-1];
typedef char RcLegacyIconHeightOffset[offsetof(RcLegacyIconShape,height)==6?1:-1];
typedef char RcLegacyIconRowOffset[offsetof(RcLegacyIconShape,row_bytes)==8?1:-1];

long __stdcall rc_convert_icon_file(long size)
{
    extern int rc_mac;
    FILE *file=rc_resource_file(NULL);
    RcLegacyIconShape shape;
    WORD tag,word;
    long expected,position;
    int extra=0;
    if(!file)return 0;
    /* Short records leave original automatic bytes undefined. Hold before any
     * such bytes are consumed; valid typed record units supply all fourteen. */
    if(rc_read_bytes(file,&tag,2)!=2){ __asm int 3 }
    if((BYTE)tag!=1&&(BYTE)tag!=3)
        rc_resource_file_fatal(2169,rc_resource_name);
    if(rc_read_bytes(file,&shape,12)!=12){ __asm int 3 }
    expected=(long)shape.height*shape.row_bytes*2+14;
    if(expected<size){
        tag&=0x00ff;
        rc_seek(file,expected,SEEK_SET);
        word=rc_mac?(WORD)rc_mac_word(tag):tag;
        *(WORD *)rc_reserve_resource_bytes(2)=word;
    }else{
        tag=(WORD)((tag&0x00ff)|0x0100);
        word=rc_mac?(WORD)rc_mac_word(tag):tag;
        *(WORD *)rc_reserve_resource_bytes(2)=word;
        rc_append_resource_bytes(&shape,12);
        extra=12;
    }
    position=rc_seek(file,0,SEEK_CUR);
    return size-position+extra+2;
}
