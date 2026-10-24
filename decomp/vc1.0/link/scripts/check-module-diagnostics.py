#!/usr/bin/env python3
"""Cached module/library diagnostics. Native ELF only; no reference execution."""
import argparse,hashlib,importlib.util,json,os,shutil,struct,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-module-diagnostics'
sha=lambda b:hashlib.sha256(b).hexdigest()
def captured(item):
    p=Path(item['path']);assert not p.is_absolute() and '..' not in p.parts
    b=(CASE/p).read_bytes();assert len(b)==item['bytes'] and sha(b)==item['sha256'];return b
def inspect_image(data,stamp):
    assert data[:2]==b'MZ';pe=struct.unpack_from('<I',data,60)[0];assert data[pe:pe+4]==b'PE\0\0'
    machine,count,timestamp,symbols,total,opt,flags=struct.unpack_from('<HHIIIHH',data,pe+4);assert machine==0x14c and timestamp==stamp
    header=pe+24;assert struct.unpack_from('<H',data,header)[0]==0x10b
    rva,size=struct.unpack_from('<II',data,header+96+6*8);assert rva and size and size%28==0
    def file_offset(rva):
        for i in range(count):
            p=header+opt+i*40;length,address,raw,offset=struct.unpack_from('<4I',data,p+8)
            if address<=rva<address+max(length,raw):return offset+rva-address
        raise AssertionError('debugRVA outside section')
    at=file_offset(rva);types=[]
    for p in range(at,at+size,28):
        kind,length,base,offset=struct.unpack_from('<4I',data,p+12);types.append(kind)
        if kind==2:assert data[offset:offset+4]==b'NB05'
    assert 1 in types and 2 in types;return types

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--trace',action='store_true');p.add_argument('--manifest',type=Path,default=CASE/'manifest.json');a=p.parse_args();begin=time.perf_counter();m=json.loads(a.manifest.read_text());config=json.loads((BINARY/'functions.json').read_text());r=json.loads(captured(m['original_receipt']));es=[json.loads(l) for l in captured(m['original_events']).decode().splitlines()]
    assert r['original_sha256']==m['original_sha256']==config['target']['sha256'] and r['mode']=='original'
    assert r['summary']==dict(event='summary',failed=0,stopped=1,exit=0,phase=6,stdout_bytes=203,stderr_bytes=0)
    names={0x35760:'add_default_libraries',0x35520:'print_module_debug_info'};expected_entries=list(dict.fromkeys(names[e['target']] for e in es if e.get('event')=='entry' and e.get('target') in names));assert expected_entries==m['expected_application_entries']
    assert m['expected_application_entries']==[n[1:] for n in config['accepted'][m['prefix_accepted_count']:m['accepted_last']]]
    clock=[e for e in es if e['event']=='clock_boundary'];assert clock==[m['clock_proof']] and clock[0]['result']==clock[0]['output']==m['fixed_time'] and m['normalization']==[]
    stdout=captured(m['stdout']);stderr=captured(m['stderr']);report=captured(m['external_report']);expected={Path(item['path']).name:captured(item) for item in m['files']};expected['diagnostics.bin']=report
    assert struct.unpack_from('<4I',report)==(0x31444d4c,1,0,2);types=inspect_image(expected['modules.exe'],m['fixed_time'])
    d=Path(tempfile.mkdtemp(prefix='module-diagnostics-cached-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,m['cwd']);input_path=d/'libraries.bin'
    for item in m['inputs']:
        data=captured(item);path=Path(item['path']).name
        if path=='libraries.bin':input_path.write_bytes(data)
        else:(work/path).write_bytes(data)
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,RECON_FIXED_TIME=str(m['fixed_time']),LC_ALL='C',TZ='PST8PDT')
    for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(key,None)
    exe=BINARY/'build'/('native-trace' if a.trace else 'native')/'module-diagnostics';assert exe.read_bytes()[:5]==b'\x7fELF\x01';stream=None;pass_fds=();prepare=0
    if a.trace:
        t=time.perf_counter();spec=importlib.util.spec_from_file_location('module_trace',BINARY/'scripts/native-traces.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);cache=module.load(BINARY/'build/native-trace');app=cache['application_definitions'];first=m['prefix_accepted_count'];seen=[n for n in cache['accepted_application'] if config['accepted'].index(app[n]['logical'])<first];assert not set(seen)&set(expected_entries)
        (d/'filter.txt').write_text(''.join(n+'\n' for n in sorted(app)));(d/'seen.txt').write_text(''.join(n+'\n' for n in seen));stream=(d/'trace.txt').open('wb');pass_fds=(stream.fileno(),);env.update(RECON_TRACE_FD=str(stream.fileno()),RECON_TRACE_FILTER=str(d/'filter.txt'),RECON_TRACE_SEEN=str(d/'seen.txt'));prepare=time.perf_counter()-t
    setup=time.perf_counter()-begin;t=time.perf_counter()
    try:result=subprocess.run([str(exe),str(input_path),*m['target_argv']],cwd=work,env=env,pass_fds=pass_fds,capture_output=True,timeout=5)
    finally:
        if stream:stream.close()
    execution=time.perf_counter()-t;t=time.perf_counter();assert result.returncode==0 and result.stdout==stdout and result.stderr==stderr
    inventory=sorted(str(p.relative_to(work)) for p in work.rglob('*') if p.is_file());assert inventory==sorted(expected)
    for name,data in expected.items():assert (work/name).read_bytes()==data,('nonidenticalartifact',name)
    assert input_path.read_bytes()==captured(next(item for item in m['inputs'] if item['path'].endswith('libraries.bin')))
    inspect_image((work/'modules.exe').read_bytes(),m['fixed_time']);oracle=time.perf_counter()-t;t=time.perf_counter();entries=None
    if a.trace:entries=(d/'trace.txt').read_text().splitlines();assert entries==expected_entries,entries
    trace=time.perf_counter()-t;row=dict(status='pass',case='direct-module-diagnostics',reach=m['reach'],trace=a.trace,entries=entries,exit=result.returncode,inventory=inventory,stdout_bytes=len(result.stdout),stderr_bytes=len(result.stderr),debug_types=types,clock_input=m['fixed_time'],normalization=[],elf_sha256=sha(exe.read_bytes()),manifest_sha256=sha(a.manifest.read_bytes()),timing=dict(setup_seconds=setup,trace_prepare_seconds=prepare,execution_seconds=execution,oracle_seconds=oracle,trace_seconds=trace,total_seconds=time.perf_counter()-begin),scope='Captured references only, nativeELF; actualnormalmainreturn preserved, appended support entries explicitlydirect; no Wine/debugger/compiler/child/CVPACK/reference refresh.')
    (d/'receipt.json').write_text(json.dumps(row,indent=2)+'\n');print(json.dumps(dict(status='pass',case='direct-module-diagnostics',trace=a.trace,receipt=str(d/'receipt.json'),seconds=row['timing']['total_seconds'])))
if __name__=='__main__':main()
