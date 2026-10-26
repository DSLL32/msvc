#include <recon-tool.h>
#include <recon-native.h>
#include "coff-writer.h"
#include "warnings.h"
#include "diagnostics.h"
#include "function-input.h"
#include "control-input.h"
#include <stdarg.h>
#include <string.h>
#include <recon.h>
extern void __fastcall change_warning_override(short number,signed char level);
extern int main(int,char **,char **);
extern void __cdecl format_diagnostic_arguments(char *,char *,va_list);
extern void __cdecl format_diagnostic_text(char *,char *,...);
extern void __cdecl emit_diagnostic(unsigned,unsigned,char *,va_list);
extern char *__fastcall unquote_diagnostic_message(char *);
extern void __fastcall remember_diagnostic_offset(long,unsigned);
extern char *__fastcall parse_diagnostic_number(char *,int *);
static void real_input(void *unused)
{
 char *arguments[]={"R:\\toolchains\\1.0-nt\\bin\\c23232.exe",0};(void)unused;main(1,arguments,0);
}
static void __fastcall comment_entry(char *text)
{
 CoffScope *scope;
 if(coff_trailer_scope||strcmp(text,"Calibration packet adapter: diagnostic-policy fixture"))recon_native_entry_finish(0);
 coff_write_comment(text);scope=coff_trailer_scope;
 if(!scope||coff_active_scope!=scope||scope->kind!=1||scope->value||scope->completed_bytes||scope->buffered_bytes!=54||
  scope->write_count!=1||scope->block->next!=scope->block||memcmp(scope->block->bytes,text,54))recon_native_entry_finish(0);
 /* Separate reuse control, after checking the genuine first call. */
 coff_write_comment("again");
 recon_native_entry_finish(coff_trailer_scope==scope&&coff_active_scope==scope&&scope->buffered_bytes==60&&scope->write_count==2&&
  !memcmp(scope->block->bytes,text,54)&&!memcmp(scope->block->bytes+54,"again",6));
}
static int actual_comment(void)
{
 return recon_native_at_entry((const void*)coff_write_comment,(const void*)comment_entry,real_input,0)==1;
}
static void __fastcall setter_entry(short number,signed char level)
{
 if(number!=5||level!=5||warning_override_count)recon_native_entry_finish(0);
 set_warning_override(number,level);
 recon_native_entry_finish(warning_override_count==1&&warning_override_level(705)==5&&!warning_override_level(5));
}
static int actual_setter(void)
{
 return recon_native_at_entry((const void*)set_warning_override,(const void*)setter_entry,real_input,0)==1;
}
static int setter_controls(void)
{
 unsigned i;warning_override_count=0;set_warning_override(5,5);set_warning_override(705,4);
 if(warning_override_count!=2||warning_override_level(705)!=5)return 0;
 warning_override_count=0;
 for(i=0;i<50;i++)set_warning_override((short)(700+i),(signed char)(i%5));
 return warning_override_count==50&&warning_override_level(749)==4&&!warning_override_level(750);
}
static void __fastcall change_entry(short number,signed char level)
{
 if(number!=705||level!=13||warning_override_count!=2||warning_override_level(705)!=5||warning_override_level(706)!=5)recon_native_entry_finish(0);
 change_warning_override(number,level);
 recon_native_entry_finish(warning_override_count==2&&warning_override_level(705)==13&&warning_override_level(706)==5);
}
static int actual_change(void)
{
 return recon_native_at_entry((const void*)change_warning_override,(const void*)change_entry,real_input,0)==1;
}
static int change_controls(void)
{
 warning_override_count=0;set_warning_override(5,5);set_warning_override(705,4);set_warning_override(706,7);
 change_warning_override(705,13);
 return warning_override_count==3&&warning_override_level(705)==13&&warning_override_level(706)==7;
}
static void __fastcall format_open_entry(unsigned number)
{
 recon_native_entry_finish(number==4705&&diagnostic_file&&diagnostic_file!=(FILE*)-1&&
  diagnostic_file->_flag==1&&!diagnostic_file->_cnt&&!diagnostic_file->_ptr&&!diagnostic_file->_base&&
  !diagnostic_file->_bufsiz&&ftell(diagnostic_file)==0);
}
static int actual_format_open(void)
{
 return recon_native_at_entry((const void*)diagnostic_cached_offset,(const void*)format_open_entry,real_input,0)==1;
}
static void __fastcall cache_entry(unsigned number)
{
 unsigned i;
 if(number!=4705)recon_native_entry_finish(0);
 for(i=0;i<20;i++)if(diagnostic_offsets[i].number||diagnostic_offsets[i].position)recon_native_entry_finish(0);
 recon_native_entry_finish(diagnostic_cached_offset(number)==0);
}
static int actual_empty_cache(void)
{
 return recon_native_at_entry((const void*)diagnostic_cached_offset,(const void*)cache_entry,real_input,0)==1;
}
static int cache_controls(void)
{
 memset(diagnostic_offsets,0,sizeof diagnostic_offsets);
 diagnostic_offsets[0].number=4705;diagnostic_offsets[0].position=3541;
 diagnostic_offsets[1].number=4700;diagnostic_offsets[1].position=3000;
 if(diagnostic_cached_offset(4705)!=3541||diagnostic_cached_offset(4706)!=3541||
  diagnostic_cached_offset(4704)!=3000||diagnostic_cached_offset(4699)!=0)return 0;
 diagnostic_offsets[0].number=100;diagnostic_offsets[0].position=1;
 diagnostic_offsets[1].number=0;
 return diagnostic_cached_offset(32866)==1&&diagnostic_cached_offset(32867)==0;
}
static void __fastcall reader_entry(unsigned number,char *buffer,unsigned capacity)
{
 unsigned i;
 if(number!=4705||buffer!=diagnostic_text||capacity!=384||ftell(diagnostic_file)!=0)recon_native_entry_finish(0);
 for(i=0;i<384;i++)if(buffer[i])recon_native_entry_finish(0);
 recon_native_entry_finish(1);
}
static int actual_reader(void)
{
 return recon_native_at_entry((const void*)read_diagnostic_message,(const void*)reader_entry,real_input,0)==1;
}
static void __fastcall parser_entry(char *text,int *number)
{
 char *result;
 if(text!=diagnostic_text||!number||strcmp(text,"\t\t\t\t\t\t/* fatals */\r\n"))recon_native_entry_finish(0);
 result=parse_diagnostic_number(text,number);
 recon_native_entry_finish(result==text&&*number==0);
}
static int actual_number_parser(void)
{
 return recon_native_at_entry((const void*)parse_diagnostic_number,(const void*)parser_entry,real_input,0)==1;
}
static int parser_controls(void)
{
 char digits[]="4705 \"statement\"",empty[]="",space[]=" 12",overflow[]="42949672960x";int number;
 if(parse_diagnostic_number(digits,&number)!=digits+4||number!=4705)return 0;
 if(parse_diagnostic_number(empty,&number)!=empty||number)return 0;
 if(parse_diagnostic_number(space,&number)!=space||number)return 0;
 return parse_diagnostic_number(overflow,&number)==overflow+11&&number==0;
}
static void __fastcall remember_entry(long position,unsigned number)
{
 unsigned i;
 if(position!=3541||number!=4705)recon_native_entry_finish(0);
 for(i=0;i<20;i++)if(diagnostic_offsets[i].number||diagnostic_offsets[i].position)recon_native_entry_finish(0);
 remember_diagnostic_offset(position,number);
 if(diagnostic_offsets[0].number!=4705||diagnostic_offsets[0].position!=3541)recon_native_entry_finish(0);
 for(i=1;i<20;i++)if(diagnostic_offsets[i].number||diagnostic_offsets[i].position)recon_native_entry_finish(0);
 recon_native_entry_finish(1);
}
static int actual_remember(void)
{
 return recon_native_at_entry((const void*)remember_diagnostic_offset,(const void*)remember_entry,real_input,0)==1;
}
static int remember_controls(void)
{
 unsigned i;
 memset(diagnostic_offsets,0,sizeof diagnostic_offsets);
 remember_diagnostic_offset(3541,4705);remember_diagnostic_offset(9999,4705);
 if(diagnostic_offsets[0].position!=3541)return 0;
 for(i=1;i<20;i++)remember_diagnostic_offset(4000+i,4705+i);
 remember_diagnostic_offset(9000,6000);
 for(i=0;i<20;i++)if(diagnostic_offsets[i].number!=4705+i||diagnostic_offsets[i].position!=(i?4000+i:3541))return 0;
 return 1;
}
static void __fastcall unquote_entry(char *text)
{
 char *result;
 if(text!=diagnostic_text+4||strcmp(text,"\t\"statement has no effect\"\r\n"))recon_native_entry_finish(0);
 result=unquote_diagnostic_message(text);
 recon_native_entry_finish(result==diagnostic_text+6&&!strcmp(result,"statement has no effect")&&diagnostic_offsets[0].number==4705&&diagnostic_offsets[0].position==3541);
}
static int actual_unquote(void)
{
 return recon_native_at_entry((const void*)unquote_diagnostic_message,(const void*)unquote_entry,real_input,0)==1;
}
static int unquote_controls(void)
{
 char escaped[]="prefix\"a\\nb\\tc\\qd\"tail",empty[]="\"\"",unterminated[]="\"last";char *result;
 result=unquote_diagnostic_message(escaped);
 if(result!=escaped+7||strcmp(result,"a\nb\tc\\qd")||strcmp(escaped+18,"tail"))return 0;
 if(unquote_diagnostic_message(empty)!=empty+1||empty[1])return 0;
 return unquote_diagnostic_message(unterminated)==unterminated+1&&!strcmp(unterminated+1,"last");
}
static void __cdecl emitter_entry(unsigned category,unsigned number,char *format,va_list arguments)
{
 /* This actual message has no conversions, so no variadic payload is read. */
 recon_native_entry_finish(category==4000&&number==705&&format==diagnostic_text+6&&
  !strcmp(format,"statement has no effect")&&arguments&&current_source_location==20&&
  function_source_location==16&&pending_source_location==30&&current_source_filename&&
  !strcmp(current_source_filename,"calibr.c"));
}
static int actual_emitter(void)
{
 return recon_native_at_entry((const void*)emit_diagnostic,(const void*)emitter_entry,real_input,0)==1;
}
static void __cdecl prefix_entry(char *buffer,char *format,...)
{
 va_list arguments;char *source;int line;
 va_start(arguments,format);source=va_arg(arguments,char *);line=va_arg(arguments,int);va_end(arguments);
 recon_native_entry_finish(buffer&&format&&!strcmp(format,"%s(%ld) : ")&&
  source==current_source_filename&&!strcmp(source,"calibr.c")&&line==20);
}
static int actual_prefix(void)
{
 return recon_native_at_entry((const void*)format_diagnostic_text,(const void*)prefix_entry,real_input,0)==1;
}
static void __cdecl arguments_entry(char *buffer,char *format,va_list arguments)
{
 va_list copy=arguments;char *source=va_arg(copy,char *);int line=va_arg(copy,int);
 if(!buffer||strcmp(format,"%s(%ld) : ")||source!=current_source_filename||strcmp(source,"calibr.c")||line!=20)recon_native_entry_finish(0);
 format_diagnostic_arguments(buffer,format,arguments);
 recon_native_entry_finish(!strcmp(buffer,"calibr.c(20) : "));
}
static int actual_arguments(void)
{
 return recon_native_at_entry((const void*)format_diagnostic_arguments,(const void*)arguments_entry,real_input,0)==1;
}
static int format_controls(void)
{
 char buffer[256];unsigned i,n;
 memset(buffer,0x5a,sizeof buffer);
 format_diagnostic_text(buffer,"%ld/%x/%c/%s/%s/%q/%%",(long)0x80000000U,0xabcdefU,'Z',"ok",(char*)0);
 if(strcmp(buffer,"-2147483648/abcdef/Z/ok//%q/%%"))return 0;
 n=strlen(buffer)+1;for(i=n;i<sizeof buffer;i++)if(buffer[i]!=0x5a)return 0;
 format_diagnostic_text(buffer,"%Fs/%ld/%02d/%lq", "far",(long)-17);
 if(strcmp(buffer,"far/-17/%02d/lq"))return 0;
 format_diagnostic_text(buffer,"");return buffer[0]==0;
}
const ReconTest calibration_tests[]={
 {"actual-comment-text-trailer-identity-and-separate-reuse-control",1,actual_comment},
 {"actual-warning-setter-short-byte-input-and-700-normalization",1,actual_setter},
 {"warning-append-first-match-and-fiftieth-entry-controls",1,setter_controls},
 {"actual-once-warning-first-entry-change-and-untouched-neighbor",1,actual_change},
 {"warning-change-first-match-retains-count-and-other-number",1,change_controls},
 {"actual-diagnostic-open-before-cache-query-number-file-and-position",1,actual_format_open},
 {"actual-empty-diagnostic-cache-query-number-and-zero-offset",1,actual_empty_cache},
 {"diagnostic-cache-exact-nearest-earlier-and-strict-distance-controls",1,cache_controls},
 {"actual-diagnostic-reader-number-buffer-capacity-and-initial-position",1,actual_reader},
 {"actual-diagnostic-number-parser-comment-line-output-and-pointer",1,actual_number_parser},
 {"diagnostic-number-parser-digits-empty-no-whitespace-skip-and-wrap",1,parser_controls},
 {"actual-diagnostic-offset-insertion-full-defined-cache-state",1,actual_remember},
 {"diagnostic-offset-retains-first-duplicate-and-ignores-full-cache",1,remember_controls},
 {"actual-diagnostic-unquote-buffer-alias-message-and-cache",1,actual_unquote},
 {"diagnostic-unquote-newline-tab-unknown-escape-empty-and-nul-controls",1,unquote_controls},
 {"actual-warning-emitter-category-number-format-and-source-owner-state",1,actual_emitter},
 {"actual-diagnostic-prefix-wrapper-format-and-two-vararg-slots",1,actual_prefix},
 {"actual-private-formatter-va-list-source-alias-line-and-defined-output",1,actual_arguments},
 {"private-formatter-intmin-hex-char-null-modifiers-literals-and-owned-tail",1,format_controls}
};
