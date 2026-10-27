/* Typed valid owners grounded in the actual old App/cache/spill contracts. */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <recon-tool.h>
#include "../../src/cmdline.h"
#include "../../src/reloc.h"
#include <recon.h>
#define CHECK(x) do { if( !(x) ) { fprintf(stderr,"cache/spill assertion failed at line %d: %s\n",__LINE__,#x); return 0; } } while( 0 )
extern char *MakeTempName( char * );
int cache_spill_unit( void ) { return SwapOutVirt() == 0; }
static int arena_no_resize( void )
{
 unsigned char *owner;
 unsigned i;
 LinkState=1;
 CHECK(PermShrink()==0 && FreeUpMemory()==0);
 owner=PermAlloc(16384);
 memset(owner,0x69,16384);
 /* Full real arena: ShrinkBlock takes its no-resize branch. No old alias
  * is used after a possibly moving realloc, and none is requested here. */
 CHECK(PermShrink()==0 && FreeUpMemory()==0);
 for(i=0;i<16384;++i) CHECK(owner[i]==0x69);
 RelSymBlock();
 CHECK(PermShrink()==0);
 return 1;
}
static int cache_owner( void )
{
 file_list file;
 void **cache;
 unsigned char *data;
 memset(&file,0,sizeof(file));
 CurrMod=0;
 file.file=AllocFileEntry("cache.bin",0);
 CHECK(CacheOpen(&file)==1);
 data=CacheRead(&file,0,16);
 CHECK(data!=0 && memcmp(data,"cache-owner-data",16)==0);
 cache=file.file->cache;
 CHECK(cache!=0 && cache[0]!=0);
 CHECK(CacheRelease()==1 && cache[0]==0);
 CHECK(CacheRelease()==0);
 CacheClose(&file,2);
 CHECK(file.file->handle==-1 && file.file->cache==0);
 RelSymBlock();
 return 1;
}
static int dictionary_owner( void )
{
 file_list file;
 omf_dict_entry dict;
 memset(&file,0,sizeof(file)); memset(&dict,0,sizeof(dict));
 dict.cache=malloc(sizeof(void *)); CHECK(dict.cache!=0);
 dict.cache[0]=malloc(512);CHECK(dict.cache[0]!=0);
 memset(dict.cache[0],0x75,512);
 file.u.dict=&dict;ObjLibFiles=&file;
 CHECK(DiscardDicts()==1 && dict.cache==0);
 CHECK(DiscardDicts()==0);
 ObjLibFiles=0;
 return 1;
}
static int spill_roundtrip( void )
{
 struct { unsigned char before; char name[13]; unsigned char after; } path;
 struct { unsigned char before; unsigned char bytes[513]; unsigned char after; } out;
 unsigned char first[37],second[513];
 unsigned long a,b;unsigned i;
 memset(&path,0xa5,sizeof(path));
 CHECK(MakeTempName(path.name)==path.name+11);
 CHECK(memcmp(path.name,"WLK02112.--`",13)==0 && path.before==0xa5 && path.after==0xa5);
 TempFile=QOpenRW("spill.tmp");CHECK(TempFile>=0);TmpFSize=0;
 for(i=0;i<sizeof(first);++i)first[i]=i+9;
 for(i=0;i<sizeof(second);++i)second[i]=(unsigned char)(i*13);
 a=SpillAlloc(sizeof(first)); b=SpillAlloc(sizeof(second));
 CHECK(a==1 && b==513 && TmpFSize==1025);
 SpillWrite(a,0,first,sizeof(first));SpillWrite(b,0,second,sizeof(second));
 memset(&out,0xa5,sizeof(out));SpillRead(a,0,out.bytes,sizeof(first));
 CHECK(memcmp(out.bytes,first,sizeof(first))==0 && out.bytes[37]==0xa5);
 CHECK(out.before==0xa5 && out.after==0xa5);
 SpillRead(b,0,out.bytes,sizeof(second));
 CHECK(memcmp(out.bytes,second,sizeof(second))==0 && out.before==0xa5 && out.after==0xa5);
 QClose(TempFile,"spill.tmp");TempFile=-1;QDelete("spill.tmp");
 return 1;
}
static int relocation_spill( void )
{
 section root,leaf;
 ovl_area area;
 reloc_info item;
 unsigned char expected[12];
 struct { unsigned char before; unsigned char bytes[12]; unsigned char after; } readback;
 unsigned i;
 memset(&root,0,sizeof(root));memset(&leaf,0,sizeof(leaf));
 memset(&area,0,sizeof(area));memset(&item,0,sizeof(item));
 for(i=0;i<sizeof(expected);++i)expected[i]=i*17+3;
 item.loc.addr=malloc(512);CHECK(item.loc.addr!=0);
 memcpy(item.loc.addr,expected,sizeof(expected));item.sizeleft=512-sizeof(expected);
 leaf.reloclist=&item;area.sections=&leaf;root.areas=&area;
 Root=&root;FmtData.type=0x80;LinkState=0x40;
 TempFile=QOpenRW("reloc.tmp");CHECK(TempFile>=0);TmpFSize=0;
 CHECK(SwapOutRelocs()==1);
 CHECK(item.sizeleft==(0x8000|500) && item.loc.spill==1 && TmpFSize==512);
 CHECK(SwapOutRelocs()==0);
 memset(&readback,0xa5,sizeof(readback));
 SpillRead(item.loc.spill,0,readback.bytes,sizeof(expected));
 CHECK(memcmp(readback.bytes,expected,sizeof(expected))==0);
 CHECK(readback.before==0xa5 && readback.after==0xa5);
 QClose(TempFile,"reloc.tmp");TempFile=-1;QDelete("reloc.tmp");Root=0;
 return 1;
}
const ReconTest cache_spill_tests[] = {
 {"empty-and-full-arena-no-resize",1,arena_no_resize},
 {"real-input-cache-reclamation",1,cache_owner},
 {"owned-OMF-dictionary-reclamation",1,dictionary_owner},
 {"sector-tokens-real-file-roundtrip",1,spill_roundtrip},
 {"nested-overlay-relocation-spill",1,relocation_spill}
};
