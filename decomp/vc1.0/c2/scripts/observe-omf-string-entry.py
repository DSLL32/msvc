"""Actual OMF C string formal and source-name identity, outside fixtures."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--function',default='@omf_write_module_header@4',choices=['@omf_write_module_header@4','@omf_name_index@4','@omf_append_string@8','@omf_append_bytes@12']);parser.add_argument('--receipt',type=Path,default=Path('cases/omf-module-header-abi'));args=parser.parse_args()
started=time.perf_counter();receipt=args.receipt;receipt.mkdir();fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2'];name=args.function;is_bytes=name=='@omf_append_bytes@12';pointer_slot=int(is_bytes or name=='@omf_append_string@8')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
    scratch=Path(tempfile.mkdtemp(prefix='function-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
    for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
    for r in phase['inputs']:
        source=fixture/r['path'];assert sha(source)==r['sha256'];shutil.copyfile(source,work/source.name)
    original,rebuilt,config,records,unused=idx.indexes();row=next(r for r in config['driver']if r['name']==name);expected_va={'@omf_write_module_header@4':0x6a980,'@omf_name_index@4':0x6a940,'@omf_append_string@8':0x6c4e0,'@omf_append_bytes@12':0x6b650}[name];assert row['start']==expected_va
    plan=receipt/'entry-plan.txt';plan.write_text('%x %02x %d %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],257+pointer_slot+int(is_bytes),name));image=scratch/'C23232.EXE';image.write_bytes(original.data)
    env=idx.wt.environment()
    for key in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(key,None)
    env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name)
    begin=time.perf_counter();result=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+key))for key in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-begin
    (receipt/'observer.stdout').write_bytes(result.stdout);(receipt/'observer.stderr').write_bytes(result.stderr);assert result.returncode==0,(result.stdout,result.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='codeview-file-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
parameter='source'if name=='@omf_write_module_header@4'else'bytes'if is_bytes else'text'if pointer_slot else'name'
snapshot="""import gdb,json
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
p=int(gdb.parse_and_eval(PARAMETER));raw=b''
for i in range(512):
 c=read(p+i,1);raw+=c
 if c==b'\\0':break
assert raw[-1:]==b'\\0'
record=None
if PARAMETER in ['text','bytes']:
 rp=int(gdb.parse_and_eval('record'));record=dict(pointer=rp,current=int(gdb.parse_and_eval('omf_current_record')),base=int(gdb.parse_and_eval('&omf_records[0]')),hex=read(rp,PREFIX_SIZE).hex())
