"""No-formal scope-stack cursor reset with exact storage and void ABI."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='_reset_exception_scope_stack';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(d):
 assert d['matched']and d['function']==name and d['no_hidden_inputs_proof']and d['formals']==row['abi']['parameters']==[]and d['hidden_inputs']==row['abi']['hidden_inputs']==[]and d['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and d['return_contract']=='void; actual callers ignore EAX'
 assert d['original']['return_address']==0x65710 and d['native']['caller']=='prepare_inline_function'
validate(e);rejected=[]
for label,edit in [('invented Function formal',lambda d:d.update(formals=[dict(name='function',location='ECX')])),('invented old depth formal',lambda d:d.update(formals=[dict(name='depth',location='EDX')])),('unknown hidden operand',lambda d:d.update(hidden_inputs=['EAX'])),('wrong cleanup',lambda d:d.update(callee_stack_cleanup=4)),('invented return zero',lambda d:d.update(return_contract='EAX zero')),('invented EAX preservation',lambda d:d.update(return_contract='EAX preserved')),('wrong original caller',lambda d:d['original'].update(return_address=0)),('wrong native caller',lambda d:d['native'].update(caller='unknown'))]:
 d=copy.deepcopy(e);edit(d)
 try:validate(d)
 except(AssertionError,IndexError,KeyError):rejected.append(label)
 else:raise AssertionError('accepted '+label)
assert row['start']==0x65af0 and row['end']==0x65afb and original.read(row['start'],11)==bytes.fromhex('c705844e080000000000c3')
original_cell=0x84e84;canonical_cell=names['_exception_scope_stack_depth'];assert all(abs(canonical_cell-names[n])>=4 for n in ['_next_exception_scope_identifier','_exception_finally_depth','_prepared_exception_scopes','_exception_return_label'])
assert original.read(original_cell,4)==b'\0'*4
assert any(s['flags']&0x40 and original.base+s['rva']<=original_cell and original_cell+4<=original.base+s['rva']+s['virtual_size']for s in original.sections)
assert 'unsigned exception_scope_stack_depth;'in Path('src/exception-scope.c').read_text()
# Complete raw direct-call candidate set; these are both proven instruction
# boundaries inside the accepted straight-line69B preparation wrapper.
calls=[]
for section in original.sections:
 if not section['flags']&0x20:continue
 b=original.data[section['off']:section['off']+section['virtual_size']];base=original.base+section['rva']
 for i in range(len(b)-4):
  if b[i]==0xe8 and base+i+5+struct.unpack_from('<i',b,i+1)[0]==row['start']:calls.append(base+i)
assert calls==[0x6570b,0x65717]
assert original.read(0x65710,2)==original.read(0x6571c,2)==bytes.fromhex('8bce')
assert original.read(0x65b0b,2)==bytes.fromhex('8bc1')and original.read(0x65737,3)==bytes.fromhex('8b4110')
if args.abi_only:print('SEH reset no-formal/void/RET0 ABI,11B extent,distinct typed cursor and8 rejects pass');raise SystemExit

def run(image,side,initial,poison):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 cell=canonical_cell if side else original_cell;before=bytearray((i*17+13)&255 for i in range(36));struct.pack_into('<I',before,16,initial);expected=bytearray(before);expected[16:20]=b'\0'*4;u.mem_write(cell-16,bytes(before));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800)
 for r in [UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX]:u.reg_write(r,poison)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 terminal=[]
 def hook(m,a,size,_):
  if a==0x500000:terminal.append('return');m.emu_stop()
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=1000)
 assert terminal==['return']and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)
 assert bytes(u.mem_read(cell-16,len(expected)))==expected
 return dict(depth=0,adjacent_bytes_unchanged=True,return_contract='void EAX not compared',storage_sha256=hashlib.sha256(expected).hexdigest())
rows=[]
for initial,poison in itertools.product([0,1,0xffffffff,0x80000000,0x31415926],[0,1,0xdeadbeef,0xffffffff,0x80000000]):
 a=run(original,False,initial,poison);b=run(canonical,True,initial,poison);assert a==b;rows.append(dict(old_depth=initial,unused_caller_saved_poison=poison,result=a))
Path('cases/seh-reset-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-started,typed_owner=dict(original=hex(original_cell),canonical=hex(canonical_cell),bytes=4,type='unsigned',name='exception_scope_stack_depth'),scope='Whole11B original/canonical,25 olddepth/caller-saved poison cases, only fixed4B cleared and16 adjacentbytes eachside unchanged; empty formal/hidden sets proven exactMOV/RET and actualcaller, RET0/nonvolatile. Both known directcallers ignoreEAX and childentries overwriteEAX before reading it; no invented zero/preservedEAX promise. No olddepth input, Function formal, table/array initialization, callee/helper or guard. Arbitrary register poisons are unused-input controls, not application ABI formals.'),indent=2)+'\n');print(len(rows),'whole scope-stack reset controls and8 actual ABI negatives pass')
