"""Observe no-formal fixup writer count and typed pool identity, no field dump."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
ap=argparse.ArgumentParser();ap.add_argument('--receipt',type=Path,default=Path('cases/omf-module-end-abi'));args=ap.parse_args();started=time.perf_counter();receipt=args.receipt;receipt.mkdir();name='_omf_finish_module';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='module-end-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x6a9d0;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 0 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='module-end-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json,os
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n));s=gdb.parse_and_eval('*object_file');fd=int(s['_file']);host=int(gdb.parse_and_eval('recon_osfhnd[%d]'%fd))-1;path='/proc/%d/fd/%d'%(gdb.selected_inferior().pid,host);assert os.readlink(path).endswith('/omftypes.obj');info=open('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,host)).read();pos=int(next(l.split()[1]for l in info.splitlines()if l.startswith('pos:')));used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0;f=dict(stream=int(gdb.parse_and_eval('object_file')),flags=int(s['_flag']),count=int(s['_cnt']),fd=fd,size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,position=pos,osfile=int(gdb.parse_and_eval('recon_osfile[%d]'%fd)),file_bytes=os.stat(path).st_size,buffer_hex=read(int(s['_base']),used).hex()if used else'');segments=[];node=gdb.parse_and_eval('segment_list');index=0
while int(node)and index<256:
 d=dict(index=index,pointer=int(node),next=int(node['segment_next']),combine=int(node['combine']))
 if d['combine']!=7:d.update(header_end=int(node['omf_header_end']),order=int(node['identity']['order']))
 segments.append(d);node=node['segment_next'];index+=1
assert not int(node)
print('C2_MODULE_END '+json.dumps(dict(file=f,segments=segments,caller=gdb.selected_frame().older().name())))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_finish_module','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_MODULE_END (.*)',r.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name]and len(lines[0].split())==5
parts=next(v for v in lines if v.startswith('omf_finish_file ')).split()[1:];f={parts[n]:int(parts[n+1],16 if parts[n]in['stream','return_address']else 10)for n in range(0,len(parts),2)};f['buffer_hex']=next(v.removeprefix('omf_finish_buffer_hex ')for v in lines if v.startswith('omf_finish_buffer_hex '));segments=[]
for v in lines:
 if v.startswith('omf_finish_segment '):
  p=v.split()[1:];segments.append({p[n]:int(p[n+1],16 if p[n]in['pointer','next']else 10)for n in range(0,len(p),2)})
observed=dict(file=f,segments=segments)
def shape(d):
 f=d['file'];assert f['stream']and len(bytes.fromhex(f['buffer_hex']))==f['used'];nodes=d['segments']
 assert all(n['index']==i and n['pointer']and n['next']==(nodes[i+1]['pointer']if i+1<len(nodes)else 0)for i,n in enumerate(nodes))
 return dict(file={k:v for k,v in f.items()if k not in['stream','return_address']},segments=[{k:v for k,v in n.items()if k not in['pointer','next']}for n in nodes])
assert shape(observed)==shape(native)
r=dict(matched=True,function=name,original_va='0x6a9d0',formals=[],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original6a9d0..6aacd no formal/register/stack/flags/x87/out pointer input, RET0. Consumes object_file global84fb0 via public ftell/fseek/fclose, segment_list84fac next36/combine53/conditional header_end60/order64 after initial flush children; current globals reloaded at original boundaries. Actual FILE owner/buffer topology/defined used prefix/lowio/descriptor position+length and closed ordered segment fields observed. object_generation_started is write-only final output and not captured as input. Child cache/FILE/segment state can change; entry snapshot never substitutes later reloads.',shape=shape(observed),original=observed,native=native,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual no-formal ABI/contextual FILE+defined prefix and directly consumed list fields only; combine7 header/order inactive and not read, no opaque Segment/FILE raw snapshot/uninitialized tails/output seed. Original read-only DuplicateHandle position/size and native procfs observations; no injected target call.256 list bound observer-only. Whole body/control/canonical/fresh matching pending.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual module finalizer inputs match',len(segments),r['shape']['file'])