s=gdb.parse_and_eval('recon_iob[1]');used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0
print('C2_OMF_STRING '+json.dumps(dict(record=record,byte_count=int(gdb.parse_and_eval('count'))if PARAMETER=='bytes'else None,pointer=p,source_global=int(gdb.parse_and_eval('source_filename')),hex=raw.hex(),column=int(gdb.parse_and_eval('listing_column')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,stdout=dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,buffer_hex=read(int(s['_base']),used).hex()if used else ''))))""".replace('PARAMETER',repr(parameter)).replace('PREFIX_SIZE',str(24 if is_bytes else 23))
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break '+name[1:].rsplit('@',1)[0],'-ex','run > native-inferior.stdout','-ex','python '+snapshot,str(Path('build/native/C23232.EXE').resolve())]
begin=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-begin;(receipt/'native.txt').write_bytes(result.stdout);assert result.returncode==0,result.stdout
native=json.loads(re.search(rb'C2_OMF_STRING (.*)',result.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[0].split()[:2]==['enter',name];arguments=[int(v,16)for v in lines[0].split()[5:]];assert len(arguments)==1+pointer_slot+int(is_bytes)
parts=lines[1].split();stdout={parts[n]:int(parts[n+1],16 if parts[n]in['cursor','value']else 10)for n in range(1,len(parts),2)};stdout['buffer_hex']=lines[2].removeprefix('buffer_hex ')
parts=lines[3].split();state={parts[n]:int(parts[n+1])for n in range(1,len(parts),2)}
source_global=int(lines[4].removeprefix('omf_string source_global '),16);raw=lines[5].removeprefix('omf_string_hex ');assert lines[-1]=='entry_end'
assert raw==native['hex']and bytes.fromhex(raw)[-1:]==b'\0'and arguments[pointer_slot]and native['pointer']
if parameter in ['source','text','bytes']:assert arguments[pointer_slot]==source_global and native['pointer']==native['source_global']and bytes.fromhex(raw)==b'omftypes.c\0'
record=None
if pointer_slot:
 assert len(lines)==9
 parts=lines[6].split();record={parts[i]:int(parts[i+1],16)for i in range(1,len(parts),2)};record['hex']=lines[7].removeprefix('omf_record_hex ')
 for observed in [record,native['record']]:
  assert observed['pointer']==observed['base']==observed['current']
  header=struct.unpack('<5I',bytes.fromhex(observed['hex'])[:20]);assert header==(128,0,0,int(is_bytes),observed['pointer']+(24 if is_bytes else 23))and bytes.fromhex(observed['hex'])[20:]==b'\x80\0\0'+(b'\x0a'if is_bytes else b'')
 assert arguments[0]==record['pointer']
 if is_bytes:assert arguments[2]==native['byte_count']==10==len(bytes.fromhex(raw))-1
else:assert len(lines)==7
assert native['stdout']=={k:v for k,v in stdout.items()if k not in ['counter','cursor','value','position']}
assert {k:native[k]for k in ['column','cw','sw']}=={k:state[k]for k in ['column','cw','sw']}
r=dict(matched=True,function=name,original_va=hex(expected_va),formals=([dict(name='record',location='ECX',type='OmfRecord pointer',original=arguments[0],native=native['record']['pointer'])]if pointer_slot else[])+[dict(name=parameter,location='EDX'if pointer_slot else'ECX',type='char pointer',original=arguments[pointer_slot],native=native['pointer'])],record_input=record,hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof=('Original6c4e0..6c540 consumes ECX record and EDX mutable char pointer, RET0; no extra incoming register/stack/x87/flags or hidden inputs. Real record slot0/current/base with defined20-byte header and3-byte prefix, source_filename pointer identity and full NUL bytes observed. 'if pointer_slot else'')+'Original consumes only ECX string pointer, savedESI assigned from it, no incoming EDX/stack/GPR/flags/x87/hidden outputs, RET0. String passed unchanged to named record helpers; source global ownership at header boundary checked independently.',original_string_hex=raw,original_source_global=source_global,native=native,original_stdout=stdout,original_state=state,binding=binding,native_executable_sha256=sha('build/native/C23232.EXE'),original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual sole char pointer formal, complete bounded NUL-terminated input and header source_filename identity. Observer512-byte bound is outside target. Pointer bytes not equated across platforms; no injected calls/record-helper behavior/output completion acceptance.')
if is_bytes:
 r['formals'].append(dict(name='count',location='entryESP+4',type='unsigned32',original=arguments[2],native=native['byte_count']))
 r['callee_stack_cleanup']=4
 r['no_hidden_inputs_proof']='Original6b650..6b69b consumes ECX record, EDX source bytes and entryESP+4 unsigned count, RET4; no other incoming register/stack/flags/x87/hidden inputs. Actual slot0/current/base/header/cursor24, count10 and source_filename owner/full11-byte NUL string independently observed; first10 bytes consumed, no incomplete child ABI claim.'
elif pointer_slot:
 r['no_hidden_inputs_proof']='Original6c4e0..6c540 consumes ECX record and EDX mutable char pointer, RET0; no extra incoming register/stack/x87/flags or hidden inputs. Actual record slot0/current/base aliases,20-byte header and3-byte prefix plus source_filename ownership/full NUL bytes independently observed; no completed output/long-string execution claim.'
 r['scope']=('Actual three-formal record/string/count inputs'if is_bytes else'Actual two-formal record/string inputs')+', complete NUL bytes and source/record/cursor alias identities; no injected calls, child behavior or cold-path execution claim.'
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual OMF string formal/ownership matches',bytes.fromhex(raw))
