from pathlib import Path
import json,subprocess,sys,time,re
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');D=S/'validation/asm-segment-registers';D.mkdir();sys.path.insert(0,str(B/'scripts'));from case_paths import native_work
P=json.loads((B/'cases/asm-parser-repair/manifest.json').read_text())['phases']['c1'];work,paths=native_work(D,P['canonical_working_directory'])
for i in P['inputs']:(work/Path(i['path']).name).write_bytes((B/'cases/asm-parser-repair'/i['path']).read_bytes())
E=json.loads((S/'validation/asm-segment-entry-996/receipt.json').read_text())['environment'];E.update(paths)
cmd='''set pagination off
set confirm off
set disable-randomization off
break make_asm_segment_value
run >%s 2>&1
python
import gdb,json
def state():
    o=gdb.parse_and_eval('asm_operand').dereference();a=o['address'].dereference();s=a['symbol'];v=s.dereference()
    return [int(gdb.parse_and_eval('asm_operand_count')),int(o['size']),int(o['flags']),int(a['displacement']),int(a['address_flags']),int(a['flags']),int(a['first_register']),int(a['second_register']),int(bool(int(s))),int(v['storage'][0]),int(v['index'])]
entry=dict(esp=int(gdb.parse_and_eval('$ebp'))+4,value=int(gdb.parse_and_eval('value')),state=state())
end
finish
python
row=dict(entry=entry,esp=int(gdb.parse_and_eval('$esp')),eax=int(gdb.parse_and_eval('$eax')) & 0xffffffff,state=state(),caller=gdb.newest_frame().name())
assert row['esp']-entry['esp']==8 and row['eax']==entry['value']
assert row['caller']=='parse_inline_assembler'
with open(%r,'w')as out:json.dump(row,out,indent=2)
end
''' %(D/'stdout',str(D/'registers.json'));(D/'commands.gdb').write_text(cmd);t=time.perf_counter();r=subprocess.run(['/usr/bin/gdb','--nx','--batch','-x',str(D/'commands.gdb'),str(B/'build/native/C13232.EXE')],cwd=work,env=E,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(D/'gdb.log').write_bytes(r.stdout);print(r.stdout.decode('latin1')[-800:]);assert not r.returncode
row=json.loads((D/'registers.json').read_text());ev=(S/'captures/asm-segment-original-seam/events').read_text();before=next([int(x)for x in line.split()[2:]]for line in ev.splitlines()if line.startswith('asm-state entry '));after=next([int(x)for x in line.split()[2:]]for line in ev.splitlines()if line.startswith('asm-state finish '));arg=tuple(int(x,16)for x in re.search(r'^segment-entry 0 ([0-9a-f]+) ([0-9a-f]+)$',ev,re.M).groups());ret=tuple(int(x,16)for x in re.search(r'^segment-finish 0 ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+)$',ev,re.M).groups());assert ret[1]==arg[1] and ret[0]-arg[0]==8;assert row['entry']['state']==before and row['state']==after
(D/'receipt.json').write_text(json.dumps(dict(status='pass',seconds=time.perf_counter()-t,native=row,original_entry=arg,original_finish=ret,original_state_before=before,original_state_after=after,contract='Actual original/native EAX returns identical incoming pointer within eachprocess; RET4/callerESPdelta8. Opaque argument payload is not read by505ba body. Independently raw observed current operand/address/symbol storage andindex match native typed fields before/after; nameanchor preserved. No address normalization of artifacts.'),indent=2)+'\n');print('SEG actual pointer identity/stack/state proof passed')
