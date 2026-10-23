"""Observe the corrected debug filename callback before accepting its body."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--native-only',action='store_true');args=parser.parse_args()
started=time.perf_counter();receipt=Path('cases/dbglist-coff-pair-abi');receipt.mkdir(exist_ok=args.native_only);fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
legacy='_coff_output_debug_pair';typed='@coff_output_debug_pair@4'
original_seconds=None
if args.native_only:
    binding=json.loads((receipt/'original-binding.json').read_text())
    assert binding['target_sha256']==sha(idx.wt.TC/'bin/C23232.EXE')and binding['manifest_sha256']==sha(fixture/'manifest.json')
    assert binding['observer_source_sha256']==sha('src/reference-stdio-observer.c')and binding['observer_sha256']==sha('build/reference-stdio-observer.exe')
    assert all(sha(receipt/name)==value for name,value in binding['files'].items())
    original,rebuilt,config,records,unused=idx.indexes()
    before=json.loads(Path('cases/dbglist-original-1229.json').read_text())
    assert binding['config_sha256']==before['config_sha256'] and before['accepted']==1229
    assert before['logical']==legacy and before['original_va']=='0x6d3b0'
    row=next(r for r in config['driver']if r['name']==typed)
    assert row['start']==0x6d3b0 and row['end']==0x6d3ec and config['accepted'][1229]==typed and len(config['accepted'])==1230
else:
    with capture_lock():
        scratch=Path(tempfile.mkdtemp(prefix='coff-filename-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
        for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
        for row in phase['inputs']:
            source=fixture/row['path'];assert sha(source)==row['sha256'];shutil.copyfile(source,work/source.name)
        original,rebuilt,config,records,unused=idx.indexes();row=next(r for r in config['driver']if r['name']==legacy);assert row['start']==0x6d3b0 and row['provisional']
        assert typed in idx.publics()and legacy not in idx.publics(),'typed stub build required before formal observation'
        plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 257 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],legacy));image=scratch/'C23232.EXE';image.write_bytes(original.data)
        env=idx.wt.environment()
        for key in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(key,None)
        env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=legacy)
        command=['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+key))for key in ['stdout','stderr','events']],windows(work)]
        begin=time.perf_counter();result=subprocess.run(command,env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-begin
        (receipt/'observer.stdout').write_bytes(result.stdout);(receipt/'observer.stderr').write_bytes(result.stderr);assert result.returncode==0,(result.stdout,result.stderr)
scratch=Path(tempfile.mkdtemp(prefix='coff-filename-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
snapshot="""import gdb,json
p=int(gdb.parse_and_eval('filename')); data=bytearray()
for i in range(1024):
    byte=bytes(gdb.selected_inferior().read_memory(p+i,1))[0]; data.append(byte)
    if not byte:break
else:raise AssertionError('unbounded filename')
s=gdb.parse_and_eval('recon_iob[1]'); used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0
print('C2_FILENAME '+json.dumps(dict(pointer=p,filename_hex=data.hex(),column=int(gdb.parse_and_eval('listing_column')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,coff=dict(emitted=int(gdb.parse_and_eval('coff_emitted_function')),codeview=int(gdb.parse_and_eval('codeview_debug')),line=int(gdb.parse_and_eval('line_debug')),pending=int(gdb.parse_and_eval('coff_has_function_lines')),source=int(gdb.parse_and_eval('current_source_filename')),listing_source=int(gdb.parse_and_eval('listing_source_filename'))),stdout=dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,buffer_hex=bytes(gdb.selected_inferior().read_memory(int(s['_base']),used)).hex()if used else ''))))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break coff_output_debug_pair','-ex','run > native-inferior.stdout','-ex','python '+snapshot,str(Path('build/native/C23232.EXE').resolve())]
begin=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-begin
(receipt/'native.txt').write_bytes(result.stdout);log=result.stdout.decode(errors='replace');assert result.returncode==0,log;native=json.loads(re.search('C2_FILENAME (.*)',log)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[0].split()[:2]==['enter',legacy]
arguments=[int(v,16)for v in lines[0].split()[5:]];assert len(arguments)==1
parts=lines[1].split();stdout={parts[n]:int(parts[n+1],16 if parts[n]in['cursor','value']else 10)for n in range(1,len(parts),2)};stdout['buffer_hex']=lines[2].removeprefix('buffer_hex ')
parts=lines[3].split();state={parts[n]:int(parts[n+1])for n in range(1,len(parts),2)};filename=lines[4].removeprefix('filename_hex ')
parts=lines[5].split();coff={parts[n]:int(parts[n+1],16 if parts[n]in ['source','listing_source']else 10)for n in range(1,len(parts),2)};assert lines[6]=='entry_end'
assert filename==native['filename_hex']==b'dbglist.c\0'.hex()
assert arguments[0]==coff['source']==coff['listing_source']and native['pointer']==native['coff']['source']==native['coff']['listing_source']
assert {k:coff[k]for k in ['emitted','codeview','line','pending']}=={k:native['coff'][k]for k in ['emitted','codeview','line','pending']}
assert {k:native[k]for k in ['column','cw','sw']}=={k:state[k]for k in ['column','cw','sw']}
assert native['stdout']=={k:v for k,v in stdout.items()if k not in ['counter','cursor','value','position']}
r=dict(matched=True,function=typed,observed_legacy_logical=legacy,original_va='0x6d3b0',formals=[dict(name='filename',location='ECX',type='input char pointer',original=arguments[0],native=native['pointer'],defined_input_hex=filename)],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Whole original6d3b0..6d3ec savesESI then consumes ECX filename; EAX/EDX/flags not read as entry arguments, no stack args, RET0. Four named guard/line globals are observed.',original_state=state,original_coff=coff,original_stdout=stdout,native=native,identity_proof='Argument equals current_source_filename7901c and listing_source_filename7e8a0 on original; same two named native aliases equal actual formal. Machine-emission6855b..68567 binds the original stores and ECX callback call.',decoration_correction='Legacy naked(void) placeholder is not an ABI proof. Typed fastcall(char*) trapping stub and ordinary callback assignment compiled before native observation. C identifier and original VA unchanged; old raw receipts keep legacy name. Full body remains unaccepted at this observation.',target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=(binding['config_sha256']if args.native_only else sha('functions.json')),native_config_sha256=sha('functions.json'),interpretation_correction='Raw original source label reads7901c: existing current_source_filename, distinct from primary805ec and generated85db4. Original bytes unchanged.',observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),native_executable_sha256=sha('build/native/C23232.EXE'),canonical_executable_sha256=sha('build/c2.exe'),original_execution_seconds=original_seconds,reused_immutable_original=args.native_only,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual filename formal, NUL-terminated bytes, source/listing pointer aliases, all guard/line globals and known stdout/column/x87 state. No injected calls, callback body or callee behavior acceptance.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Filename dbglist.c, both pointer aliases and COFF/stdio entry state match',state,{k:v for k,v in coff.items()if k not in ['source','listing_source']})
