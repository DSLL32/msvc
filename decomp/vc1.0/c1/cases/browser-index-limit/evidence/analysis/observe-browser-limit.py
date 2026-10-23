from pathlib import Path
import json,subprocess,sys,time,re,hashlib
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');D=S/'validation/browser-limit-return';D.mkdir();sys.path.insert(0,str(B/'scripts'));from case_paths import native_work,windows
C=B/'cases/browser-index-limit';P=json.loads((C/'manifest.json').read_text())['phases']['c1'];work,paths=native_work(D,P['canonical_working_directory'])
for i in P['inputs']:(work/Path(i['path']).name).write_bytes((C/i['path']).read_bytes())
E=dict(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=windows(B.parents[2]/P['target']),MSC_CMD_FLAGS=P['environment']['MSC_CMD_FLAGS'].format(bin=windows(B.parents[2]/'toolchains/1.0-nt/bin'),include=windows(B.parents[2]/'toolchains/1.0-nt/include')));E.update(paths)
cmd='''set pagination off
set confirm off
set disable-randomization off
break *browser_index_overflow
run >%s 2>&1
python
import gdb,json,struct,hashlib
from pathlib import Path
def u(x):return int(gdb.parse_and_eval(x)) & 0xffffffff
inf=gdb.selected_inferior();sp=u('$esp');ret=struct.unpack('<I',bytes(inf.read_memory(sp,4)))[0]
def snapshot():
    row={n:u(n)for n in ['next_symbol_index','browser_symbol_limit','options.option_FR','browser_filename','browser_output']}
    ptr,count,base=struct.unpack('<III',bytes(inf.read_memory(row['browser_output'],12)))
    row['pending']=bytes(inf.read_memory(base,ptr-base)).hex()
    row['registers']={r:u('$'+r)for r in ['ebx','esi','edi','ebp']}
    row['esp']=u('$esp');row['scanner']=u('scanner_flags');row['parser']=u('parser_flags')
    return row
before=snapshot()
Path(%r).write_bytes(Path('registry.sbt').read_bytes())
gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 1
continue
python
row=dict(before=before,after=snapshot(),caller=gdb.newest_frame().name())
a=row['before'];z=row['after']
assert z['esp']-a['esp']==4 and z['registers']==a['registers'] and z['scanner']==a['scanner'] and z['parser']==a['parser']
assert a['options.option_FR']==a['browser_filename']!=0 and z['options.option_FR']==z['browser_filename']==0
assert a['browser_output']==z['browser_output']!=0 and a['next_symbol_index']==z['next_symbol_index']==a['browser_symbol_limit']==z['browser_symbol_limit']==65526
with open(%r,'w')as out:json.dump(row,out,indent=2)
end
'''%(D/'stdout',str(D/'staged-browser-prefix.sbt'),str(D/'registers.json'));(D/'commands.gdb').write_text(cmd);t=time.perf_counter();r=subprocess.run(['/usr/bin/gdb','--nx','--batch','-x',str(D/'commands.gdb'),str(B/'build/native/C13232.EXE')],cwd=work,env=E,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(D/'gdb.log').write_bytes(r.stdout);assert not r.returncode,r.stdout.decode('latin1');row=json.loads((D/'registers.json').read_text());ev=(S/'captures/browser-limit-return-original-buffers/events').read_text();a=[int(x,16)for x in re.search(r'^browser-registers entry (.*)$',ev,re.M)[1].split()];z=[int(x,16)for x in re.search(r'^browser-registers finish (.*)$',ev,re.M)[1].split()];assert z[0]-a[0]==4 and a[1:]==z[1:]
ctx=re.findall(r'^browser-limit (entry|finish) (\d+) (\d+) ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+)$',ev,re.M);assert len(ctx)==2;before,after=ctx;assert before[1:3]==after[1:3]==('65526','65526') and before[3]==before[4] and int(before[3],16) and after[3:5]==('00000000','00000000') and before[5]==after[5] and int(before[5],16)
buffers=re.findall(r'^buffer 0 (\d+) ([0-9a-f]*)$',ev,re.M);assert len(buffers)==2 and row['before']['pending']==buffers[0][1]and row['after']['pending']==buffers[1][1]
raw=(D/'staged-browser-prefix.sbt').read_bytes();original=(S/'captures/browser-limit-pin-enter-1008/work/registry.sbt').read_bytes();assert raw==original
proof=dict(status='pass',seconds=time.perf_counter()-t,native=row,original_registers=[a,z],original_browser_states=ctx,original_pending_buffers=buffers,staged_browser_prefix=dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest(),original_raw_reference='browser-limit-pin-enter-1008/work/registry.sbt'),contract='Actual default65535 limit decreases by nine ordinary builtinmacro browser indices; both original/native real threshold65526 equals nextsymbolindex. Same input filename pointer aliases at entry, both distinct globals clear after target; FILE owner remains identical/open. Exact pending FILE bytes plus full staged browser disk prefix agree. cdeclRET0/ESP+4/nonvolatile/fullflagwords unchanged. No counters or data patched.');(D/'receipt.json').write_text(json.dumps(proof,indent=2)+'\n');print('Actual browser-limit states/FILE/pendingbytes/RET0 PASS')
