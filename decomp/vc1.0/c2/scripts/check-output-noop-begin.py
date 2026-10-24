"""Four unused begin-function callback inputs and exact RET8 stack ABI."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@output_noop_function@16';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
params=[dict(name=n,location=l,type=t)for n,l,t in [('symbol','ECX','FunctionSymbol pointer'),('filename','EDX','char pointer'),('frame','entryESP+4','signed32'),('line','entryESP+8','unsigned32')]]
def validate(e):
 assert e['matched']and e['function']==name and e['previous_provisional_name']=='_output_noop_function'and e['original_va']=='0x68d70'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==8 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==params and all(f['use']=='unused by original RET8 body'for f in e['formals'])
 assert e['shape']==dict(symbol_nonnull=True,filename_nonnull=True,frame=0,line=34)and e['original']['return_address']==0x68278 and e['native']['caller']=='emit_backend_work'and e['native']['frame_owner']==0
 for side in ['original','native']:
  d=e[side];assert d['symbol']==d['symbol_owner']and d['symbol']and d['filename']==d['filename_owner']and d['filename']and d['frame']==int(bool(d['frame_flags']&0x10 or d['symbol_flags']&0x1000))==0 and d['line']==d['line_owner']==34
  assert [f[side]for f in e['formals']]==[d[k]for k in ['symbol','filename','frame','line']]
validate(e);reject=[]
for label,change in [('missing unused filename',lambda e:e['formals'].pop(1)),('missing unused line',lambda e:e['formals'].pop()),('wrong frame slot',lambda e:e['formals'][2].update(location='entryESP+8')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=0)),('wrong symbol owner',lambda e:e['native'].update(symbol_owner=0)),('wrong filename owner',lambda e:e['original'].update(filename_owner=0)),('wrong frame owner',lambda e:e['native'].update(frame_owner=1)),('wrong line owner',lambda e:e['original'].update(line_owner=35)),('invented pointee input',lambda e:e.update(hidden_inputs=['filename.contents']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x68d70 and row['end']==0x68d73 and original.read(0x68d70,3)==b'\xc2\x08\0'
assert Path('src/output.c').read_text().count('output_callbacks.begin_function = output_noop_function;')==2
assert 'void __fastcall output_noop_function(FunctionSymbol *symbol, char *filename, int frame, unsigned line);'in Path('src/output.h').read_text()
assert '__declspec(naked) void output_noop_function'not in Path('src/stubs.c').read_text()
if args.abi_only:print('Begin-function no-op actual4 unused formals/9 rejects,RET8 extent and typed callback bindings pass');raise SystemExit

def run(image,native,formals,flags):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in[(0x10000,0x100000),(0x300000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 symbol,filename,frame,line=formals;u.mem_write(0x300800,struct.pack('<4I',0x500000,frame,line,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol);u.reg_write(UC_X86_REG_EDX,filename);u.reg_write(UC_X86_REG_EAX,0x89abcdef);u.reg_write(UC_X86_REG_EFLAGS,flags);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 def access(m,kind,address,size,value,_):assert 0x300000<=address and address+size<=0x310000, 'No global or pointee input/output in disabled callback'
 u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access);u.emu_start(names[name]if native else row['start'],0x500000,count=100)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x30080c and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300804,12))==struct.pack('<3I',frame,line,0xfeedbeef)
 return dict(stack_cleanup=8,no_pointee_or_global_access=True,returns='void')
rows=[]
for formals,flags in itertools.product([(0,0,0,0),(0xdeadbeef,0xffffffff,1,34),(0xffffffff,0x12345678,0x80000000,0xffffffff),(0x400080,0x400180,0xffffffff,0x80000000)],[0x202,0x203,0x246]):
 a=run(original,False,formals,flags);b=run(canonical,True,formals,flags);assert a==b;rows.append(dict(formals=formals,incoming_flags=flags,result=a))
Path('cases/output-noop-begin-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole3-byte originalRET8 versus canonical empty four-argument fastcall C, exact entryESP+12/callee-saved and original stack argument/guard preservation. Memory hooks reject any non-stack read/write; arbitrary unmapped pointer/scalar request shapes prove unused formals, not valid frontend data. Incoming EFLAGS varied only as unused inputs; output flags/void EAX are caller-clobbered and not compared. Actual4 unused args all concretely observed by caller global/local aliases and metadata calculation, filename payload not needed. Named callback/cached logical alias refinement preserves C identity; no assembly adapter or invented no-argument ABI.'),indent=2)+'\n');print(len(rows),'whole begin-noop ABI controls and9 actual ABI rejects pass')
