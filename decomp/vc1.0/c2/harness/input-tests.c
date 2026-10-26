/* Native-only typed POD access uses the same register-return ABI as the
   input owner and both App consumers. Case flags remain inherited unchanged. */
#include <recon-tool.h>
#include <recon-native.h>
#include "input.h"
#include <recon.h>
extern int main(int,char **,char **);
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);
}
static void double_entry(void)
{
 DoubleBits bits;FILE *file=input_file;char *before=file->_ptr;
 if(file!=initializer_file||ftell(file)!=7||file->_flag!=9||file->_cnt!=186||
  file->_bufsiz!=4096||file->_ptr-file->_base!=7)recon_native_entry_finish(0);
 bits=input_double_bits();
 recon_native_entry_finish(!bits.low&&bits.high==0x3fc00000U&&input_file==file&&
  ftell(file)==15&&file->_ptr==before+8&&file->_cnt==178&&file->_flag==9);
}
int c2_actual_double_input(void)
{
 return recon_native_at_entry((const void*)input_double_bits,(const void*)double_entry,real_input,0)==1;
}
int c2_double_input_bit_controls(void)
{
 static const DoubleBits values[]={{0,0},{0,0x80000000U},{0x12345678U,0x7ff81234U},{0,0x3ff00000U}};
 FILE *file=fopen("double-input-control.bin","w+b");DoubleBits result;unsigned i;
 if(!file)return 0;
 if(fwrite(values,sizeof(DoubleBits),RECON_COUNT(values),file)!=RECON_COUNT(values)){fclose(file);return 0;}
 rewind(file);input_file=file;
 for(i=0;i<RECON_COUNT(values);i++){
  result=input_double_bits();if(result.low!=values[i].low||result.high!=values[i].high){fclose(file);return 0;}
 }
 fclose(file);return 1;
}
