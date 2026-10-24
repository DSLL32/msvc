#!/usr/bin/env python3
"""Own original-only setlocale/_setmbcp witness; no native/API acceptance."""
import argparse,hashlib,json,re,struct,time
from pathlib import Path
from allocation_imports import export_rva,pe_read
from multibyte_fields import defaults,read,seed
from provenance import sha

PHASES=('before','after','mb-before','mb-after','next')

def require(ok,message):
 if not ok:raise ValueError('C1 isolated locale: '+message)

def vector(raw):
 require(len(raw)>=8,'vector header');root,count=struct.unpack_from('<2I',raw);require(root and count<4096,'vector root/count');at=8;spans=[]
 for i in range(count):
  require(at+8<=len(raw),'vector entry');pointer,length=struct.unpack_from('<2I',raw,at);at+=8
  require(pointer and 0<length<=65536 and at+length<=len(raw),'vector bytes');text=raw[at:at+length];at+=length
  require(text[-1:]==b'\0'and b'\0'not in text[:-1],'vector terminator');spans.append((pointer,pointer+length))
 require(at==len(raw),'vector trailing');require(all(b[0]>=a[1]or b[1]<=a[0]for i,a in enumerate(spans)for b in spans[i+1:]),'vector string alias')
 return root,count

def fp_semantic(raw):
 require(len(raw)==112,'full112 FloatSave');cw,sw,tag=(n&65535 for n in struct.unpack_from('<3I',raw));top=(sw>>11)&7
 return dict(cw=cw,sw=sw,tag=tag,top=top,live=[dict(logical=i,tag=(tag>>(2*((top+i)&7)))&3,raw80=raw[28+10*i:38+10*i].hex())for i in range(8)if((tag>>(2*((top+i)&7)))&3)!=3])

