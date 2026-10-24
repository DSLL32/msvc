"""MODEND no-formal ABI and complete one-byte append after begin8a/NULL."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='_omf_write_module_end';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6aad0'and e['formals']==row['abi']['parameters']==[]and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['no_hidden_inputs_proof']and e['original']['return_address']==0x6aaac and e['native']['caller']=='omf_finish_module'
validate(e);reject=[]
for label,change in [('invented record formal',lambda e:e.update(formals=[dict(name='record',location='EAX')])),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong caller',lambda e:e['native'].update(caller='omf_begin_record')),('wrong original call',lambda e:e['original'].update(return_address=0)),('invented globals input',lambda e:e.update(hidden_inputs=['record_cache']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6aad0 and row['end']==0x6aaea and original.read(0x6aae9,1)==b'\xc3'and original.read(0x6aad2,5)==bytes.fromhex('b98a000000')
if args.abi_only:print('MODEND actual no-formal/caller/5 rejects and26-byte begin8a extent pass');raise SystemExit

def run(image,native,count,position,poison,record):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in[(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 buf=bytearray((i*29+7)&255 for i in range(1004));struct.pack_into('<2I',buf,12,count,record+position);u.mem_write(record,bytes(buf));u.mem_write(0x300800,struct.pack('<2I',0x500000,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800)
 for r in[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX]:u.reg_write(r,poison)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 begin=names['@omf_begin_record@8']if native else 0x6b480;events=[];terminal=False
 def access(m,kind,address,size,value,_):
  if address<record+1004 and address+size>record:assert events,'No incoming record exists before begin returns'
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal=True;m.emu_stop();return
  if a!=begin:return
  assert not events and m.reg_read(UC_X86_REG_ECX)==0x8a and m.reg_read(UC_X86_REG_EDX)==0 and bytes(m.mem_read(record,1004))==buf;events.append(['begin',0x8a,0]);esp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EAX,record);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access);u.emu_start(names[name]if native else row['start'],0,count=300);buf[position]=0;struct.pack_into('<2I',buf,12,(count+1)&0xffffffff,record+position+1)
 assert terminal and len(events)==1 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300804,4))==struct.pack('<I',0xfeedbeef)and bytes(u.mem_read(record,1004))==buf
 return dict(events=events,record_sha256=hashlib.sha256(buf).hexdigest(),stack_cleanup=0)
rows=[]
for count,position,poison,record in itertools.product([0,1,0xffffffff],[20,23,1003],[0,0x89abcdef,0xffffffff],[0x400400,0x400900]):
 a=run(original,False,count,position,poison,record);b=run(canonical,True,count,position,poison,record);assert a==b;rows.append(dict(count=count,cursor_offset=position,incoming_registers=poison,returned_record=record,result=a))
Path('cases/omf-modend-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole26-byte no-formal MODEND versus independent one-zero-byte/count+1/cursor+1 model.54 original/canonical states cover two child-returned record objects,count wrap,final writable byte,caller-register poison,begin8a/NULL,RET0/saved regs/stack/full owned guards. Memory hooks reject any record access before begin child; destination is future child output, no input pointer/snapshot/seed. Child mocked, no cold child behavior or final output inferred by controls. Actual caller return6aaac/native finalizer observed separately; full exact case completion is separate.'),indent=2)+'\n');print(len(rows),'whole MODEND controls/5 actual ABI rejects pass')
