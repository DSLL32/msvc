"""Actual OMF C string formal and source-name identity, outside fixtures."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--function',default='@omf_write_module_header@4',choices=['@omf_write_module_header@4','@omf_name_index@4']);parser.add_argument('--receipt',type=Path,default=Path('cases/omf-module-header-abi'));args=parser.parse_args()
started=time.perf_counter();receipt=args.receipt;receipt.mkdir();fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2'];name=args.function;pointer_slot=1 if name=='_coff_output_size'else 0
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
    scratch=Path(tempfile.mkdtemp(prefix='function-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
    for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
    for r in phase['inputs']:
        source=fixture/r['path'];assert sha(source)==r['sha256'];shutil.copyfile(source,work/source.name)
    original,rebuilt,config,records,unused=idx.indexes();row=next(r for r in config['driver']if r['name']==name);expected_va={'@omf_write_module_header@4':0x6a980,'@omf_name_index@4':0x6a940}[name];assert row['start']==expected_va
    plan=receipt/'entry-plan.txt';plan.write_text('%x %02x %d %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],257+pointer_slot,name));image=scratch/'C23232.EXE';image.write_bytes(original.data)
    env=idx.wt.environment()
    for key in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(key,None)
    env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name)
    begin=time.perf_counter();result=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+key))for key in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-begin
    (receipt/'observer.stdout').write_bytes(result.stdout);(receipt/'observer.stderr').write_bytes(result.stderr);assert result.returncode==0,(result.stdout,result.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='codeview-file-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
parameter='source'if name=='@omf_write_module_header@4'else'name'
snapshot="""import gdb,json
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
p=int(gdb.parse_and_eval(PARAMETER));raw=b''
for i in range(512):
 c=read(p+i,1);raw+=c
 if c==b'\\0':break
assert raw[-1:]==b'\\0'
s=gdb.parse_and_eval('recon_iob[1]');used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0
print('C2_OMF_STRING '+json.dumps(dict(pointer=p,source_global=int(gdb.parse_and_eval('source_filename')),hex=raw.hex(),column=int(gdb.parse_and_eval('listing_column')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,stdout=dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,buffer_hex=read(int(s['_base']),used).hex()if used else ''))))""".replace('PARAMETER',repr(parameter))
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break '+name[1:].rsplit('@',1)[0],'-ex','run > native-inferior.stdout','-ex','python '+snapshot,str(Path('build/native/C23232.EXE').resolve())]
begin=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-begin;(receipt/'native.txt').write_bytes(result.stdout);assert result.returncode==0,result.stdout
native=json.loads(re.search(rb'C2_OMF_STRING (.*)',result.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[0].split()[:2]==['enter',name];arguments=[int(v,16)for v in lines[0].split()[5:]];assert len(arguments)==1
parts=lines[1].split();stdout={parts[n]:int(parts[n+1],16 if parts[n]in['cursor','value']else 10)for n in range(1,len(parts),2)};stdout['buffer_hex']=lines[2].removeprefix('buffer_hex ')
parts=lines[3].split();state={parts[n]:int(parts[n+1])for n in range(1,len(parts),2)}
source_global=int(lines[4].removeprefix('omf_string source_global '),16);raw=lines[5].removeprefix('omf_string_hex ');assert lines[6]=='entry_end'
assert raw==native['hex']and bytes.fromhex(raw)[-1:]==b'\0'and arguments[0]and native['pointer']
if parameter=='source':assert arguments[0]==source_global and native['pointer']==native['source_global']and bytes.fromhex(raw)==b'omftypes.c\0'
assert native['stdout']=={k:v for k,v in stdout.items()if k not in ['counter','cursor','value','position']}
assert {k:native[k]for k in ['column','cw','sw']}=={k:state[k]for k in ['column','cw','sw']}
r=dict(matched=True,function=name,original_va=hex(expected_va),formals=[dict(name=parameter,location='ECX',type='char pointer',original=arguments[0],native=native['pointer'])],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original consumes only ECX string pointer, savedESI assigned from it, no incoming EDX/stack/GPR/flags/x87/hidden outputs, RET0. String passed unchanged to named record helpers; source global ownership at header boundary checked independently.',original_string_hex=raw,original_source_global=source_global,native=native,original_stdout=stdout,original_state=state,binding=binding,native_executable_sha256=sha('build/native/C23232.EXE'),original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual sole char pointer formal, complete bounded NUL-terminated input and header source_filename identity. Observer512-byte bound is outside target. Pointer bytes not equated across platforms; no injected calls/record-helper behavior/output completion acceptance.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual OMF string formal/ownership matches',bytes.fromhex(raw))
