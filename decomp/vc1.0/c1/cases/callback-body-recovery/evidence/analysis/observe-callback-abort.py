from pathlib import Path
import json,subprocess,sys,time,re
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');D=S/'validation/callback-body-abort-return';D.mkdir();sys.path.insert(0,str(B/'scripts'));from case_paths import native_work,windows
C=B/'cases/callback-body-recovery';P=json.loads((C/'manifest.json').read_text())['phases']['c1'];work,paths=native_work(D,P['canonical_working_directory'])
for i in P['inputs']:(work/Path(i['path']).name).write_bytes((C/i['path']).read_bytes())
E=dict(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=windows(B.parents[2]/P['target']),MSC_CMD_FLAGS=P['environment']['MSC_CMD_FLAGS'].format(bin=windows(B.parents[2]/'toolchains/1.0-nt/bin'),include=windows(B.parents[2]/'toolchains/1.0-nt/include')));E.update(paths)
cmd='''set pagination off
set confirm off
set disable-randomization off
break *abort_function_definition
run >%s 2>&1
python
import gdb,json,struct
def u(x):return int(gdb.parse_and_eval(x)) & 0xffffffff
fields=['function_parameter_declarations','current_parameter','function_parameter_context','declaration_controls.fields.parameter_context','function_symbol']
def snapshot():return dict(values=[u(n)for n in fields],esp=u('$esp'),flags=[u('scanner_flags'),u('parser_flags')],registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
rows=[]
for index in range(2):
    before=snapshot();ret=struct.unpack('<I',bytes(gdb.selected_inferior().read_memory(before['esp'],4)))[0]
    gdb.Breakpoint('*'+str(ret),temporary=True);gdb.execute('disable 1');gdb.execute('continue')
    after=snapshot();caller=gdb.newest_frame().name()
    assert before['values']==after['values']==[0]*5
    assert after['flags']==[(before['flags'][0]|0x01000000)&~0x00080000,before['flags'][1]]
    assert after['esp']-before['esp']==4 and after['registers']==before['registers'] and caller=='prepare_function_definition'
    rows.append(dict(index=index,before=before,after=after,caller=caller))
    gdb.execute('enable 1')
    if index==0:gdb.execute('continue')
with open(%r,'w')as out:json.dump(rows,out,indent=2)
end
'''%(D/'stdout',str(D/'registers.json'));(D/'commands.gdb').write_text(cmd);t=time.perf_counter();r=subprocess.run(['/usr/bin/gdb','--nx','--batch','-x',str(D/'commands.gdb'),str(B/'build/native/C13232.EXE')],cwd=work,env=E,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(D/'gdb.log').write_bytes(r.stdout);assert not r.returncode,r.stdout.decode('latin1');rows=json.loads((D/'registers.json').read_text());ev=(S/'captures/callback-body-abort-return-original/events').read_text();original=[]
for n,row in enumerate(rows):
    a=[int(x,16)for x in re.search(r'^abort-registers %d entry (.*)$'%n,ev,re.M)[1].split()];z=[int(x,16)for x in re.search(r'^abort-registers %d finish (.*)$'%n,ev,re.M)[1].split()];assert z[0]-a[0]==4 and a[1:]==z[1:]
    states=[int(x,16)for x in re.search(r'^abort-state %d entry (.*)$'%n,ev,re.M)[1].split()];outputs=[int(x,16)for x in re.search(r'^abort-state %d finish (.*)$'%n,ev,re.M)[1].split()];assert states==row['before']['values']and outputs==row['after']['values']
    f=[int(x,16)for x in re.search(r'^flag-words %d entry (.*)$'%n,ev,re.M)[1].split()];g=[int(x,16)for x in re.search(r'^flag-words %d finish (.*)$'%n,ev,re.M)[1].split()];assert f==row['before']['flags']and g==row['after']['flags'];original.append(dict(index=n,entry_registers=a,return_registers=z,input_globals=states,output_globals=outputs,input_flags=f,output_flags=g))
proof=dict(status='pass',seconds=time.perf_counter()-t,native=rows,original=original,contract='Both real aborted definitions: five separate DWORD object inputs/outputs, scanner/parser complete32bitwords, cdeclRET0/callerESP+4/nonvolatiles and prepare_function_definition caller match. Actual all five valuesNULL/0 on both entry+return. Raw2a508 ORWORD@scanner+2 0100 and2a511 ANDWORDfff7 establish fullword OR01000000/clear00080000; actual0→01000000. No nonzero context seeded or unwritten bytes invented.');(D/'receipt.json').write_text(json.dumps(proof,indent=2)+'\n');print('Both actual abort contexts/scanner masks/RET0 PASS')
