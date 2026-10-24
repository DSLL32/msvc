"""SEH preparation wrapper: opaque pointer, contextual reset cells and calls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@prepare_inline_function@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(d):
 assert d['matched']and d['function']==name and d['no_hidden_inputs_proof']and d['hidden_inputs']==row['abi']['hidden_inputs']==[]and d['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and d['return_contract']=='void; caller ignores EAX'
 assert [{k:f[k]for k in ['name','location','type']}for f in d['formals']]==row['abi']['parameters']==[dict(name='function',location='ECX',type='opaque Function pointer')]
 for side in ['original','native']:
  v=d[side];assert v['function']==v['current']==v['parent']==d['formals'][0][side]and v['function']
 assert d['original']['return_address']==0x4c3a8 and d['native']['caller']=='compile_functions'
validate(e);rejected=[]
for label,edit in [('missing formal',lambda d:d['formals'].clear()),('wrong register',lambda d:d['formals'][0].update(location='EDX')),('wrong pointee contract',lambda d:d['formals'][0].update(type='complete Function snapshot')),('lost current alias',lambda d:d['original'].update(current=0)),('lost caller alias',lambda d:d['native'].update(parent=0)),('unknown hidden input',lambda d:d.update(hidden_inputs=['?'])),('wrong cleanup',lambda d:d.update(callee_stack_cleanup=4)),('invented return contract',lambda d:d.update(return_contract='EAX zero'))]:
 d=copy.deepcopy(e);edit(d)
 try:validate(d)
 except(AssertionError,IndexError,KeyError):rejected.append(label)
 else:raise AssertionError('accepted '+label)
assert row['start']==0x656e0 and row['end']==0x65725 and original.read(0x65724,1)==b'\xc3'
for a,target in [(0x6570b,0x65af0),(0x65712,0x65b00),(0x65717,0x65af0),(0x6571e,0x65730)]:assert original.read(a,5)==b'\xe8'+struct.pack('<i',target-a-5)
# Independent typed owners, not four aliases whose zero values hide storage.
original_cells=[0x7b9a4,0x7b9a0,0x7b998,0x7b99c];cell_names=['_next_exception_scope_identifier','_exception_finally_depth','_prepared_exception_scopes','_exception_return_label'];canonical_cells=[names[n]for n in cell_names];assert len(set(canonical_cells))==4 and all(abs(a-b)>=4 for i,a in enumerate(canonical_cells)for b in canonical_cells[i+1:])
for a in original_cells:assert any(s['flags']&0x80 and original.base+s['rva']<=a and a+4<=original.base+s['rva']+s['virtual_size']for s in original.sections)
source=Path('src/exception-scope.c').read_text();header=Path('src/exception-scope.h').read_text();assert 'ExceptionScope *prepared_exception_scopes;'in source and 'Node *exception_return_label;'in source and 'unsigned exception_finally_depth, next_exception_scope_identifier;'in source
children=[('_reset_exception_scope_stack',0x65af0),('@collect_exception_scopes@4',0x65b00),('@rewrite_exception_expressions@4',0x65730)]
for n,a in children:
 r=next(v for v in cfg['driver']if v['name']==n);assert r['start']==a
 if n not in cfg['accepted']:assert r['provisional']and r['end']==a+1
if args.abi_only:print('SEH wrapper sole opaque Function/RET0 ABI,69B extent,4 distinct typed BSS owners and8 rejects pass');raise SystemExit

def run(image,side,initial,mutations,pointer,stop):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 cells=canonical_cells if side else original_cells
 for i,a in enumerate(cells):u.mem_write(a,struct.pack('<I',(initial+i*0x1020304)&0xffffffff))
 u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,pointer);u.reg_write(UC_X86_REG_EDX,0xdeadbeef)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 edges={names[n]if side else a:n for n,a in children};expected=[0]*4;events=[];terminal=[];order=['_reset_exception_scope_stack','@collect_exception_scopes@4','_reset_exception_scope_stack','@rewrite_exception_expressions@4']
 def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
 def hook(m,a,size,_):
  if a==0x500000:terminal.append('return');m.emu_stop();return
  if a not in edges:return
  child=edges[a];index=len(events);assert index<4 and child==order[index]and [word(c)for c in cells]==expected
  if child!='_reset_exception_scope_stack':assert m.reg_read(UC_X86_REG_ECX)==pointer
  events.append([child,'same Function'if child!='_reset_exception_scope_stack'else'no formal',list(expected)])
  if index==stop:terminal.append('child-stop');m.emu_stop();return
  if mutations:
   for i,c in enumerate(cells):expected[i]=((index+1)*0x1020304+i*0x110011)&0xffffffff;m.mem_write(c,struct.pack('<I',expected[i]))
  esp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EAX,0xdead0000|index);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=1000)
 assert [word(c)for c in cells]==expected
 if stop<4:assert terminal==['child-stop']and len(events)==stop+1
 else:assert terminal==['return']and len(events)==4 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)
 return dict(terminal=terminal[0],events=events,cells=expected)
rows=[]
for initial,mutations,pointer,stop in itertools.product([0,0xffffffff,0x31415926],[False,True],[0,0xdeadbeef,0x400000],range(5)):
 a=run(original,False,initial,mutations,pointer,stop);b=run(canonical,True,initial,mutations,pointer,stop);assert a==b;rows.append(dict(initial=initial,children_mutate_cells=mutations,function_pointer=pointer,stop_after_child_index=stop,result=a))
Path('cases/seh-prepare-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-started,typed_owners=[dict(name=n,original=hex(a),canonical=hex(b),bytes=4)for n,a,b in zip(cell_names,original_cells,canonical_cells)],scope='Whole69B original/canonical wrapper, all4 contextual cells cleared before first child, four distinct storage identities, reset/collect/reset/rewrite exact order, original opaqueFunction retained across all caller-saved clobbers. Child mutations remain visible at each later call and final return; first/second-reset and collect/rewrite stops preserve honest partial-body scope. RET0/nonvolatile at normal return; voidEAX unused, no invented value claim. NULL/unmappedFunction tests prove no parent dereference with mocked children, not supported actual backend inputs. First actual case traps at independent reset; no childbody or1616B envelope credit.'),indent=2)+'\n');print(len(rows),'whole SEH-wrapper controls and8 actual ABI negatives pass')
