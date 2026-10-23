from pathlib import Path
import json,subprocess,sys,time,re,hashlib
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');D=S/'validation/registry-scope-return';D.mkdir();sys.path.insert(0,str(B/'scripts'));from case_paths import native_work,windows
C=B/'cases/prototype-scope-registry';P=json.loads((C/'manifest.json').read_text())['phases']['c1'];work,paths=native_work(D,P['canonical_working_directory'])
for i in P['inputs']:(work/Path(i['path']).name).write_bytes((C/i['path']).read_bytes())
E=dict(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=windows(B.parents[2]/P['target']),MSC_CMD_FLAGS=P['environment']['MSC_CMD_FLAGS'].format(bin=windows(B.parents[2]/'toolchains/1.0-nt/bin'),include=windows(B.parents[2]/'toolchains/1.0-nt/include')));E.update(paths)
cmd='''set pagination off
set confirm off
set disable-randomization off
break *enclosing_declaration_scope
run >%s 2>&1
python
import gdb,json,struct
def u(x):return int(gdb.parse_and_eval(x)) & 0xffffffff
inf=gdb.selected_inferior();sp=u('$esp');ret=struct.unpack('<I',bytes(inf.read_memory(sp,4)))[0]
links=[];scope=gdb.parse_and_eval('current_scope')
while int(scope):
    item=scope.dereference();links.append([int(scope),int(item['next']),int(item['flags']) & 255])
    if links[-1][2]!=1:break
    assert len(links)<16
    scope=item['next']
before=dict(esp=sp,return_address=ret,links=links,selected=u('scope_directory[scope_depth]'),depth=u('scope_depth'),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']},scanner=u('scanner_flags'),parser=u('parser_flags'))
gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 1
continue
python
row=dict(before=before,esp=u('$esp'),result=u('$eax'),caller=gdb.newest_frame().name(),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']},scanner=u('scanner_flags'),parser=u('parser_flags'))
assert row['esp']-sp==4 and row['result']==links[-1][0] and before['selected']==row['result']
assert row['registers']==before['registers'] and row['caller']=='create_tag'
assert row['scanner']==before['scanner'] and row['parser']==before['parser']
with open(%r,'w')as out:json.dump(row,out,indent=2)
end
'''%(D/'stdout',str(D/'registers.json'));(D/'commands.gdb').write_text(cmd);t=time.perf_counter();r=subprocess.run(['/usr/bin/gdb','--nx','--batch','-x',str(D/'commands.gdb'),str(B/'build/native/C13232.EXE')],cwd=work,env=E,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(D/'gdb.log').write_bytes(r.stdout);assert not r.returncode,r.stdout.decode('latin1');row=json.loads((D/'registers.json').read_text());ev=(S/'captures/registry-scope-return-original-retry/events').read_text();a=[int(x,16)for x in re.search(r'^scope-registers entry (.*)$',ev,re.M)[1].split()];z=[int(x,16)for x in re.search(r'^scope-registers finish (.*)$',ev,re.M)[1].split()];links=[[int(a,16),int(b,16),int(c)]for a,b,c in re.findall(r'^scope-link \d+ ([0-9a-f]+) ([0-9a-f]+) (\d+)$',ev,re.M)];ctx=re.search(r'^scope-context ([0-9a-f]+) ([0-9a-f]+) (\d+) ([0-9a-f]+)$',ev,re.M);assert z[0]-a[0]==4 and a[1:]==z[1:5] and z[5]==links[-1][0]==int(ctx[4],16);assert [p[2]for p in links]==[p[2]for p in row['before']['links']]==[1,0]
flags=re.findall(r'^flag-words 0 (entry|finish) ([0-9a-f]+) ([0-9a-f]+)$',ev,re.M);assert flags[0][1:]==flags[1][1:] and int(flags[0][1],16)==row['scanner'] and int(flags[0][2],16)==row['parser']
proof=dict(status='pass',seconds=time.perf_counter()-t,native=row,original_entry=a,original_finish=z,original_context=ctx.groups(),original_chain=links,original_flags=flags,reference_events_sha256=hashlib.sha256(ev.encode()).hexdigest(),contract='Fresh actual owned current_scope.next chain kindBYTE1→0, scope_depth0 and scope-directory selected outerowner independently match; original and native return the exact outerowner within their process. cdeclRET0/callerESPdelta4, all four nonvolatile registers and scanner/parser fullwords unchanged. No crossprocess pointer equality/normalization claim. Whole source runtime independently succeeds0.');(D/'receipt.json').write_text(json.dumps(proof,indent=2)+'\n');print('Actual scope chain/returned owner/RET0/nonvolatile/flags proof PASS')
