from pathlib import Path
import json,subprocess,sys,time,os,struct
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');D=S/'validation/hdrstop-packing-return';D.mkdir();sys.path.insert(0,str(B/'scripts'));from case_paths import native_work,windows;from binary import PE
C=B/'cases/pragma-hdrstop-pch';P=json.loads((C/'manifest.json').read_text())['phases']['c1-create'];work,paths=native_work(D,P['canonical_working_directory'])
for i in P['inputs']:
 p=work/Path(i['path']).name;p.write_bytes((C/i['path']).read_bytes());os.utime(p,ns=(i['mtime_ns'],i['mtime_ns']))
E=dict(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=windows(B.parents[2]/P['target']),MSC_CMD_FLAGS=P['environment']['MSC_CMD_FLAGS'].format(bin=windows(B.parents[2]/'toolchains/1.0-nt/bin'),include=windows(B.parents[2]/'toolchains/1.0-nt/include')));E.update(paths)
cmd='''set pagination off
set confirm off
set disable-randomization off
break *clear_packing
run >%s 2>&1
python
import gdb,json,struct
def u(x):return int(gdb.parse_and_eval(x)) & 0xffffffff
inf=gdb.selected_inferior();sp=u('$esp');ret,arg=struct.unpack('<II',bytes(inf.read_memory(sp,8)))
before=dict(esp=sp,return_address=ret,argument=arg,object=bytes(inf.read_memory(arg,12)).hex(),scanner_flags=u('scanner_flags'),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 1
continue
python
row=dict(before=before,esp=u('$esp'),object=bytes(inf.read_memory(arg,12)).hex(),caller=gdb.newest_frame().name(),scanner_flags=u('scanner_flags'),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
assert row['esp']-sp==4 and row['object']=='00'*12
assert row['registers']==before['registers'] and row['caller']=='push_packing' and row['scanner_flags']==before['scanner_flags']
with open(%r,'w')as out:json.dump(row,out,indent=2)
end
'''%(D/'stdout',str(D/'registers.json'));(D/'commands.gdb').write_text(cmd);t=time.perf_counter();r=subprocess.run(['/usr/bin/gdb','--nx','--batch','-x',str(D/'commands.gdb'),str(B/'build/native/C13232.EXE')],cwd=work,env=E,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(D/'gdb.log').write_bytes(r.stdout);assert not r.returncode,r.stdout.decode('latin1');row=json.loads((D/'registers.json').read_text());original=PE(B.parents[2]/P['target']);raw=original.read(0x1e810,25);assert raw.hex()=='8b442404c70000000000c7400400000000c7400800000000c3';(D/'receipt.json').write_text(json.dumps(dict(status='pass',seconds=time.perf_counter()-t,native=row,original_body=raw.hex(),original_contract='Unbranched rawbody reads one pointer slot, writeszero at all0/4/8 offsets, RET0; no priorpayload read or hiddenpointer. Fresh original/native entrypointer/formal proof separate. Nativeactual outputfull12zeros/nonvolatile/global/ESP+4 observed on actualreused record.',original_entry_reference='captures/hdrstop-clear-abi-1003'),indent=2)+'\n');print('clear_packing full12byte output/cdecl contract PASS')
