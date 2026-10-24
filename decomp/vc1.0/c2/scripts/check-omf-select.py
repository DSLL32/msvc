"""Actual selected Segment and complete byte-combine/returned-owner controls."""
from pathlib import Path
import argparse,copy,hashlib,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_select_segment@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='segment',location='ECX',type='Segment pointer')]
 assert e['native']['segment_size']==72
 for side in ['original','native']:
  d=e[side];assert d['segment']==d['owner']==e['formals'][0][side]and d['segment']and len(bytes.fromhex(d['segment_hex']))==72 and bytes.fromhex(d['segment_hex'])[53]==0
validate(e);reject=[]
for label,change in [('missing Segment',lambda e:e.update(formals=[])),('wrong register',lambda e:e['formals'][0].update(location='EDX')),('wrong owner',lambda e:e['native'].update(owner=0)),('wrong combine',lambda e:e['native'].update(segment_hex=e['native']['segment_hex'][:106]+'07'+e['native']['segment_hex'][108:])),('hidden EAX',lambda e:e.update(hidden_inputs=['EAX']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b6a0 and row['end']==0x6b6bf and original.read(0x6b6be,1)==b'\xc3'

if args.abi_only:
 print(name,'actual ABI/negative/extent/storage checks pass; full semantic grid is explicit');raise SystemExit

def run(image,native,combine,returned):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 segment=0x400080;pool=names['_omf_records']if native else 0x7a8dc;current=names['_omf_current_record']if native else 0x7b88c;child=names['@omf_begin_record@8']if native else 0x6b480;data=bytearray((i*13+5)&255 for i in range(72));data[53]=combine;u.mem_write(segment,bytes(data));before=bytes((i*17+3)&255 for i in range(4016));u.mem_write(pool,before);u.mem_write(current,struct.pack('<I',pool+1004));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,segment);events=[]
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a!=child:return
  ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);assert ecx==(0xc3 if combine==7 else 0xa1)and edx==segment;events.append(ecx);m.mem_write(current,struct.pack('<I',pool+2008));m.reg_write(UC_X86_REG_EAX,pool+returned*1004);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=300)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(segment,72))==data and bytes(u.mem_read(pool,4016))==before and struct.unpack('<I',u.mem_read(current,4))[0]==pool+returned*1004 and len(events)==1
 return dict(record_type=events[0],returned_slot=returned)
rows=[]
for combine in [0,1,2,6,7,8,128,255]:
 for returned in [0,3]:
  a=run(original,False,combine,returned);b=run(canonical,True,combine,returned);assert a==b;rows.append(dict(combine=combine,result=a))
Path('cases/omf-select-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole31-byte original/canonical selector: byte subtract7/compare1 means exactly combine7, a1/c3 context args, returned record overwrites child-mutated current pointer, all Segment72/pool4016 bytes and callee-saved/RET0 unchanged.16 bounded controls mock begin only; actual debug_symbols_segment combine0 observed, raw failed parser assumption2 retained without recapture. Callback now has typed fastcall4 ABI; historical old-name evidence preserved.'),indent=2)+'\n');print(len(rows),'whole OMF selector controls and5 actual ABI rejects pass')
