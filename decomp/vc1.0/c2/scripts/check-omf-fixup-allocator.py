"""No-formal counter ABI, typed BSS pool and whole bounded address allocator."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='_omf_allocate_fixup';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6bd10'and e['formals']==row['abi']['parameters']==[]and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['no_hidden_inputs_proof']
 assert e['original']['count']==e['native']['count']==0 and e['original']['return_address']==0x6b99c and e['native']['caller']=='omf_add_fixup'
validate(e);reject=[]
for label,change in [('invented ECX formal',lambda e:e.update(formals=[dict(name='entry',location='ECX')])),('wrong counter input',lambda e:e['native'].update(count=1)),('wrong original count',lambda e:e['original'].update(count=-1)),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('invented old pool input',lambda e:e.update(hidden_inputs=['old_pool']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6bd10 and row['end']==0x6bd46 and original.read(0x6bd45,1)==b'\xc3'
assert original.read(0x6bd16,6)==bytes.fromhex('81f9fa000000')and original.read(0x6bd34,10)==bytes.fromhex('8d04498d04c554910700')
for image,pool,counter in[(original,0x79154,0x85094),(canonical,names['_omf_fixups'],names['_omf_fixup_count'])]:
 section=next(s for s in image.sections if image.base+s['rva']<=pool<image.base+s['rva']+s['virtual_size']);assert section['flags']&0x80 and pool+6000<=image.base+section['rva']+section['virtual_size']and not pool<=counter<pool+6000
 counter_section=next(s for s in image.sections if image.base+s['rva']<=counter<image.base+s['rva']+s['virtual_size']);assert counter_section['flags']&0x80 or image.read(counter,4)==bytes(4)  # BSS has loader-zero state, not raw-file bytes at offset0
assert 0x79154+250*24==0x7a8c4 and len({names['_omf_fixups'],names['_omf_fixup_count'],names['_omf_fixup_threads']})==3
if args.abi_only:print('Fixup allocator actual no-formal ABI/5 rejects,54-byte extent and250x24 typed BSS/counter storage pass');raise SystemExit

def run(image,native,count,poison):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in[(0x10000,0x100000),(0x300000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 pool=names['_omf_fixups']if native else 0x79154;counter=names['_omf_fixup_count']if native else 0x85094;error=names['@compiler_internal_error@8']if native else 0x6fb70;u.mem_write(counter,struct.pack('<I',count));u.mem_write(0x300800,struct.pack('<2I',0x500000,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800)
 for r in[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX]:u.reg_write(r,poison)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 terminal=None
 def access(m,kind,address,size,value,_):assert address+size<=pool or address>=pool+6000, 'Allocator must not read or write any old pool entry'
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal='return';m.emu_stop()
  elif a==error:
   ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);assert count>=250 and edx==0x772 and bytes(m.mem_read(ecx,15))==b'@(#)omf.c:1.10\0';terminal='internal';m.emu_stop()
 u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access);u.emu_start(names[name]if native else row['start'],0,count=200);actual=struct.unpack('<I',u.mem_read(counter,4))[0]
 if count<250:
  assert terminal=='return'and actual==count+1 and u.reg_read(UC_X86_REG_EAX)==pool+24*count and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300804,4))==struct.pack('<I',0xfeedbeef);result=dict(terminal=terminal,slot=count,byte_offset=24*count,next_count=actual)
 else:assert terminal=='internal'and actual==count;result=dict(terminal=terminal,code=0x772,next_count=actual)
 return result
rows=[]
for count,poison in itertools.product([0,1,124,248,249,250,251,0x7fffffff],[0,0x89abcdef,0xffffffff]):
 a=run(original,False,count,poison);b=run(canonical,True,count,poison);assert a==b;rows.append(dict(count=count,incoming_caller_registers=poison,result=a))
Path('cases/omf-fixup-allocator-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,pool=dict(original_start='0x79154',original_end='0x7a8c4',entries=250,stride=24,bytes=6000,original_bss=True,canonical_bss=True),scope='Whole54-byte original/canonical allocator, no incoming formals; counter0/1/124/248/249 -> typed base+24*index and increment,250/251/INTMAX -> actual772 diagnostic before selection. Three incoming caller-register poisons, RET0/callee-saved/stack checked. Memory hooks reject any read/write of old pool entries: pool is address output, no snapshot/seed. Signed negative indices not modeled as valid; known lifecycle is BSS0/allocatorincrement/writerreset0, no invented native guard, real negative counter would hold for producer/ABI review. Error stops, no returning-fatal claim. Struct24/offsets compile checked, original LEAs prove250x24 extent, explicit typed BSS pool6000 introduced at encountered allocator, not copied data.'),indent=2)+'\n');print(len(rows),'whole fixup allocator controls and5 actual ABI rejects pass')
