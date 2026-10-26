/* Borrow the real method-alias symbols at the ordinary OMF entry. Original
   defined fields were read at the same live marker; no private flags or
   identifiers are fabricated. The complete IO object is the actual-path oracle. */
#include <recon-tool.h>
#include <recon-native.h>
#include <recon-fixture.h>
#include "omf-output.h"
#include "machine.h"
#include "records.h"
#include <string.h>
#include <recon.h>
extern int main(int,char **,char **);
void __fastcall omf_emit_weak_external(FunctionSymbol *symbol);
/* omf_find_record renews COMENT records; observation must not call it. */
static OmfRecord *comment_record(void)
{unsigned i;for(i=0;i<4;++i)if(omf_records[i].type==0x88)return &omf_records[i];return 0;}
static void real_input(void *unused)
{char *argv[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,argv,0);}
static void fields(FunctionSymbol *symbol,FunctionSymbol *fallback,Segment *segment,unsigned *out)
{
 out[0]=symbol->symbol.identifier;out[1]=symbol->symbol.kind;
 out[2]=symbol->field24;out[3]=((BackendSymbol*)symbol)->output_flags;
 out[4]=symbol->symbol.field14;out[5]=symbol->field40;
 out[6]=fallback->symbol.identifier;out[7]=fallback->symbol.kind;
 out[8]=fallback->field24;out[9]=fallback->symbol.field0e;
 out[10]=((BackendSymbol*)fallback)->output_flags;out[11]=fallback->flags52;
 out[12]=fallback->symbol.field14;out[13]=segment->symbol.identifier;
 out[14]=segment->object_section;out[15]=segment->object_attributes.omf_comdat.flags;
}
static int original_input(unsigned *observed)
{
 unsigned expected[16];FILE *file;char path[1024];const char *assets=recon_case_assets();
 if(!assets||strlen(assets)+20>=sizeof(path))return 0;
 strcpy(path,assets);strcat(path,"/weak-fields.bin");file=fopen(path,"rb");if(!file)return 0;
 if(fread(expected,sizeof(expected),1,file)!=1||fgetc(file)!=EOF||ferror(file)){
  fclose(file);return 0;
 }
 fclose(file);return !memcmp(expected,observed,sizeof(expected));
}
static void __fastcall weak_entry(FunctionSymbol *symbol)
{
 FunctionSymbol *fallback=(FunctionSymbol*)lookup_global_symbol(symbol->field40);
 Segment *segment;unsigned before[16],after[16],i;
 if(!fallback||symbol==fallback)recon_native_entry_finish(0);
 segment=(Segment*)lookup_global_symbol(fallback->symbol.field18.identifier);
 if(!segment)recon_native_entry_finish(0);fields(symbol,fallback,segment,before);
 if(!original_input(before)||before[5]!=before[6]||before[10]||before[12]||before[14]||
  !(before[8]&0x4000)||!(before[9]&0x20)||(before[8]&0xf0)!=0x50)
  recon_native_entry_finish(0);
 omf_emit_weak_external(symbol);fields(symbol,fallback,segment,after);
 for(i=0;i<16;++i){if(i==10||i==11||i==12||i==14)continue;
  if(before[i]!=after[i])recon_native_entry_finish(0);
 }
 recon_native_entry_finish(after[10]==(before[10]|5)&&after[11]==(before[11]|0x20)&&
  after[12]==before[4]+1&&after[14]!=0&&!comment_record());
}
static int actual_weak(void)
{return recon_native_at_entry((const void*)omf_emit_weak_external,(const void*)weak_entry,real_input,0)==1;}
static void __fastcall comment_entry(FunctionSymbol *symbol)
{
 FunctionSymbol *fallback=(FunctionSymbol*)lookup_global_symbol(symbol->field40);
 OmfRecord *record;unsigned weak=symbol->symbol.field14,strong=fallback->symbol.field14;
 if(weak!=2||strong!=3||comment_record())recon_native_entry_finish(0);
 omf_write_weak_external_comment(symbol);record=comment_record();
 recon_native_entry_finish(record&&record->count==4&&record->cursor[-4]==0&&
  record->cursor[-3]==0xa8&&record->cursor[-2]==weak&&record->cursor[-1]==strong&&
  symbol->symbol.field14==weak&&fallback->symbol.field14==strong);
}
static int actual_comment(void)
{return recon_native_at_entry((const void*)omf_write_weak_external_comment,(const void*)comment_entry,real_input,0)==1;}
/* Adapter unit at a genuine initialized fixup entry. Reuse the real symbol,
   record, target threads and descriptor; no private OMF globals are forced.
   Stop before resuming compilation after the two deliberately added entries. */
void __fastcall omf_add_fixup(SymbolHeader *symbol,unsigned char type,unsigned displacement,int relative);
static void __fastcall segment_fixup_entry(SymbolHeader *symbol,unsigned char type,unsigned displacement,int relative)
{
 OmfRecord *record=omf_current_record;OmfFixup *wrapped,*direct;
 int before=omf_fixup_count;unsigned count,identifier,attributes;unsigned char *cursor;
 (void)type;(void)relative;
 if(!record||!symbol||before<0||before>247)recon_native_entry_finish(0);
 count=record->count;cursor=record->cursor;identifier=symbol->identifier;attributes=symbol->field0e;
 omf_output_value(displacement,symbol);wrapped=&omf_fixups[before];
 omf_add_fixup(symbol,2,displacement,0);direct=&omf_fixups[before+1];
 if(omf_fixup_count!=before+2||omf_current_record!=record||record->count!=count||record->cursor!=cursor||
  wrapped->type!=2||wrapped->location!=(unsigned short)(count+displacement)||
  direct->type!=wrapped->type||direct->location!=wrapped->location||
  (wrapped->flags&15)!=(direct->flags&15)||!(wrapped->flags&OMF_FIXUP_SEGMENT_RELATIVE)||
  wrapped->frame_code!=direct->frame_code||wrapped->frame_datum!=direct->frame_datum||
  wrapped->target_code!=direct->target_code||symbol->identifier!=identifier||symbol->field0e!=attributes)
  recon_native_entry_finish(0);
 if(!(wrapped->flags&OMF_FIXUP_TARGET_THREAD)&&wrapped->target_datum!=direct->target_datum)
  recon_native_entry_finish(0);
 if(!(wrapped->flags&OMF_FIXUP_NO_ADDEND)&&wrapped->addend!=direct->addend)
  recon_native_entry_finish(0);
 recon_native_entry_finish(1);
}
static int actual_segment_fixup_adapter(void)
{return recon_native_at_entry((const void*)omf_add_fixup,(const void*)segment_fixup_entry,real_input,0)==1;}
const ReconTest weak_tests[]={
 {"original-bound live method alias fallback publication",1,actual_weak},
 {"actual WKEXT attribute class and ordered external indices",1,actual_comment},
 {"unit-qualified segment fixup adapter borrows actual record and symbol",1,actual_segment_fixup_adapter}
};
