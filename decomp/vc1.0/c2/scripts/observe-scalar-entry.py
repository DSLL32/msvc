"""Explicit read-only fastcall scalar-entry observations, outside fixtures."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--native-only',action='store_true');parser.add_argument('--profile',default='dbglist');parser.add_argument('--function',required=True);parser.add_argument('--parameter',action='append',default=[]);parser.add_argument('--no-hidden-inputs-proof',required=True);parser.add_argument('--receipt',required=True,type=Path);parser.add_argument('--stack-cleanup',type=int,default=0);args=parser.parse_args()
parameters=[v.split(':')for v in args.parameter];assert 0<=len(parameters)<=8 and all(len(v)==2 and v[1]in ['signed32','unsigned32','signed16','unsigned16']for v in parameters)
assert args.stack_cleanup==max(len(parameters)-2,0)*4
started=time.perf_counter();receipt=args.receipt;receipt.mkdir(exist_ok=args.native_only);config=json.loads(Path('functions.json').read_text());fixture=Path(config['cases'][args.profile]['fixture_manifest']).parent;phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2'];name=args.function
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
original_seconds=None
if args.native_only:
    binding=json.loads((receipt/'original-binding.json').read_text())
    for key,path in [('target_sha256',idx.wt.TC/'bin/C23232.EXE'),('manifest_sha256',fixture/'manifest.json'),('config_sha256','functions.json'),('observer_source_sha256','src/reference-stdio-observer.c'),('observer_sha256','build/reference-stdio-observer.exe')]:
        assert binding[key]==sha(path),(key,'immutable original binding changed')
    assert all(sha(receipt/file)==value for file,value in binding['files'].items())
else:
    with capture_lock():
        scratch=Path(tempfile.mkdtemp(prefix='scalar-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/phase['canonical_working_directory'].rsplit('\\',1)[1]
        for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
        for row in phase['inputs']:
            source=fixture/row['path'];assert sha(source)==row['sha256'];shutil.copyfile(source,work/source.name)
        original,rebuilt,config,records,unused=idx.indexes();row=next(r for r in config['driver']if r['name']==name)
        plan=receipt/'entry-plan.txt';plan.write_text('%x %02x %d %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],0x100|len(parameters),name));image=scratch/'C23232.EXE';image.write_bytes(original.data)
        env=idx.wt.environment()
        for key in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(key,None)
        env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name)
        command=['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+key))for key in ['stdout','stderr','events']],windows(work)]
        begin=time.perf_counter();result=subprocess.run(command,env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-begin
        (receipt/'observer.stdout').write_bytes(result.stdout);(receipt/'observer.stderr').write_bytes(result.stderr);assert result.returncode==0,(result.stdout,result.stderr)
if not args.native_only:
    binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']})
    (receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='scalar-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
snapshot="import gdb,json; s=gdb.parse_and_eval('recon_iob[1]'); used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0; values=[int(gdb.parse_and_eval(p))for p in PARAMETERS]; print('C2_SCALARS '+json.dumps(dict(values=values,coff_object=int(gdb.parse_and_eval('coff_object')),column=int(gdb.parse_and_eval('listing_column')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,stdout=dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,buffer_hex=bytes(gdb.selected_inferior().read_memory(int(s['_base']),used)).hex()if used else ''))))"
snapshot=snapshot.replace('PARAMETERS',repr([p[0]for p in parameters]));command=['gdb','-q','--batch','-ex','set pagination off','-ex','break '+re.sub(r'@\d+$','',name[1:]),'-ex','run > native-inferior.stdout','-ex','python '+snapshot,str(Path('build/native/C23232.EXE').resolve())]
begin=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-begin
(receipt/'native.txt').write_bytes(result.stdout);log=result.stdout.decode(errors='replace');assert result.returncode==0,log;native=json.loads(re.search('C2_SCALARS (.*)',log)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[0].split()[:2]==['enter',name]
values=[int(v,16)for v in lines[0].split()[5:]];assert len(values)==len(parameters);raw_values=values[:]
for index,(parameter,kind)in enumerate(parameters):
    bits=16 if kind.endswith('16')else 32;values[index]&=(1<<bits)-1
    if kind.startswith('signed')and values[index]>=(1<<(bits-1)):values[index]-=1<<bits
assert values==native['values'],(values,native['values'])
parts=lines[1].split();stdout={parts[n]:int(parts[n+1],16 if parts[n]in['cursor','value']else 10)for n in range(1,len(parts),2)};stdout['buffer_hex']=lines[2].removeprefix('buffer_hex ')
parts=lines[3].split();state={parts[n]:int(parts[n+1])for n in range(1,len(parts),2)};assert lines[-1]=='entry_end'
omf_state=None
if name=='_omf_open_module':
    assert len(lines)==6 and lines[4]=='omf_entry coff_object 0'and native['coff_object']==0;omf_state=dict(coff_object=0)
else:assert len(lines)==5
assert native['stdout']=={k:v for k,v in stdout.items()if k not in ['counter','cursor','value','position']}
assert {k:native[k]for k in ['column','cw','sw']}=={k:state[k]for k in ['column','cw','sw']}
r=dict(matched=True,function=name,profile=args.profile,omf_state=omf_state,formals=[dict(name=p,location=('ECX'if i==0 else'EDX'if i==1 else'entryESP+%d'%(4*(i-1))),type=t,value=values[i])for i,(p,t)in enumerate(parameters)],hidden_inputs=[],no_hidden_inputs_proof=args.no_hidden_inputs_proof,callee_stack_cleanup=args.stack_cleanup,original_raw_values=raw_values,original_values=values,original_state=state,native=native,original_stdout=stdout,target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),native_executable_sha256=sha('build/native/C23232.EXE'),original_execution_seconds=original_seconds,reused_immutable_original=args.native_only,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual scalar fastcall register/stack formals (16-bit parameters compare their proved lower-word contract; full original register/slot words retained) and known stdout/column/x87 entry state. No injected calls, return observation, child behavior, arbitrary-input equivalence or completed artifact claim.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual',name,'scalar formals match',values)
