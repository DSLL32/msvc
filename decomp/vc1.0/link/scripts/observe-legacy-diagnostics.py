#!/usr/bin/env python3
"""Native GDB ABI observer for the explicitly external callback caller."""
import argparse,hashlib,json,os,struct,subprocess,tempfile,time
from pathlib import Path
from case_paths import ROOT,SCRATCH,PROGRAM
BINARY=Path(__file__).resolve().parents[1]
CASE=BINARY/'cases/legacy-resource-diagnostics'
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--function',choices=['strings','menu','dialog'],required=True)
    ap.add_argument('--return-state',action='store_true')
    options=ap.parse_args()
    payload=next(x for x in json.loads((CASE/'analysis/provenance.json').read_text())['payloads'] if x['label']==options.function)
    parent=SCRATCH/'vc1.0/link/diagnostic-native-observations';parent.mkdir(parents=True,exist_ok=True)
    work=Path(tempfile.mkdtemp(prefix=options.function+'-',dir=parent));output=work/'state.json'
    source=r'''
import gdb,json,struct
from pathlib import Path
out=Path(OUTPUT)
def val(x):return int(gdb.parse_and_eval(x))&0xffffffff
def memory(address,size):return bytes(gdb.selected_inferior().read_memory(address,size))
def words(address,count):return list(struct.unpack('<'+'I'*count,memory(address,count*4)))
def snap():
    esp=val('$esp');args=words(esp,3);stream=words(val('&recon_iob[1]'),8);n=stream[0]-stream[2]
    assert 0<=n<=4096
    pointer=val('diagnostic_input');size=val('diagnostic_input_size')
    return dict(eip=val('$eip'),esp=esp,arguments=args,registers={n:val('$'+n) for n in ['eax','ebx','esi','edi','ebp']},
        input_pointer=pointer,input_size=size,payload=memory(pointer,size).hex(),guards=memory(pointer-32,32).hex()+memory(pointer+size,32).hex(),
        stdout_file=stream,buffered=memory(stream[2],n).hex() if n else '',
        errors=[val('*diagnostic_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],
        mb_cur_max=val('recon_mb_cur_max'),ctype=memory(val('recon_pctype')-2,514).hex())
gdb.execute('set pagination off');gdb.execute('set confirm off')
gdb.execute('break *'+FUNCTION)
gdb.execute('run '+RUN_ARGUMENTS+' >'+STDOUT+' 2>'+STDERR)
assert val('$eip')==val('&'+FUNCTION),'wrong native entry'
state=dict(entry=snap())
if RETURNS:
    before=state['entry'];gdb.execute('tbreak *'+str(before['arguments'][0]));gdb.execute('continue')
    state['return']=snap();after=state['return']
    assert after['eip']==before['arguments'][0] and after['esp']==before['esp']+4,'cdecl stack return mismatch'
    for n in ['ebx','esi','edi','ebp']:assert before['registers'][n]==after['registers'][n],n
    assert after['payload']==before['payload'] and after['guards']==before['guards'],'input/guard mutation'
    gdb.execute('continue')
    assert gdb.selected_inferior().pid==0,'native did not exit'
out.write_text(json.dumps(state,indent=2)+'\n')
'''
    source=source.replace('OUTPUT',repr(str(output))).replace('FUNCTION',repr('display_legacy_'+options.function)).replace('STDOUT',repr(str(work/'stdout.bin'))).replace('STDERR',repr(str(work/'stderr.bin'))).replace('RETURNS',repr(options.return_state)).replace('RUN_ARGUMENTS',repr(str(payload['type'])+' '+str(CASE/'inputs'/(options.function+'.bin'))))
    script=work/'observe.py';script.write_text(source)
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    env.update(RECON_PROGRAM_PATH=PROGRAM,RECON_DRIVE_R=str(ROOT),RECON_DRIVE_S=str(SCRATCH),RECON_CANONICAL_CWD=r'S:\vc1.0\link\legacy-resource-diagnostics',LC_ALL='C',TZ='PST8PDT')
    command=['gdb','-q','-nx','-batch','-ex','file '+str(BINARY/'build/native/legacy-diagnostics'),'-ex',f'set args {payload["type"]} {CASE/"inputs"/(options.function+".bin")}', '-ex','source '+str(script)]
    begin=time.perf_counter();r=subprocess.run(command,env=env,cwd=work,capture_output=True,timeout=20);elapsed=time.perf_counter()-begin
    (work/'gdb.stdout').write_bytes(r.stdout);(work/'gdb.stderr').write_bytes(r.stderr)
    assert r.returncode==0 and output.exists(),(r.returncode,r.stdout.decode(),r.stderr.decode())
    actual=json.loads(output.read_text());expected=[json.loads(x) for x in (CASE/'baseline'/options.function/'1/events.jsonl').read_text().splitlines()]
    from legacy_diagnostic_abi import validate
    checks=validate(actual,expected,(CASE/'inputs'/(options.function+'.bin')).read_bytes(),options.function,options.return_state)
    if options.return_state:
        assert (work/'stdout.bin').read_bytes()==(CASE/'baseline'/options.function/'1/stdout.bin').read_bytes()
        assert (work/'stderr.bin').read_bytes()==b''
    receipt=dict(status='pass',function=options.function,external_caller=True,return_checked=options.return_state,seconds=elapsed,checks=checks,elf_sha256=hashlib.sha256((BINARY/'build/native/legacy-diagnostics').read_bytes()).hexdigest(),source_sha256=hashlib.sha256((BINARY/'src/resource-legacy.c').read_bytes()).hexdigest(),command=command)
    (work/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(work)
if __name__=='__main__':main()
