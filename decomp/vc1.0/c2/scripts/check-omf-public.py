"""Actual public-emitter inputs and complete defined-kind output controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,os,shutil,struct,subprocess,tempfile,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--native-guard',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_emit_public@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def fields(d):
 h=bytes.fromhex(d['header_hex']);i=bytes.fromhex(d['identity_hex']);t=bytes.fromhex(d['type_hex']);v=bytes.fromhex(d['offset_hex']);assert len(h)==len(v)==4 and len(i)==len(t)==8
 return dict(offset=struct.unpack('<I',v)[0],kind=struct.unpack('<b',h[:1])[0],symbol_flags=struct.unpack('<H',h[2:])[0],segment_identifier=struct.unpack('<I',i[:4])[0],identifier=struct.unpack('<I',i[4:])[0],type=struct.unpack('<H',t[:2])[0],output_flags=struct.unpack('<H',t[2:4])[0],attributes=struct.unpack('<I',t[4:])[0],name_hex=d['name_hex'])
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6aef0'and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer'),dict(name='segment',location='EDX',type='Segment pointer')]
 expected=dict(offset=136,kind=1,symbol_flags=0x980,segment_identifier=2,identifier=121,type=0x1014,output_flags=5,attributes=0xa3,name_hex=b'_omf_callback\0'.hex());assert e['observed_fields']==expected
 for side in ['original','native']:
  d=e[side];assert d['symbol']==d['member']==e['formals'][0][side]and d['segment']==d['segment_owner']==e['formals'][1][side]and d['symbol']and d['segment']and d['bucket']==57 and d['index']==0 and fields(d)==expected;assert bytes.fromhex(d['header_hex'])[1]==0  # observed SymbolHeader.field0d naming discriminator
validate(e);reject=[]
for label,change in [('missing context formal',lambda e:e['formals'].pop()),('wrong context register',lambda e:e['formals'][1].update(location='entryESP+4')),('wrong Segment owner',lambda e:e['native'].update(segment_owner=0)),('wrong global owner',lambda e:e['original'].update(member=0)),('wrong offset',lambda e:e['native'].update(offset_hex='00000000')),('wrong type WORD',lambda e:e['native'].update(type_hex='ffff'+e['native']['type_hex'][4:])),('missing source name',lambda e:e['native'].update(name_hex='')),('invented stack input',lambda e:e.update(hidden_inputs=['record_type']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6aef0 and row['end']==0x6afbe and original.read(0x6af70,4)==b'\x83\xc4\x04\xc3'and original.read(0x6af14,4)==b'\x8b\x4c\x24\x0c'

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 symbol=0x400080;segment=0x400280;record=0x400400;text=0x400900;d=bytearray((i*13+5)&255 for i in range(84));struct.pack_into('<I',d,4,c['offset']);d[12]=c['kind'];d[48]=c['label'];struct.pack_into('<H',d,32,c['type']);struct.pack_into('<I',d,36,c['attrs']);u.mem_write(symbol,bytes(d));seg=bytes((i*17+3)&255 for i in range(72));u.mem_write(segment,seg);buf=bytearray((i*29+7)&255 for i in range(1004));struct.pack_into('<5I',buf,0,0x91,segment,0,c['count'],record+23);u.mem_write(record,bytes(buf));u.mem_write(text,b'shown\0');u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol);u.reg_write(UC_X86_REG_EDX,segment)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 specs=[('begin','@omf_begin_record@8',0x6b480),('display','@symbol_display_name@4',0x4e000),('string','@omf_append_string@8',0x6c4e0),('index','@omf_write_index@8',0x6c2b0),('error','@compiler_internal_error@8',0x6fb70)];children={names[n]if native else a:k for k,n,a in specs};events=[];terminal=None
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal='return';m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);result=0xdeadbeef
  if k=='error':
   raw=b''
   for i in range(32):
    b=bytes(m.mem_read(ecx+i,1));raw+=b
    if b==b'\0':break
   assert edx==0x638 and raw==b'@(#)omf.c:1.10\0';events.append(['error',edx]);terminal='internal-error';m.emu_stop();return
  if k=='begin':
   typ=0x91 if c['kind']==1 or c['kind']==4 and c['attrs']&0xf0==0x50 else 0xb7;assert ecx==typ and edx==segment;events.append(['begin',typ,'segment']);result=record
   if c['mutate']:m.mem_write(symbol+4,struct.pack('<I',0x10203040));d[4:8]=struct.pack('<I',0x10203040)
  elif k=='display':
   assert ecx==symbol;events.append(['display']);result=text
   if c['mutate']:m.mem_write(record+12,struct.pack('<I',7));m.mem_write(record+16,struct.pack('<I',record+64));struct.pack_into('<I',buf,12,7);struct.pack_into('<I',buf,16,record+64)
  elif k=='string':
   assert ecx==record and edx==text;events.append(['string']);cursor=struct.unpack('<I',m.mem_read(record+16,4))[0];count=struct.unpack('<I',m.mem_read(record+12,4))[0];m.mem_write(cursor,b'\x01Z');m.mem_write(record+12,struct.pack('<I',(count+2)&0xffffffff));m.mem_write(record+16,struct.pack('<I',cursor+2));buf[cursor-record:cursor-record+2]=b'\x01Z';struct.pack_into('<I',buf,12,(count+2)&0xffffffff);struct.pack_into('<I',buf,16,cursor+2)
   if c['mutate']:m.mem_write(symbol+32,struct.pack('<H',c['type']^0xffff));d[32:34]=struct.pack('<H',c['type']^0xffff)
  else:
   typ=c['type']^0xffff if c['mutate']else c['type'];typ=typ if typ<0x8000 else typ-65536;assert ecx==record and edx==typ&0xffffffff;cursor=struct.unpack('<I',m.mem_read(record+16,4))[0];assert bytes(m.mem_read(cursor-4,4))==struct.pack('<I',c['offset']);events.append(['index',typ])
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=1500)
 if c['kind']==3 and c['label']!=4:
  assert terminal=='internal-error'and events==[['error',0x638]]and bytes(u.mem_read(symbol,84))==d and bytes(u.mem_read(record,1004))==buf;return dict(terminal=terminal,events=events)
 count,cursor=struct.unpack('<2I',buf[12:20]);buf[cursor-record:cursor-record+4]=struct.pack('<I',c['offset']);struct.pack_into('<2I',buf,12,(count+4)&0xffffffff,cursor+4)
 assert terminal=='return'and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(symbol,84))==d and bytes(u.mem_read(segment,72))==seg and bytes(u.mem_read(record,1004))==buf and bytes(u.mem_read(text,6))==b'shown\0'and len(events)==4
 return dict(terminal=terminal,events=events,payload_hex=struct.pack('<I',c['offset']).hex(),record_sha256=hashlib.sha256(buf).hexdigest())
rows=[]
for (kind,attrs),offset,typ,count,mutate in itertools.product([(1,0),(3,0),(4,0x50),(4,0x40),(4,0x10)],[136,0x89abcdef],[0x1014,0xffff],[0,0xffffffff],[False,True]):
 c=dict(kind=kind,attrs=attrs,label=4,offset=offset,type=typ,count=count,mutate=mutate);a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
for label in [0,3,255]:
 c=dict(kind=3,attrs=0,label=label,offset=136,type=0x1014,count=0,mutate=False);a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
guard_path=Path('cases/omf-public-native-guard.json')
if args.native_guard:
 from case_paths import C2_SCRATCH,native_work,windows
 import wine_tool as wt
 fixture=Path(cfg['cases']['omf']['fixture_manifest']).parent;phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2'];scratch=Path(tempfile.mkdtemp(prefix='public-kind-guard-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
 for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
 env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(wt.TC/'bin')),RECON_PROGRAM_PATH=windows(wt.TC/'bin/C23232.EXE'))
 command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_emit_public','-ex','run > inferior.stdout','-ex','print /d symbol->kind','-ex','set variable symbol->kind = 2','-ex','continue','-ex','x/i $pc-1','-ex','bt 2',str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);seconds=time.perf_counter()-t;out=Path('cases/omf-public-native-guard.txt');out.write_bytes(r.stdout);assert r.returncode==0 and b'$1 = 1'in r.stdout and b'SIGTRAP'in r.stdout and b'int3'in r.stdout and b'omf_emit_public'in r.stdout,r.stdout;guard=dict(path=str(out),sha256=hashlib.sha256(r.stdout).hexdigest(),seconds=seconds,actual_kind=1,injected_kind=2,scope='Native-only controlled unsupported-kind boundary; original unsupported local type is uninitialized and is neither observed as a defined ABI input nor seeded/emulated. No original equality or frontend-validity claim for kind2.');guard_path.write_text(json.dumps(guard,indent=2)+'\n')
guard=json.loads(guard_path.read_text());assert hashlib.sha256(Path(guard['path']).read_bytes()).hexdigest()==guard['sha256']
Path('cases/omf-public-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,native_guard=guard,seconds=time.perf_counter()-started,scope='Whole206-byte emitter on defined kind1/3/4 and genuine invalid-label638 diagnostic: cached entry offset despite child mutation, selected public/local type, post-string record count/cursor reload, LE32 prefix/count wrap and signed typeWORD reload after children, all84 Symbol/72 Segment/1004 record/6 text bytes and callee-saved/RET0 checked.83 controls mock children, error stops genuine diagnostic; unsupported original kind uninitialized local type remains a source assumption, exact native default traps. Known marker/6b710 callers do not establish all public reachability; real other kind holds. No invented default/guard in Win32 or extra incoming parameter.'),indent=2)+'\n');print(len(rows),'whole defined-kind OMF public controls and8 actual ABI rejects pass; native unsupported kind traps')
