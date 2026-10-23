"""Observe first printf's reached internal CRT contracts and public native state."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/dbglist-stdio-support');receipt.mkdir(exist_ok=False)
fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
contracts={'_printf':2,'__stbuf':1,'__isatty':1,'__output':3,'__ftbuf':2,'_write_char':3,'_write_multi_char':4,'_write_string':4,'_get_int_arg':1,'_get_short_arg':1}
with capture_lock():
    scratch=Path(tempfile.mkdtemp(prefix='stdio-support-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
    for p in work.iterdir():assert p.is_file() and not p.is_symlink();p.unlink()
    for row in phase['inputs']:
        data=(fixture/row['path']).read_bytes();assert hashlib.sha256(data).hexdigest()==row['sha256'];(work/Path(row['path']).name).write_bytes(data)
    original,rebuilt,config,records,unused=idx.indexes();runtime=json.loads(Path('runtime.json').read_text());plan=receipt/'entry-plan.txt'
    selected=[next(r for r in runtime if r['name']==name)for name in contracts]
    plan.write_text(''.join('%x %02x %d %s\n'%(r['start']-original.base,original.read(r['start'],1)[0],contracts[r['name']],r['name'])for r in selected))
    image=scratch/'C23232.EXE';image.write_bytes(original.data)
    env=idx.wt.environment();env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan))
    command=['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+name))for name in ['stdout','stderr','events']],windows(work)]
    result=subprocess.run(command,env=env,capture_output=True,timeout=60)
    assert result.returncode==0,(result.returncode,result.stdout,result.stderr)
original_seconds=time.perf_counter()-started
scratch=Path(tempfile.mkdtemp(prefix='stdio-support-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
snapshot="import gdb,json; s=gdb.parse_and_eval('recon_iob[1]'); used=int(s['_ptr'])-int(s['_base']) if int(s['_base']) else 0; fd=int(gdb.parse_and_eval('recon_osfhnd[1]'))-1; info=open('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,fd)).read(); position=int(next(x.split()[1]for x in info.splitlines()if x.startswith('pos:'))); print(PREFIX+json.dumps(dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr']) and not int(s['_base'])),used=used,position=position,buffer_hex=bytes(gdb.selected_inferior().read_memory(int(s['_base']),used)).hex() if used else '')))"
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break parse_options','-ex','run > native-inferior.stdout','-ex','break recon_printf','-ex','continue',
 '-ex','python '+snapshot.replace('PREFIX',repr('C2_STDIO_BEFORE ')),'-ex','finish','-ex','printf "C2_STDIO_RESULT %d\\n",$eax','-ex','python '+snapshot.replace('PREFIX',repr('C2_STDIO_AFTER ')),str(Path('build/native/C23232.EXE').resolve())]
phase_started=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-phase_started
(receipt/'native.txt').write_bytes(result.stdout);log=result.stdout.decode(errors='replace');assert result.returncode==0,log
text=(receipt/'original.txt').read_text();blocks=text.splitlines();observations=[]
for i in range(0,len(blocks),3):
    header=blocks[i].split();state=blocks[i+1].split();fields={state[n]:int(state[n+1],16 if state[n]in ['cursor','value'] else 10)for n in range(1,len(state),2)}
    fields['buffer_hex']=blocks[i+2].removeprefix('buffer_hex ')
    observations.append(dict(stage=header[0],name=header[1],eax=int(header[3],16),args=[int(x,16)for x in header[5:]],file=fields))
public=[r for r in observations if r['name']=='_printf'];assert [r['stage']for r in public]==['enter','return']
left=[{k:v for k,v in r['file'].items()if k not in ['counter','cursor','value']}for r in public]
right=[json.loads(re.search('C2_STDIO_'+stage+' (.*)',log)[1])for stage in ['BEFORE','AFTER']]
assert left==right,(left,right)
assert public[-1]['eax']==int(re.search(r'C2_STDIO_RESULT (-?\d+)',log)[1])==24
assert bytes.fromhex(right[-1]['buffer_hex'])==b'\tTITLE\tdbglist.c\n\t.386P\n'
seen={r['name']for r in observations};assert {'_printf','__stbuf','__isatty','__output','__ftbuf','_write_char','_write_string','_get_int_arg'}<=seen,seen
(receipt/'validation.json').write_text(json.dumps(dict(observations=observations,public_original_file=left,public_native_file=right,public_result=24,matched_public_seam=True,
 boundary='Internal original CRT ABIs are observed, not independently reconstructed or matched to fabricated native entries. Native public printf FILE/buffer/descriptor state+bytes and return are authoritative alongside complete-case artifact oracle.',
 original_observer_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,plan_sha256=hashlib.sha256(plan.read_bytes()).hexdigest(),executable_sha256=hashlib.sha256(Path('build/native/C23232.EXE').read_bytes()).hexdigest()),indent=2)+'\n')
print(sorted(seen),left)
