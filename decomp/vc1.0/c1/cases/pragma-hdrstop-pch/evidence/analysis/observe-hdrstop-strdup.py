from pathlib import Path
import json,subprocess,sys,time,re
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');D=S/'validation/hdrstop-strdup-return-1000';D.mkdir();sys.path.insert(0,str(B/'scripts'));from case_paths import native_work,windows
C=B/'cases/pragma-hdrstop-pch';P=json.loads((C/'manifest.json').read_text())['phases']['c1-create'];work,paths=native_work(D,P['canonical_working_directory'])
for i in P['inputs']:
 p=work/Path(i['path']).name;p.write_bytes((C/i['path']).read_bytes());import os;os.utime(p,ns=(i['mtime_ns'],i['mtime_ns']))
E=dict(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=windows(B.parents[2]/P['target']),MSC_CMD_FLAGS=P['environment']['MSC_CMD_FLAGS'].format(bin=windows(B.parents[2]/'toolchains/1.0-nt/bin'),include=windows(B.parents[2]/'toolchains/1.0-nt/include')));E.update(paths)
cmd='''set pagination off
set confirm off
set disable-randomization off
break synchronize_pch_pragma
run >%s 2>&1
delete 1
break *strdup
continue
python
import gdb,json,struct
def u(x):return int(gdb.parse_and_eval(x)) & 0xffffffff
def text(p):return gdb.Value(p).cast(gdb.lookup_type('char').pointer()).string(encoding='latin1')
inf=gdb.selected_inferior();sp=u('$esp');ret,arg=struct.unpack('<II',bytes(inf.read_memory(sp,8)))
assert gdb.newest_frame().older().name()=='synchronize_pch_pragma'
assert arg==u('text_work_cursor')
before=dict(esp=sp,return_address=ret,argument=arg,text=text(arg),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 2
continue
python
row=dict(before=before,esp=u('$esp'),eax=u('$eax'),text=text(u('$eax')),caller=gdb.newest_frame().name(),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
assert row['esp']-sp==4 and row['eax']!=arg and row['text']==before['text']=='hdrstop.pch'
assert row['registers']==before['registers'] and row['caller']=='synchronize_pch_pragma'
with open(%r,'w')as out:json.dump(row,out,indent=2)
end
'''%(D/'stdout',str(D/'registers.json'));(D/'commands.gdb').write_text(cmd);t=time.perf_counter();r=subprocess.run(['/usr/bin/gdb','--nx','--batch','-x',str(D/'commands.gdb'),str(B/'build/native/C13232.EXE')],cwd=work,env=E,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(D/'gdb.log').write_bytes(r.stdout);print(r.stdout.decode('latin1')[-1000:]);assert not r.returncode
row=json.loads((D/'registers.json').read_text());events=(S/'captures/hdrstop-strdup-return-1000/events').read_text();a=[int(x,16)for x in re.search(r'^strdup-entry 0 ([0-9a-f]+) ([0-9a-f]+)$',events,re.M).groups()];z=[int(x,16)for x in re.search(r'^strdup-finish 0 ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+)$',events,re.M).groups()];texts=re.findall(r'^pointer-text (entry|finish) ([0-9a-f]+) ([0-9a-f]+)$',events,re.M);assert z[0]-a[0]==4 and z[1]!=a[1] and [bytes.fromhex(t[2])for t in texts]==[b'hdrstop.pch\0']*2
(D/'receipt.json').write_text(json.dumps(dict(status='pass',seconds=time.perf_counter()-t,native=row,original_entry=a,original_finish=z,original_text=texts,contract='OriginalLIBC __strdup and native hoststrdup exactinput from ownedtext_work_cursor; newlyallocated distinct returnpointer/fullNULtext hdrstop.pch, cdeclRET0/ESPdelta4 and native nonvolatilepreservation. Originalcaller/savedregister evidence raw; no appfunction/nativeCRTinternalgraph credit.'),indent=2)+'\n');print('strdup input/return/text/stack PASS')
