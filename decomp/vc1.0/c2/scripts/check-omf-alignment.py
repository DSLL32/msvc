"""Defined OMF alignment mappings and explicit unsupported native boundary."""
from pathlib import Path
import argparse,copy,hashlib,json,os,shutil,struct,subprocess,tempfile,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--native-guard',action='store_true');args=ap.parse_args();started=time.perf_counter()
original,canonical,cfg,_,_=indexes();names=publics();name='@omf_alignment@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert e['formals']==[dict(name='alignment',location='ECX',type='unsigned32',value=4)]and row['abi']['parameters']==[dict(name='alignment',location='ECX',type='unsigned32')]
 assert e['native']['values']==e['original_values']==e['original_raw_values']==[4]
validate(e);reject=[]
for label,change in [('missing formal',lambda e:e.update(formals=[])),('wrong ECX',lambda e:e['formals'][0].update(location='EDX')),('wrong actual alignment',lambda e:e['native'].update(values=[8])),('invented hidden input',lambda e:e.update(hidden_inputs=['EAX'])),('wrong RET cleanup',lambda e:e.update(callee_stack_cleanup=4))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6a310 and row['end']==0x6a346
assert original.read(0x6a324,1)==b'\xc3'and original.read(0x6a345,1)==b'\xc3'
raw_entry_eax=int((p.parent/'original.txt').read_text().splitlines()[0].split()[3],16);assert raw_entry_eax==0x7a8dc

def run(image,entry,alignment,eax):
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x100000);u.mem_map(0x300000,0x10000);u.mem_map(0x400000,0x1000)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,alignment);u.reg_write(UC_X86_REG_EAX,eax);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 def stop(m,a,size,_):
  if a==0x400000:m.emu_stop()
 u.hook_add(UC_HOOK_CODE,stop);u.emu_start(entry,0,count=200)
 assert u.reg_read(UC_X86_REG_EIP)==0x400000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)
 return u.reg_read(UC_X86_REG_EAX)
controls=[]
for alignment,want in [(1,1),(2,2),(4,5),(16,3)]:
 for eax in [0,raw_entry_eax,0xffffffff]:
  assert run(original,row['start'],alignment,eax)==run(canonical,names[name],alignment,eax)==want;controls.append(dict(alignment=alignment,incoming_eax=eax,result=want))
unsupported=[]
for alignment in [0,3,8,32,0xffffffff]:
 for eax in [0,raw_entry_eax,0xffffffff]:
  assert run(original,row['start'],alignment,eax)==eax;unsupported.append(dict(alignment=alignment,incoming_eax=eax,returned_eax=eax))
source=Path('src/omf-output.c').read_text();body=source[source.index('unsigned __fastcall omf_alignment(unsigned alignment)\n{'):].split('\n}\n',1)[0];assert '#ifdef RECON_NATIVE_TEST\n    __asm int 3\n#endif' in body and body.count('return ')==4
native_guard=None
if args.native_guard:
 from case_paths import C2_SCRATCH,native_work,windows
 import wine_tool as wt
 fixture=Path(cfg['cases']['omf']['fixture_manifest']).parent;phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2'];scratch=Path(tempfile.mkdtemp(prefix='alignment-guard-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
 for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
 env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(wt.TC/'bin')),RECON_PROGRAM_PATH=windows(wt.TC/'bin/C23232.EXE'))
 command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_alignment','-ex','run > inferior.stdout','-ex','print alignment','-ex','set variable alignment = 3','-ex','continue','-ex','x/i $pc-1','-ex','bt 2',str(Path('build/native/C23232.EXE').resolve())]
 phase_start=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);seconds=time.perf_counter()-phase_start
 output=Path('cases/omf-alignment-native-guard.txt');output.write_bytes(r.stdout);assert r.returncode==0 and b'$1 = 4' in r.stdout and b'SIGTRAP' in r.stdout and b'int3' in r.stdout and b'omf_alignment (alignment=3)'in r.stdout,r.stdout
 native_guard=dict(path=str(output),sha256=hashlib.sha256(r.stdout).hexdigest(),seconds=seconds,actual_valid_input=4,injected_unsupported=3,scope='Native-only controlled change of actual alignment formal at entry; unsupported value must SIGTRAP in this function. Not evidence that original frontend produces3 or unsupported original equivalence.')
report=dict(actual_abi=True,rejected=reject,original_entry_eax=raw_entry_eax,defined_mapping_controls=controls,unsupported_original_only=unsupported,native_guard=native_guard,extent=[row['start'],row['end']],seconds=time.perf_counter()-started,caller_contract=dict(direct_calls=['0x6a8d6 SEGDEF','0x6c19e COMDAT initializer'],known_producers=['Segment.alignment default4','debug Segment alignment1','stack_alignment default4']),scope='Full54-byte original helper; defined mappings1/2/4/16 only. Original unsupported RET preserves incoming EAX (source missing return); canonical unsupported result is intentionally unclaimed. Exact native unsupported branch traps, no invented result/init/register bridge. Actual current producer4; public reachability of other values remains unproved and holds if encountered. Caller analysis bounded to known recovered producers, not an exhaustive external-input assertion.')
if args.native_guard:
 Path('cases/omf-alignment-native-guard.json').write_text(json.dumps(native_guard,indent=2)+'\n')
else:
 report['native_guard']=json.loads(Path('cases/omf-alignment-native-guard.json').read_text());assert hashlib.sha256(Path(report['native_guard']['path']).read_bytes()).hexdigest()==report['native_guard']['sha256']
Path('cases/omf-alignment-validation.json').write_text(json.dumps(report,indent=2)+'\n');print('12 defined alignment original/canonical controls,15 unsupported original EAX observations,5 ABI rejects pass; unsupported native trap explicit')
