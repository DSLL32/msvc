"""Native-only actual EAX/ECX transport through automatic trace hooks."""
import hashlib,json,os,shutil,subprocess,tempfile,time
from pathlib import Path
from case_paths import C2_SCRATCH,native_work,windows
import pe_indexes as idx
started=time.perf_counter();out=Path('cases/comdat-alignment8-native');out.mkdir(exist_ok=True)
fixture=Path('../fixtures/c2-omf-comdat-communal');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2'];scratch=Path(tempfile.mkdtemp(prefix='alignment-eax-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
source=Path('src/omf-output.c').read_text().splitlines();line=next(i+1 for i,s in enumerate(source)if s=='    switch (alignment) {')
gdb_source='''import gdb,json
rows=[]
def reg(n):return int(gdb.parse_and_eval('$'+n))&0xffffffff
class Entry(gdb.Breakpoint):
 def stop(self):
  caller=gdb.newest_frame().older();caller.select();name=caller.name();producer=int(gdb.parse_and_eval('record' if name=='omf_write_segment_definition' else 'value'))&0xffffffff;record=gdb.parse_and_eval('record');count=int(record['count']);cursor_offset=int(record['cursor'])-int(record);gdb.newest_frame().select()
  row=dict(eax=reg('eax'),ecx=reg('ecx'),esp=reg('esp'),caller=name,producer=producer,count=count,cursor_offset=cursor_offset,nonvol={r:reg(r)for r in ['ebx','esi','edi','ebp']});rows.append(row);return False
class Returned(gdb.FinishBreakpoint):
 def __init__(self,row):super().__init__(internal=True);self.row=row
 def stop(self):
  self.row.update(return_eax=reg('eax'),return_esp=reg('esp'),return_nonvol={r:reg(r)for r in ['ebx','esi','edi','ebp']});return False
class Body(gdb.Breakpoint):
 def stop(self):
  row=rows[-1];row.update(incoming_eax=int(gdb.parse_and_eval('incoming_eax'))&0xffffffff,alignment=int(gdb.parse_and_eval('alignment'))&0xffffffff,pool=int(gdb.parse_and_eval('&omf_record_storage.records[0]')),slots=[int(gdb.parse_and_eval('&omf_record_storage.records[%d]'%i))for i in range(4)]);Returned(row);return False
Entry('*omf_alignment',internal=True)
Body('src/omf-output.c:LINE',internal=True)
gdb.execute('run > inferior.stdout')
print('C2_ALIGNMENT_ROWS '+json.dumps(rows))
'''.replace('LINE',str(line))
(out/'observe.gdb.py').write_text(gdb_source);exe=Path('build/native-trace/C23232.EXE').resolve();t=time.perf_counter();r=subprocess.run(['gdb','-q','--batch','-ex','set pagination off','-ex','source '+str((out/'observe.gdb.py').resolve()),str(exe)],cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);seconds=time.perf_counter()-t;(out/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout
raw=next(s.split('C2_ALIGNMENT_ROWS ',1)[1]for s in r.stdout.decode().splitlines()if s.startswith('C2_ALIGNMENT_ROWS '));rows=json.loads(raw);assert rows
for row in rows:
 assert row['incoming_eax']==row['eax']==row['producer']and row['alignment']==row['ecx']and row['return_esp']==row['esp']+4 and row['return_nonvol']==row['nonvol'],row
 assert row['return_eax']=={1:1,2:2,4:5,16:3}.get(row['alignment'],row['incoming_eax']),row
 assert row['slots']==[row['pool']+i*1004 for i in range(4)]and row['pool']%8==4
assert {row['caller']for row in rows}=={'omf_write_segment_definition','omf_initialize_record'}
assert any(row['alignment']==8 and row['caller']=='omf_write_segment_definition'for row in rows)
expected=next(v for v in phase['outputs']if v['path'].endswith('packets.obj'));obj=(work/'packets.obj').read_bytes();assert hashlib.sha256(obj).hexdigest()==expected['sha256']
report=dict(matched=True,rows=rows,native_execution_seconds=seconds,total_seconds=time.perf_counter()-started,executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),manifest_sha256=hashlib.sha256((fixture/'manifest.json').read_bytes()).hexdigest(),source_sha256=hashlib.sha256(Path('src/omf-output.c').read_bytes()).hexdigest(),gdb_sha256=hashlib.sha256(r.stdout).hexdigest(),object_sha256=hashlib.sha256(obj).hexdigest(),scope='Raw function entry EAX/ECX and actual caller record/full selection producer, then native regcall C formals after automatic entry hook and full EAX/ESP/nonvol at return. Both actual callers, traced ELF; exact complete1962B object. Breakpoints only; no register/value injection.')
(out/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print(len(rows),'actual traced native alignment calls preserve EAX/ECX/producers/RET0; object exact')
