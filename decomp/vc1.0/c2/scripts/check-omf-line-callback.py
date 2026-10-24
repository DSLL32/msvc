"""Source-line callback ABI, current-symbol reload and conditional child selection."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_output_debug_record@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
params=[dict(name=k,location=l,type='unsigned32')for k,l in [('line','ECX'),('offset','EDX')]]
def validate(e):
 assert e['matched']and e['function']==name and e['previous_provisional_name']=='_omf_output_debug_record'and e['original_va']=='0x6b740'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in['name','location','type']}for f in e['formals']]==row['abi']['parameters']==params
 assert e['shape']==dict(line=34,offset=0,flags=1,segment_id=5)and e['original']['return_address']==0x68434 and e['native']['caller']=='emit_machine_instructions'
 for side in ['original','native']:
  d=e[side];assert d['line']==d['source_line']==34 and d['offset']==d['caller_offset']==0 and d['flags']==1 and d['segment_id']==5 and d['symbol']and[f[side]for f in e['formals']]==[d['line'],d['offset']]
validate(e);reject=[]
for label,change in [('missing offset',lambda e:e['formals'].pop()),('wrong offset register',lambda e:e['formals'][1].update(location='entryESP+4')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong line alias',lambda e:e['original'].update(source_line=0)),('wrong offset alias',lambda e:e['native'].update(caller_offset=1)),('wrong current symbol',lambda e:e['native'].update(symbol=0)),('wrong flags',lambda e:e['native'].update(flags=32)),('wrong segment ID',lambda e:e['original'].update(segment_id=6)),('invented lookup output',lambda e:e.update(hidden_inputs=['segment']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b740 and row['end']==0x6b780 and original.read(0x6b77f,1)==b'\xc3'and original.read(0x6b753,6)==bytes.fromhex('8b0dac310800')
assert 'void __fastcall omf_output_debug_record(unsigned line, unsigned offset);'in Path('src/output.h').read_text()and'output_callbacks.debug_record = omf_output_debug_record;'in Path('src/output.c').read_text()
if args.abi_only:print('Line callback actual2 formals/caller aliases/9 rejects,64-byte extent/reload and typed callback pass');raise SystemExit

def run(image,native,line,offset,flags,mode):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in[(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 symbol=0x400080;other=0x400180;segment=0x400400;current=names['_current_symbol']if native else 0x831ac;d=bytearray((i*13+5)&255 for i in range(64));struct.pack_into('<H',d,14,flags);struct.pack_into('<I',d,24,5);d2=bytearray(d);struct.pack_into('<H',d2,14,flags^32);struct.pack_into('<I',d2,24,99);u.mem_write(symbol,bytes(d));u.mem_write(other,bytes(d2));u.mem_write(current,struct.pack('<I',symbol));u.mem_write(0x300800,struct.pack('<2I',0x500000,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,line);u.reg_write(UC_X86_REG_EDX,offset);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 lookup=names['@lookup_global_symbol@4']if native else 0x4dce0;normal=names['@omf_write_line@12']if native else 0x6b780;comdat=names['@omf_write_comdat_line@12']if native else 0x6b7f0;events=[];terminal=False;owner=symbol
 def hook(m,a,size,_):
  nonlocal terminal,owner
  if a==0x500000:terminal=True;m.emu_stop();return
  if a not in[lookup,normal,comdat]:return
  esp=m.reg_read(UC_X86_REG_ESP)
  if a==lookup:
   assert not events and m.reg_read(UC_X86_REG_ECX)==5;events.append(['lookup',5]);cleanup=0
   if mode=='flip':struct.pack_into('<H',d,14,flags^32);m.mem_write(symbol,bytes(d))
   elif mode=='switch':owner=other;m.mem_write(current,struct.pack('<I',other))
  else:
   post_flags=flags if mode=='unchanged'else flags^32;chosen=comdat if post_flags&32 else normal;assert len(events)==1 and a==chosen and m.reg_read(UC_X86_REG_ECX)&65535==line&65535 and m.reg_read(UC_X86_REG_EDX)==offset and struct.unpack('<I',m.mem_read(esp+4,4))[0]==segment;events.append(['comdat'if a==comdat else'normal',line&65535,offset]);cleanup=4
  m.reg_write(UC_X86_REG_EAX,segment if a==lookup else 0xdeadbeef);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4+cleanup);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=400)
 assert terminal and len(events)==2 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300804,4))==struct.pack('<I',0xfeedbeef)and bytes(u.mem_read(symbol,64))==d and bytes(u.mem_read(other,64))==d2 and struct.unpack('<I',u.mem_read(current,4))[0]==owner
 return dict(events=events,current_owner='other'if owner==other else'original')
rows=[]
for line,offset,flags,mode in itertools.product([0,34,65535,0x12345678,0xffffffff],[0,0x89abcdef,0xffffffff],[1,32,0xffff,0xffdf],['unchanged','flip','switch']):
 a=run(original,False,line,offset,flags,mode);b=run(canonical,True,line,offset,flags,mode);assert a==b;rows.append(dict(line=line,offset=offset,flags=flags,lookup_mutation=mode,result=a))
Path('cases/omf-line-callback-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole64-byte callback: two saved input registers across lookup, original current-symbol ID before child, flags/global pointer reloaded after child, normal/COMDAT selected by post-child bit20, low16 line/unchanged32 offset/returned segment forwarded.180 original/canonical states include in-place flag flip and swapped current-symbol pointer, high-bit inputs, child RET4, parent RET0/saved regs/owned guards. Children mocked, both remain named traps until their own original frontier; no cold line-record output claim. Actual line34/offset0/caller aliases/current flags1/segmentID5 observed. No injected input/result or assembly adapter.'),indent=2)+'\n');print(len(rows),'whole line callback controls/9 actual ABI rejects pass')
