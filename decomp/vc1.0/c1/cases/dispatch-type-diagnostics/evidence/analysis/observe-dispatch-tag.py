from pathlib import Path
import json,subprocess,sys,time,re,os
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');D=S/'validation/dispatch-tag-return-1007';D.mkdir();sys.path.insert(0,str(B/'scripts'));from case_paths import native_work,windows
C=B/'cases/dispatch-type-diagnostics';P=json.loads((C/'manifest.json').read_text())['phases']['c1'];work,paths=native_work(D,P['canonical_working_directory'])
for i in P['inputs']:(work/Path(i['path']).name).write_bytes((C/i['path']).read_bytes())
E=dict(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=windows(B.parents[2]/P['target']),MSC_CMD_FLAGS=P['environment']['MSC_CMD_FLAGS'].format(bin=windows(B.parents[2]/'toolchains/1.0-nt/bin'),include=windows(B.parents[2]/'toolchains/1.0-nt/include')));E.update(paths)
cmd='''set pagination off
set confirm off
set disable-randomization off
break *require_complete_type
run >%s 2>&1
python
import gdb,json,struct
def u(x):return int(gdb.parse_and_eval(x)) & 0xffffffff
inf=gdb.selected_inferior();sp=u('$esp');ret,arg=struct.unpack('<II',bytes(inf.read_memory(sp,8)));tag=struct.unpack('<I',bytes(inf.read_memory(arg+8,4)))[0]
before=dict(esp=sp,return_address=ret,argument=arg,tag=tag,flags=bytes(inf.read_memory(tag+1,1))[0],errors=u('error_count'),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 1
continue
python
row=dict(before=before,esp=u('$esp'),tag=struct.unpack('<I',bytes(inf.read_memory(arg+8,4)))[0],flags=bytes(inf.read_memory(tag+1,1))[0],errors=u('error_count'),caller=gdb.newest_frame().name(),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
assert row['esp']-sp==8 and row['tag']==tag and row['flags']==(before['flags']|128) and row['errors']==before['errors']+1
assert row['registers']==before['registers'] and row['caller']=='complete_type_size'
with open(%r,'w')as out:json.dump(row,out,indent=2)
end
'''%(D/'stdout',str(D/'registers.json'));(D/'commands.gdb').write_text(cmd);t=time.perf_counter();r=subprocess.run(['/usr/bin/gdb','--nx','--batch','-x',str(D/'commands.gdb'),str(B/'build/native/C13232.EXE')],cwd=work,env=E,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(D/'gdb.log').write_bytes(r.stdout);assert not r.returncode,r.stdout.decode('latin1');row=json.loads((D/'registers.json').read_text());ev=(S/'captures/dispatch-tag-return-1007/events').read_text();a=[int(x,16)for x in re.search(r'^type-entry 0 ([0-9a-f]+) ([0-9a-f]+)$',ev,re.M).groups()];z=[int(x,16)for x in re.search(r'^type-finish 0 ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+)$',ev,re.M).groups()];x=re.search(r'^tag-byte entry ([0-9a-f]+) (\d+)$',ev,re.M);y=re.search(r'^tag-byte finish ([0-9a-f]+) (\d+)$',ev,re.M);assert z[0]-a[0]==8 and x[1]==y[1] and int(x[2])==row['before']['flags']==2 and int(y[2])==row['flags']==130
(D/'receipt.json').write_text(json.dumps(dict(status='pass',seconds=time.perf_counter()-t,native=row,original_entry=a,original_finish=z,original_tag_before=x.groups(),original_tag_after=y.groups(),contract='Fresh original/native actual tagowner stable, rawBYTE flags2→130; stdcallRET4/callerESPdelta8; nativeerror_count1→2/nonvolatilepreservation and complete_type_size caller. Full original stdout/status4 independently proves exact diagnostic sequence/count.'),indent=2)+'\n');print('Actual tagBYTE2→130/RET4/errorcount proof PASS')
