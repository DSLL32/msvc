from pathlib import Path
import json,subprocess,sys,time
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');D=S/'validation/parser-return8-registers';D.mkdir();sys.path.insert(0,str(B/'scripts'));from case_paths import native_work
P=json.loads((B/'cases/protocol-parser-repair/manifest.json').read_text())['phases']['c1'];work,paths=native_work(D,P['canonical_working_directory'])
for i in P['inputs']:(work/Path(i['path']).name).write_bytes((B/'cases/protocol-parser-repair'/i['path']).read_bytes())
E=json.loads((S/'validation/parser-return8-entry-993/receipt.json').read_text())['environment'];E.update(paths)
cmd='''set pagination off
set confirm off
set disable-randomization off
break recovery_token_value
run >%s 2>&1
python
import gdb,json
entry=dict(esp=int(gdb.parse_and_eval('$ebp'))+4,token=int(gdb.parse_and_eval('token')))
end
finish
python
row=dict(entry=entry,esp=int(gdb.parse_and_eval('$esp')),eax=int(gdb.parse_and_eval('$eax')) & 0xffffffff,edx=int(gdb.parse_and_eval('$edx')) & 0xffffffff,caller=gdb.newest_frame().name())
assert row['esp']-entry['esp']==8
assert entry['token']==130 and row['eax'] & 255==130
assert row['caller']=='parse_translation_unit'
with open(%r,'w')as out:json.dump(row,out,indent=2)
end
''' %(D/'stdout',str(D/'registers.json'));(D/'commands.gdb').write_text(cmd);t=time.perf_counter();r=subprocess.run(['/usr/bin/gdb','--nx','--batch','-x',str(D/'commands.gdb'),str(B/'build/native/C13232.EXE')],cwd=work,env=E,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(D/'gdb.log').write_bytes(r.stdout);print(r.stdout.decode('latin1')[-800:]);assert not r.returncode
entry=json.loads((D/'registers.json').read_text());ev=(S/'captures/parser-original-return8/events').read_text();import re
original_entries=[tuple(int(x,16)for x in r)for r in re.findall(r'^return8-entry \d+ ([0-9a-f]+) ([0-9a-f]+)$',ev,re.M)];original_returns=[tuple(int(x,16)for x in r)for r in re.findall(r'^return8-finish \d+ ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+)$',ev,re.M)];assert len(original_entries)==len(original_returns)==1;assert original_entries[0][1]==130 and original_returns[0][1]&255==130 and original_returns[0][0]-original_entries[0][0]==8
(D/'receipt.json').write_text(json.dumps(dict(status='pass',seconds=time.perf_counter()-t,original_entry=original_entries,original_finish=original_returns,native=entry,defined_payload_mask=['eax & 0xff =130'],uncompared_payload='Original locals leave upper EAX bytes and EDX unspecified for insertedsemicolon130. Rawvalues retained, no invented initialization or artifact normalization.',stack_contract='Original/native entry ESP→caller return ESP difference8: returnaddress4 plus stdcall4; no hidden outputpointer.'),indent=2)+'\n');print('actual return register and caller stack proof passed')