def check(directory):
 started=time.perf_counter();directory=directory.resolve();receipt=json.loads((directory/'receipt.json').read_text())
 require((receipt['kind'],receipt['mode'],receipt['exit'])==('explicit-original-capture','masked',125),'capture role')
 for name,row in receipt['artifacts'].items():
  path=directory/name;require(Path(name).name==name and path.is_file()and path.stat().st_size==row['size']and sha(path)==row['sha256'],'artifact drift '+name)
 schema=json.loads((directory/'locale-schema.json').read_text());candidate=json.loads((directory/'candidate.json').read_text());target=directory/'loaded-target.bin';crt=directory/'loaded-msvcrt.bin'
 require(sha(target)==receipt['target_sha256']==schema['target_sha256'],'actual target hFile');require(sha(crt)==receipt['canonical_crt_sha256']==schema['provider_sha256']and crt.stat().st_size==schema['provider_bytes'],'actual canonical CRT hFile')
 require(sha(directory/'locale-observer.inc')==schema['transport_source']['sha256'],'compiled locale observer')
 require(candidate['functions'][-1]['name']=='_initialize_multibyte_scanner'and candidate['functions'][-1]['chunks']==[[0x12f08,0x12f3e],[0x307c0,0x307d2]],'exact actual scanner trial')
 require(candidate['expected_frontier']['rva']==0x12f3e and candidate['isolated_locale_observers']['analysis_only_until_native_success'],'honest current role')
 for name,rva in schema['exports'].items():require(export_rva(crt,name)==rva,'canonical export '+name)
 for row in schema['canonical_instructions']:
  raw=bytes.fromhex(row['bytes']);require(pe_read(crt,row['rva'],len(raw))==raw,'canonical CRT instructions')
 for key in ('call','multibyte_call'):
  call=schema[key];require(pe_read(target,call['call_instruction_rva'],6)==bytes.fromhex(call['call_bytes']),'actual caller '+key)
 require(pe_read(target,schema['call']['arguments'][1]-0x10600000,1)==b'\0','own empty string identity')
 events=(directory/'events.txt').read_text();require('failure 'not in events,'controller failure')
 require(re.findall(r'^frontier rva=(\w+)$',events,re.M)==['00012f3e'],'genuine next target frontier')
 require(events.count('isolated-locale-observer setlocale-iat=0005d120 setmbcp-iat=0005d148 exports=14 no-query-calls=1 no-seeds=1\n')==1,'observer contract')
 module=int(re.search(r'^module name=c1n32_l.dll actual=(\w+) ',events,re.M)[1],16);base=int(re.search(r'^module name=MSVCRT.dll actual=(\w+) ',events,re.M|re.I)[1],16);require(module==0x10600000,'preferred target base only')
 entry=re.findall(r'^public-entry seq=(\d+) module=MSVCRT.dll name=setlocale provider=(\w+) rva=00005729 iat=0005d120 esp=(\w+) return=(\w+) category=(\d+) locale=(\w+) error=(\d+)$',events,re.M)
 returned=re.findall(r'^public-return seq=(\d+) module=MSVCRT.dll name=setlocale pc=(\w+) esp=(\w+) entry-esp=(\w+) result=(\w+) error=(\d+)$',events,re.M)
 mbentry=re.findall(r'^mb-entry seq=(\d+) pc=(\w+) esp=(\w+) return=(\w+) codepage=(-?\d+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+)$',events,re.M)
 mbreturned=re.findall(r'^mb-return seq=(\d+) pc=(\w+) esp=(\w+) entry-esp=(\w+) result=(\w+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+)$',events,re.M)
 require(len(entry)==len(returned)==len(mbentry)==len(mbreturned)==1,'exact API call inventory');entry,returned,mbentry,mbreturned=entry[0],returned[0],mbentry[0],mbreturned[0]
 require(int(entry[1],16)==base+0x5729 and int(entry[3],16)==int(returned[1],16)==module+0x12b73 and int(entry[4])==2 and int(entry[5],16)==module+0x601c1,'locale actual entry/caller/arguments')
 require(int(returned[2],16)==int(entry[2],16)+4 and returned[3]==entry[2]and entry[6]==returned[5]=='0','locale cdecl RET0/error')
 require(int(mbentry[1],16)==base+0x2257 and int(mbentry[3],16)==int(mbreturned[1],16)==module+0x12f12 and int(mbentry[4])==-4,'mb actual entry/caller/argument')
 require(int(mbreturned[2],16)==int(mbentry[2],16)+4 and mbreturned[3]==mbentry[2]and int(mbreturned[4],16)==0 and mbentry[5:]==mbreturned[5:],'mb RET0/result/nonvolatiles')
 observed={};private={}
 for phase in PHASES:
  def blob(suffix):return(directory/('locale-'+phase+'-'+suffix+'.bin')).read_bytes()
  w=list(struct.unpack('<18I',blob('words')));errors=list(struct.unpack('<3I',blob('errors')))
  require(w[0]==base and w[12]==base+0x3fba8 and w[16]==base+0x3fbaa,'canonical provider/static/retained owner')
  require(errors==[0,0,0],'own actual three errors')
  require(len(blob('mbctype'))==257 and len(blob('ctype'))==len(blob('old-ctype'))==514,'full table widths including EOF')
  require(len(blob('teb'))==4096 and len(blob('ptd'))==16 and list(struct.unpack_from('<2I',blob('ptd'),8))==errors[:2]and struct.unpack_from('<I',blob('teb'),0x34)[0]==errors[2],'raw TEB/PTD errors')
  state=re.findall(r'^locale-state phase='+phase+r' teb=(\w+) tid=(\d+) tls-index=(\d+) ptd=(\w+) errno=(\d+) doserrno=(\d+) LastError=(\d+)$',events,re.M);require(len(state)==1,'actual per-phase TLS inventory')
  state=state[0];teb,tid,tls,ptd=int(state[0],16),int(state[1]),int(state[2]),int(state[3],16)
  require(tls<64 and struct.unpack_from('<I',blob('teb'),0x18)[0]==teb and struct.unpack_from('<I',blob('teb'),0x24)[0]==tid and struct.unpack_from('<I',blob('teb'),0xe10+tls*4)[0]==ptd and list(map(int,state[4:]))==errors,'actual TEB/TID/TLS/PTD identity')
  for name,pointer,length in [('ctype',w[11]-2,514),('old-ctype',w[16]-2,514),('result',w[17],len(blob('result')))]:
   region=struct.unpack('<7I',blob(name+'-region'))
   if pointer:require(region[4]==0x1000 and region[0]<=pointer and pointer+length<=region[0]+region[3]and region[1]and region[5]in(4,8),'owned mapped span '+name)
   else:require(region==(0,)*7 and length==0,'absent result owner')
  private[phase]={}
  for name,baseline in [('CRT-argv','original-crt-argv.bin'),('CRT-environment','original-crt-environment.bin'),('Invoke-argv','invoke-entry-argv.bin')]:
   raw=blob(name);require(raw==(directory/baseline).read_bytes(),'fresh private vector identity/content/order '+name);root,count=vector(raw)
   if name=='CRT-argv':require((root,count)==(w[14],w[13]),'host CRT vector storage')
   if name=='CRT-environment':require(root==w[15],'CRT environment storage')
   private[phase][name]=dict(count=count,sha256=hashlib.sha256(raw).hexdigest())
  require(len(blob('target-data'))==len(blob('target-snapshot'))==6548 and len(blob('target-BSS'))==0xa06c,'full original target extents')
  observed[phase]=dict(words=w,errors=errors,fp=fp_semantic(blob('fp')),result=blob('result').decode('ascii'),table_sha256={name:hashlib.sha256(blob(name)).hexdigest()for name in ('mbctype','ctype','old-ctype')})
 before,after,mbbefore,mbafter,later=(observed[phase]for phase in PHASES)
 require(before['words'][1:11]==[1]+[0]*9 and before['words'][11]==base+0x3fbaa and before['words'][17]==0,'own exact isolated initial state')
 expected=before['words'][:];expected[2]=1252;expected[5]=0x409;expected[11]=after['words'][11];expected[17]=int(returned[4],16)
 require(after['words']==expected and after['words'][11]!=before['words'][11]and after['words'][17]and after['result']=='English_United States.1252\0','own actual isolated setlocale effects')
 require(mbbefore==after,'locale result/table/state lifetime to next actual API')
 expected=after['words'][:];expected[10]=1252
 require(mbafter['words']==expected and mbafter['words'][9]==0 and mbafter['result']==after['result'],'own actual SBCS _setmbcp effects')
 require(later==mbafter,'MB state/result/ownership to next target')
 for phase in PHASES:
  for suffix in ('fp','mbctype','old-ctype','target-data','target-snapshot','target-BSS'):
   require((directory/('locale-'+phase+'-'+suffix+'.bin')).read_bytes()==(directory/('locale-before-'+suffix+'.bin')).read_bytes(),'complete stable field '+suffix)
  if phase!='before':require((directory/('locale-'+phase+'-ctype.bin')).read_bytes()==(directory/'locale-after-ctype.bin').read_bytes(),'new ctype ownership/lifetime')
 require(not any(value&4 for value in(directory/'locale-mb-after-mbctype.bin').read_bytes()[1:]),'own SBCS table has no lead bytes')
 for suffix in ('target-data','target-snapshot'):
  actual=read((directory/('locale-before-'+suffix+'.bin')).read_bytes());wanted=seed('snapshot')if receipt['probe']=='restore'else defaults();require(actual==wanted,'scanner full initialized/saved typed state')
 require(read((directory/'compiler-data-000.bin').read_bytes())==defaults(),'all512 canonical entry defaults')
 return dict(schema=1,kind='own original-only canonical setlocale plus _setmbcp witness; no native API success or app acceptance',directory=str(directory),receipt_sha256=sha(directory/'receipt.json'),schema_sha256=sha(directory/'locale-schema.json'),provider_sha256=sha(crt),target_sha256=sha(target),checker_sha256=sha(Path(__file__)),phases=observed,private_vectors=private,artifacts_checked=len(receipt['artifacts']),full112_FP_unchanged=True,full_original_target_regions_unchanged=True,scanner_initialized_bytes=512,original_snapshot_bytes=6548,unmodeled_native_initialized_bytes=4200,ownership='actual static identities/aliased pointers/complete bounded table and return-string bytes plus mapped spans only; allocator object extents not inferred',seconds=time.perf_counter()-started)

if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('--output',type=Path);args=parser.parse_args();result=check(args.original)
 if args.output:
  require(not args.output.exists(),'immutable validation receipt destination exists');args.output.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(dict(status='PASS',seconds=result['seconds'],artifacts_checked=result['artifacts_checked'],native_success_claimed=False,receipt_sha256=result['receipt_sha256'])))
