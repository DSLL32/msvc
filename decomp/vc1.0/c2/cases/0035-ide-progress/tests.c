#include "case.h"
#include <recon-native.h>
#include "ide.h"
#include "coff-writer.h"
#include <stdio.h>
#include <string.h>
#include <recon.h>
extern int main(int,char**,char**);
static void real_input(void *unused)
{
 char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};
 (void)unused;main(1,argv,0);
}
/* Genuine first writer input: channel2, first17-byte progress record,
   context-owned64-byte buffer and all three actual callback identities.
   The writer must return the consumed byte count without altering its input. */
static int __fastcall write_entry(int channel,char *text,unsigned bytes)
{
 IdeContext *context=ide_context;unsigned char saved[64];int result;
 if(!context||channel!=2||ide_channel!=2||text!=context->buffer||bytes!=17||
  strcmp(text,"@I4rotate_profile")||context->capacity!=64||
  context->open_channel!=ide_open_channel||context->write_bytes!=ide_write_channel||
  context->report_error!=ide_error||fileno(stderr)!=2)recon_native_entry_finish(0);
 memcpy(saved,text,sizeof(saved));result=ide_write_channel(channel,text,bytes);
 recon_native_entry_finish(result==17&&ide_context==context&&
  ide_channel==2&&context->buffer==text&&context->capacity==64&&
  !memcmp(saved,text,sizeof(saved))&&fileno(stderr)==2);return 0;
}
static int actual_protocol_owner(void)
{return recon_native_at_entry((const void*)ide_write_channel,(const void*)write_entry,real_input,0)==1;}
/* The real profile's forward function identifier9c has no COFF handle yet.
   Check the new fixup owner/actual relocation identity and preserve every
   other bucket plus the relocation's defined fields. The complete object
   oracle separately checks eventual symbol resolution. */
static void __fastcall fixup_entry(unsigned identifier,CoffRelocation *relocation)
{
 CoffFunctionFixup *old[32],*added;unsigned i,address,symbol;unsigned short type;
 if(identifier!=0x9c||!relocation||relocation->symbol)recon_native_entry_finish(0);
 address=relocation->address;symbol=relocation->symbol;type=relocation->type;
 for(i=0;i<32;++i)old[i]=coff_function_fixups[i];
 coff_add_function_fixup(identifier,relocation);added=coff_function_fixups[identifier&31];
 if(!added||added==old[identifier&31]||added->identifier!=identifier||
  added->relocation!=relocation||added->next!=old[identifier&31]||
  relocation->address!=address||relocation->symbol!=symbol||relocation->type!=type)
  recon_native_entry_finish(0);
 for(i=0;i<32;++i)if(i!=(identifier&31)&&coff_function_fixups[i]!=old[i])
  recon_native_entry_finish(0);
 recon_native_entry_finish(1);
}
static int actual_forward_fixup(void)
{return recon_native_at_entry((const void*)coff_add_function_fixup,(const void*)fixup_entry,real_input,0)==1;}
const ReconTest ide_progress_tests[]={
 {"actual IDE channel/buffer/callback ownership and raw byte result",1,actual_protocol_owner},
 {"actual forward-function relocation owner and bucket identity",1,actual_forward_fixup}
};
