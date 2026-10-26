#include <recon-tool.h>
#include <recon-native.h>
#include "input.h"
#include "intrinsics.h"
#include "instruction-rewrite.h"
#include <string.h>
#include <recon.h>
extern int main(int,char **,char **);
extern Node *__fastcall lower_intrinsic(Node *,Node *);
static void real_input(void *unused)
{
 char *arguments[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,arguments,0);
}
static void input_entry(void)
{
 FILE *file=input_file;char *before;
 if(file!=initializer_file||ftell(file)!=7||file->_flag!=9||file->_cnt!=84||file->_bufsiz!=4096||file->_ptr-file->_base!=7)recon_native_entry_finish(0);
 before=file->_ptr;
 if(input_float_bits()!=0x3e000000U)recon_native_entry_finish(0);
 recon_native_entry_finish(input_file==file&&ftell(file)==11&&file->_ptr==before+4&&file->_cnt==80);
}
static int actual_float_input(void)
{
 return recon_native_at_entry((const void*)input_float_bits,(const void*)input_entry,real_input,0)==1;
}
static int bit_transport_controls(void)
{
 unsigned data[]={0x80000000U,0x7fc12345U,0x3f800000U};unsigned i;FILE *file=fopen("float-input-control.bin","w+b");
 if(!file)return 0;
 if(fwrite(data,sizeof(unsigned),3,file)!=3){fclose(file);return 0;}
 rewind(file);input_file=file;
 for(i=0;i<3;i++)if(input_float_bits()!=data[i]){fclose(file);return 0;}
 fclose(file);return 1;
}
static void __fastcall absolute_intrinsic_entry(Node *node,Node *parent)
{
 Node *argument,*result;
 if(node->opcode!=0x7a||node->attributes.words.type!=0x1008||node->left.node->left.value!=17||
  !node->second.node||node->second.node->second.node->opcode!=0x4c)recon_native_entry_finish(0);
 argument=node->second.node->left.node;
 result=lower_intrinsic(node,parent);
 recon_native_entry_finish(result==node&&node->opcode==0x84&&node->left.node==argument&&node->attributes.words.type==0x1008);
}
static int actual_absolute_intrinsic(void)
{
 return recon_native_at_entry((const void*)lower_intrinsic,(const void*)absolute_intrinsic_entry,real_input,0)==1;
}
static void __cdecl multiply_entry(const Extended *left,const Extended *right,Extended *result)
{
 static const Extended a={0,0xa0000000U,0x3fff,8},b={0,0xc0000000U,0x3ffd,8},expected={0,0xf0000000U,0x3ffd,8};
 if(!left||!right||!result||left==right||result==left||result==right||memcmp(left,&a,12)||memcmp(right,&b,12))recon_native_entry_finish(0);
 recon_native_entry_finish(!multiply_extended(left,right,result)&&!memcmp(result,&expected,12)&&!memcmp(left,&a,12)&&!memcmp(right,&b,12));
}
static int actual_multiply(void)
{
 return recon_native_at_entry((const void*)multiply_extended,(const void*)multiply_entry,real_input,0)==1;
}
static int multiply_controls(void)
{
 Extended a={0,0xc0000000U,0x3fff,4},b={0,0xa0000000U,0x3fff,4},result;
 if(multiply_extended(&a,&b,&result)||result.format!=4||result.low||result.high!=0xf0000000U||result.exponent!=0x3fff)return 0;
 a.high=0x80000000U;a.exponent=0x4000;a.format=10;b.high=0xc0000000U;b.exponent=0xc000;b.format=8;
 return !multiply_extended(&a,&b,&result)&&result.format==10&&!result.low&&result.high==0xc0000000U&&result.exponent==0xc001;
}
static void __cdecl divide_entry(const Extended *left,const Extended *right,Extended *result)
{
 static const Extended a={0,0xe0000000U,0x4000,8},b={0,0xa0000000U,0x3fff,8},expected={0x33333000U,0xb3333333U,0x4000,8};
 if(!left||!right||!result||left==right||result==left||result==right||memcmp(left,&a,12)||memcmp(right,&b,12))recon_native_entry_finish(0);
 recon_native_entry_finish(!divide_extended(left,right,result)&&!memcmp(result,&expected,12)&&!memcmp(left,&a,12)&&!memcmp(right,&b,12));
}
static int actual_divide(void)
{
 return recon_native_at_entry((const void*)divide_extended,(const void*)divide_entry,real_input,0)==1;
}
static int divide_controls(void)
{
 Extended a={0,0xc0000000U,0x4001,4},b={0,0x80000000U,0x4000,4},result;
 if(divide_extended(&a,&b,&result)||result.format!=4||result.low||result.high!=0xc0000000U||result.exponent!=0x4000)return 0;
 a.high=0xa0000000U;a.exponent=0xbfff;a.format=10;b.exponent=0x3ffe;b.format=8;
 return !divide_extended(&a,&b,&result)&&result.format==10&&!result.low&&result.high==0xa0000000U&&result.exponent==0xc000;
}
static void __cdecl absolute_entry(const Extended *number,Extended *result)
{
 static const Extended source={0,0xb0000000U,0xbffe,8},expected={0,0xb0000000U,0x3ffe,8};
 if(!number||!result||number==result||memcmp(number,&source,12))recon_native_entry_finish(0);
 recon_native_entry_finish(!absolute_extended(number,result)&&!memcmp(result,&expected,12)&&!memcmp(number,&source,12));
}
static int actual_absolute(void)
{
 return recon_native_at_entry((const void*)absolute_extended,(const void*)absolute_entry,real_input,0)==1;
}
static int absolute_controls(void)
{
 Extended number={0,0,0x8000,4},result;
 if(absolute_extended(&number,&result)||result.low||result.high||result.exponent||result.format!=4)return 0;
 number.high=0xa0000000U;number.exponent=0xbfff;number.format=10;
 if(absolute_extended(&number,&result)||result.low||result.high!=number.high||result.exponent!=0x3fff||result.format!=10)return 0;
 number.exponent=0x3fff;return !absolute_extended(&number,&result)&&result.exponent==0x3fff;
}
static void unsigned_entry(const unsigned *integer,Extended *number)
{
 static const Extended expected={0,0x80000013U,0x401e,4};
 if(!integer||!number||*integer!=0x80000013U)recon_native_entry_finish(0);
 unsigned_integer_to_extended(integer,number);
 recon_native_entry_finish(*integer==0x80000013U&&!memcmp(number,&expected,12));
}
static int actual_unsigned(void)
{
 return recon_native_at_entry((const void*)unsigned_integer_to_extended,(const void*)unsigned_entry,real_input,0)==1;
}
static int unsigned_controls(void)
{
 static const unsigned values[]={0U,0x7fffffffU,0x80000000U,0xffffffffU};
 static const unsigned high[]={0U,0xfffffffeU,0x80000000U,0xffffffffU};
 static const unsigned short exponent[]={0,0x401d,0x401e,0x401e};
 unsigned i;Extended result;
 for(i=0;i<4;i++){
  unsigned_integer_to_extended(&values[i],&result);
  if(result.low||result.high!=high[i]||result.exponent!=exponent[i]||result.format!=4)return 0;
 }
 return 1;
}
static void __cdecl minimum_entry(const Extended *left,const Extended *right,Extended *result)
{
 static const Extended a={0,0xa0000000U,0x3ffd,8},b={0,0xe0000000U,0x3ffd,8};
 if(!left||!right||!result||result==left||result==right||memcmp(left,&a,12)||memcmp(right,&b,12))recon_native_entry_finish(0);
 recon_native_entry_finish(!minimum_extended(left,right,result)&&!memcmp(result,&a,12)&&!memcmp(left,&a,12)&&!memcmp(right,&b,12));
}
static int actual_minimum(void)
{
 return recon_native_at_entry((const void*)minimum_extended,(const void*)minimum_entry,real_input,0)==1;
}
static int minimum_controls(void)
{
 Extended a={0,0xa0000000U,0x3fff,4},b={0,0xc0000000U,0x3ffe,8},result;
 if(minimum_extended(&a,&b,&result)||result.low||result.high!=b.high||result.exponent!=b.exponent||result.format!=8)return 0;
 if(minimum_extended(&b,&a,&result)||result.high!=b.high||result.exponent!=b.exponent||result.format!=8)return 0;
 a.high=0;a.exponent=0x8000;a.format=10;b.high=0;b.exponent=0;
 return !minimum_extended(&a,&b,&result)&&!result.low&&!result.high&&result.exponent==0x8000&&result.format==10;
}
static void to_float_entry(const Extended *number,float *result)
{
 static const Extended source={0xaaaaa800U,0xaaaaaaaaU,0x3ffd,8};unsigned bits;
 if(!number||!result||(const void*)number==(void*)result||memcmp(number,&source,12))recon_native_entry_finish(0);
 if(extended_to_float(number,result))recon_native_entry_finish(0);
 memcpy(&bits,result,4);recon_native_entry_finish(bits==0x3eaaaaabU&&!memcmp(number,&source,12));
}
static int actual_to_float(void)
{
 return recon_native_at_entry((const void*)extended_to_float,(const void*)to_float_entry,real_input,0)==1;
}
static int to_float_controls(void)
{
 Extended number={0,0,0x8000,10};float result;unsigned bits;int error;
 if(extended_to_float(&number,&result))return 0;memcpy(&bits,&result,4);if(bits!=0x80000000U)return 0;
 number.high=0x80000000U;number.exponent=0x3fff;
 if(extended_to_float(&number,&result))return 0;memcpy(&bits,&result,4);if(bits!=0x3f800000U)return 0;
 number.exponent=0x407f;error=extended_to_float(&number,&result);memcpy(&bits,&result,4);
 return error==1&&bits==0x7f800000U;
}
static void from_float_entry(const float *number,Extended *result)
{
 static const Extended expected={0,0xaaaaab00U,0x3ffd,4};unsigned bits;
 if(!number||!result||(const void*)number==(void*)result)recon_native_entry_finish(0);
 memcpy(&bits,number,4);if(bits!=0x3eaaaaabU)recon_native_entry_finish(0);
 float_to_extended(number,result);memcpy(&bits,number,4);
 recon_native_entry_finish(bits==0x3eaaaaabU&&!memcmp(result,&expected,12));
}
static int actual_from_float(void)
{
 return recon_native_at_entry((const void*)float_to_extended,(const void*)from_float_entry,real_input,0)==1;
}
static int from_float_controls(void)
{
 static const unsigned values[]={0x80000000U,0x3f800000U,0x7f7fffffU,1};
 static const Extended expected[]={ {0,0,0x8000,4},{0,0x80000000U,0x3fff,4},
  {0,0xffffff00U,0x407e,4},{0,0x80000000U,0x3f6a,4} };
 unsigned i;float source;Extended result;
 for(i=0;i<4;i++){
  memcpy(&source,&values[i],4);float_to_extended(&source,&result);
  if(memcmp(&result,&expected[i],12))return 0;
 }
 return 1;
}
static void __fastcall narrow_entry(MachineInstruction *instruction)
{
 unsigned char before[32];
 if(instruction->opcode!=0x32d20||instruction->operands[0].flags!=1||
  instruction->operands[0].value.reg.words.number!=0x402c||instruction->record.next_use!=instruction->next)recon_native_entry_finish(0);
 memcpy(before,instruction,32);
 recon_native_entry_finish(narrow_result_safe(instruction)==1&&!memcmp(before,instruction,32));
}
static int actual_narrow(void)
{
 return recon_native_at_entry((const void*)narrow_result_safe,(const void*)narrow_entry,real_input,0)==1;
}
static int narrow_controls(void)
{
 MachineInstruction value,first,last;
 memset(&value,0,sizeof value);memset(&first,0,sizeof first);memset(&last,0,sizeof last);
 value.operands[0].value.reg.words.number=0x4001;if(!narrow_result_safe(&value))return 0;
 value.operands[0].value.reg.words.number=1;if(narrow_result_safe(&value))return 0;
 value.operands[0].value.reg.words.number=0x8001;if(narrow_result_safe(&value))return 0;
 value.opcode=0x20020;value.record.next_use=&first;first.opcode=0x20;first.operands[0].flags=1;
 if(!narrow_result_safe(&value))return 0;
 first.operands[0].flags=2;if(narrow_result_safe(&value))return 0;first.operands[0].flags=1;
 first.opcode=0x40;if(narrow_result_safe(&value))return 0;first.opcode=0x20;
 value.opcode=0x10020;first.operands[1].flags=1;if(narrow_result_safe(&value))return 0;
 value.opcode=0x20;first.opcode=0x20020;first.record.next_use=&last;last.opcode=0x10;last.operands[0].flags=1;
 if(!narrow_result_safe(&value))return 0;
 first.opcode=0x20;return narrow_result_safe(&value)==0;
}
const ReconTest mixed_wave_tests[]={
 {"actual-float-initializer-owner-file-position-buffer-and-raw-result",1,actual_float_input},
 {"float-input-preserves-negative-zero-nan-payload-and-one-bits",1,bit_transport_controls},
 {"actual-fabs-intrinsic17-unary-lowering-preserves-value-owner",1,actual_absolute_intrinsic},
 {"actual-extended-multiply-all-input-result-bytes-aliases-and-return",1,actual_multiply},
 {"extended-multiply-single-and-extended-tag-signed-controls",1,multiply_controls},
 {"actual-extended-divide-defined-input-result-bytes-and-normalized-return",1,actual_divide},
 {"extended-divide-left-right-order-sign-and-tag-controls",1,divide_controls},
 {"actual-extended-absolute-source-result-bytes-and-owner",1,actual_absolute},
 {"extended-absolute-negative-zero-finite-sign-and-tag-controls",1,absolute_controls},
 {"actual-unsigned-extended-conversion-input-and-all-result-bytes",1,actual_unsigned},
 {"unsigned-extended-zero-signed-boundary-and-maximum-controls",1,unsigned_controls},
 {"actual-extended-minimum-input-result-bytes-and-left-owner",1,actual_minimum},
 {"extended-minimum-both-directions-equal-signed-zero-and-mixed-tags",1,minimum_controls},
 {"actual-extended-to-float-input-all-bytes-destination-and-rounding",1,actual_to_float},
 {"extended-to-float-negative-zero-one-and-bounded-overflow-controls",1,to_float_controls},
 {"actual-float-to-extended-source-bits-and-complete-result",1,actual_from_float},
 {"float-to-extended-negative-zero-one-largest-and-smallest-controls",1,from_float_controls},
 {"actual-narrow-result-register-class-shortcut-and-readonly-owner",1,actual_narrow},
 {"narrow-result-class-width-selected-operand-and-use-chain-controls",1,narrow_controls}
};
