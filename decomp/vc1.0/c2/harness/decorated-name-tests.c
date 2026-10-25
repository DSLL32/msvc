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

static NameSlice __fastcall ledger_token_entry(char **source)
{
    int valid = source && *source && !strcmp(*source,"ledger_sink@@3IC") &&
        decorated_last_token == -1 && !decorated_scope_mode &&
        !invalid_name_buffer.next && !invalid_name_buffer.remaining;
    recon_native_entry_finish(valid);
}
int c2_ledger_token_inputs(void)
{
    return recon_native_at_entry((const void *)scan_decorated_token,
        (const void *)ledger_token_entry,real_input,0)==1;
}
int c2_decorated_token_controls(void)
{
    char word[]="Alpha$9_@tail", reference[]="0rest", empty[]="@tail";
    char storage[3],*cursor,*start;NameSlice slice;int index,ch;
    invalid_name_buffer.next=0;invalid_name_buffer.remaining=0;
    decorated_last_token=-1;
    cursor=word;slice=scan_decorated_token(&cursor);
    if(slice.text!=word||slice.length!=8||cursor!=word+9||
        decorated_last_token!=0||decorated_tokens[0].text!=word||
        decorated_tokens[0].length!=8)return 0;
    cursor=reference;slice=scan_decorated_token(&cursor);
    if(slice.text!=word||slice.length!=8||cursor!=reference+1||
        decorated_last_token!=0)return 0;
    for(index=1;index<=11;index++){
        cursor=word;slice=scan_decorated_token(&cursor);
        if(slice.text!=word||slice.length!=8||decorated_last_token!=(index<9?index:9))return 0;
    }
    cursor=empty;slice=scan_decorated_token(&cursor);
    if(slice.text!=empty||slice.length||cursor!=empty+1||decorated_last_token!=9)return 0;
    invalid_name_buffer.next=word;invalid_name_buffer.remaining=123;
    decorated_last_token=-1;cursor=reference;slice=scan_decorated_token(&cursor);
    if(slice.text!=word||slice.length!=123||cursor!=reference+1||decorated_last_token!=-1)return 0;
    slice=scan_decorated_token(0);if(slice.text!=word||slice.length!=123)return 0;
    cursor=0;slice=scan_decorated_token(&cursor);if(slice.text!=word||slice.length!=123||cursor)return 0;
    /* Every initial byte: valid identifier/terminator, digit reference or
       invalid byte. Pointer advancement and source identity are observable. */
    for(ch=0;ch<256;ch++){
        storage[0]=(char)ch;storage[1]=0;storage[2]=0x5a;
        cursor=start=storage;decorated_last_token=-1;
        slice=scan_decorated_token(&cursor);
        if(ch=='@'||ch=='_'||ch=='$'||(ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')){
            if(slice.text!=start||slice.length!=(ch=='@'?0:1)||cursor!=start+1||decorated_last_token!=0)return 0;
        }else if(slice.text!=word||slice.length!=123||cursor!=start+(ch!=0)||decorated_last_token!=-1)return 0;
        if(storage[2]!=0x5a)return 0;
    }
    invalid_name_buffer.next=0;invalid_name_buffer.remaining=0;
    return 1;
}
int c2_ledger_qualified_controls(void)
{
    static const char *names[]={"?ledger_sink@@3IC","??0Cursor@Ledger@@QAE@PBULedgerEntry@@@Z",
        "??1Ledger@@QAE@XZ","??ACursor@Ledger@@QBEABULedgerEntry@@I@Z",
        "?add@Ledger@@QAE?AULedgerPair@@ABULedgerEntry@@I@Z"};
    static const char *expected[]={"ledger_sink","Ledger::Cursor::Cursor","Ledger::~Ledger",
        "Ledger::Cursor::operator[]","Ledger::add"};
    char output[264];NameBuffer result;unsigned index,i;
    invalid_name_buffer.next=0;invalid_name_buffer.remaining=0;decorated_scope_mode=0;
    for(index=0;index<RECON_COUNT(names);index++){
        memset(output,0x5a,sizeof output);
        result=decode_decorated_name(output,(char *)names[index],246);
        if(result.next!=output||result.remaining!=246||strcmp(output,expected[index])||decorated_scope_mode)return 0;
        for(i=246;i<sizeof output;i++)if((unsigned char)output[i]!=0x5a)return 0;
    }
    return 1;
}
