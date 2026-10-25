/* Native-only typed observation glue. Compiled with the same register-POD
   return ABI as the App owner; case objects retain the common binary flags. */
#include <string.h>
#include <recon-tool.h>
#include <recon-native.h>
#include "decorated-names.h"
#include <recon.h>
extern int main(int,char **,char **);
static void real_input(void *unused)
{
    char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};
    (void)unused;main(1,argv,0);
}
static char *__fastcall wrapper_entry(char *name)
{
    recon_native_entry_finish(!strcmp(name,"??_C@_04ICHP@rec?3?$AA@"));
}
int c2_decorated_wrapper_inputs(void)
{
    return recon_native_at_entry((const void *)undecorate_function_name,
        (const void *)wrapper_entry,real_input,0)==1;
}
static NameBuffer __fastcall decoder_entry(char *destination,char *source,int capacity)
{
    unsigned i;int valid=destination&&source&&destination!=source&&capacity==246&&
        !strcmp(source,"??_C@_04ICHP@rec?3?$AA@")&&
        decorated_last_token==-1&&!decorated_scope_mode&&!invalid_name_buffer.next&&
        !invalid_name_buffer.remaining;
    for(i=0;i<246&&valid;i++)if(destination[i])valid=0;
    recon_native_entry_finish(valid);
}
int c2_decorated_decoder_inputs(void)
{
    return recon_native_at_entry((const void *)decode_decorated_name,
        (const void *)decoder_entry,real_input,0)==1;
}

/* These cold private-API controls avoid the still-masked token child. All
  41 operator rows have original table provenance; namespace/constructor
   completion is explicitly outside this case's execution coverage. */
int c2_decorated_operator_controls(void)
{
    static const int capacities[]={0,1,2,7,8,9,10,14,32,246};
    char name[8],output[264],expected[264],spelling[64];
    NameBuffer result;int row,index,capacity,length,written;
    for(row=0;row<41;row++){
        name[0]=name[1]='?';
        if(row<8){name[2]=(char)('2'+row);name[3]='@';name[4]=0;}
        else if(row<34){name[2]=(char)('A'+row-8);name[3]='@';name[4]=0;}
        else{name[2]='_';name[3]=(char)('0'+row-34);name[4]='@';name[5]=0;}
        strcpy(spelling,"operator");strcat(spelling,decorated_operator_names[row]);
        length=strlen(spelling);
        for(index=0;index<RECON_COUNT(capacities);index++){
            capacity=capacities[index];memset(output,0x5a,sizeof output);memset(expected,0x5a,sizeof expected);
            if(!capacity)expected[0]=0;
            else{
                /* strncpy zero-pads the complete requested logical extent. */
                memset(expected,0,capacity);written=length<capacity-1?length:capacity-1;
                memcpy(expected,spelling,written);
            }
            decorated_scope_mode=0;
            result=decode_decorated_name(output,name,capacity);
            if(result.next!=output||result.remaining!=capacity||decorated_last_token!=-1||
                decorated_scope_mode||memcmp(output,expected,sizeof output))return 0;
        }
    }
    return 1;
}
int c2_decorated_failure_controls(void)
{
    static const char *names[]={"plain","??","??_","??_7","??_C","??!","??[","??_/","??0@"};
    char output[264],*first,*second;NameBuffer result;unsigned i,unsigned_byte;
    for(i=0;i<RECON_COUNT(names);i++){
        memset(output,0x5a,sizeof output);decorated_scope_mode=0;
        result=decode_decorated_name(output,(char *)names[i],246);
        if(result.next||result.remaining||decorated_last_token!=-1||decorated_scope_mode)return 0;
        for(unsigned_byte=0;unsigned_byte<sizeof output;unsigned_byte++)
            if((unsigned char)output[unsigned_byte]!=0x5a)return 0;
    }
    decorated_scope_mode=1;memset(output,0x5a,sizeof output);
    result=decode_decorated_name(output,"?",246);
    if(result.next!=output||result.remaining!=246||output[0]!=0x5a)return 0;
    decorated_scope_mode=0;
    first=undecorate_function_name("??_C@");second=undecorate_function_name("plain");
    return first&&first==second&&!*first;
}
