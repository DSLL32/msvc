#!/usr/bin/env python3
"""Cached direct C6 converter fixture; execute only the native reconstruction."""
import argparse,hashlib,importlib.util,json,os,struct,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1]
CASE=BINARY/'cases/direct-cvtomf-codeview'
sha=lambda b:hashlib.sha256(b).hexdigest()
def captured(record):
    path=Path(record['path']);assert not path.is_absolute() and '..' not in path.parts
    data=(CASE/path).read_bytes();assert len(data)==record['bytes'] and sha(data)==record['sha256'],'cached reference/input changed: '+str(path)
    return data

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--trace',action='store_true');ap.add_argument('--unsupported-lines',action='store_true')
    ap.add_argument('--manifest',type=Path,default=CASE/'variants/no-lines/manifest.json')
    args=ap.parse_args();begin=time.perf_counter();m=json.loads(args.manifest.read_text());config=json.loads((BINARY/'functions.json').read_text())
    first,last=m['first'],m['last'];reference=json.loads(captured(m['original_receipt']))
    assert reference['masked'] and reference['trace_new'] and reference['variant']=='no-lines'
    assert reference['accepted_count']==last and reference['accepted_sha256']==sha(json.dumps(config['accepted'][:last]).encode())
    assert reference['original_sha256']==m['original_sha256']==config['target']['sha256']
    events=list(map(json.loads,captured(m['original_events']).decode().splitlines()))
    actual_reference_entries=[e['name'][1:] for e in events if e['event']=='first_entry']
    assert actual_reference_entries==m['expected_application_entries']==[n[1:] for n in config['accepted'][first:last]]
    assert events[-1]==dict(event='summary',failed=0,held=0,phase=3,exit=0,result=0,stdout_bytes=108,stderr_bytes=0)
    variant=m['unsupported_line_variant'] if args.unsupported_lines else m
    data=captured(variant['input']);stdout=captured(m['stdout']);stderr=captured(m['stderr']);expected=captured(m['output'])
    assert not stderr and len(expected)==1648
    assert struct.unpack_from('<HH',expected,0)==(0x14c,2) and struct.unpack_from('<H',expected,54)[0]==2
    directory=Path(tempfile.mkdtemp(prefix='cvtomf-cached-',dir=SCRATCH/'vc1.0/link'))
    work,drives=native_work(directory,m['cwd']);(work/'legacy.obj').write_bytes(data)
    executable=BINARY/'build'/('native-trace' if args.trace else 'native')/'cvtomf'
    assert executable.read_bytes()[:5]==b'\x7fELF\x01'
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT')
    for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(key,None)
    stream=None;pass_fds=();trace_prepare=0
    if args.trace:
        t=time.perf_counter();spec=importlib.util.spec_from_file_location('cvtomf_trace_names',BINARY/'scripts/native-traces.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        cache=module.load(BINARY/'build/native-trace');app=cache['application_definitions']
        assert '__wrap_omf_convert_debug_symbols' not in app
        seen=[n for n in cache['accepted_application'] if config['accepted'].index(app[n]['logical'])<first]
        assert not set(actual_reference_entries)&set(seen)
        (directory/'filter.txt').write_text(''.join(n+'\n' for n in sorted(app)));(directory/'seen.txt').write_text(''.join(n+'\n' for n in seen))
        stream=(directory/'trace.txt').open('wb');pass_fds=(stream.fileno(),)
        env.update(RECON_TRACE_FD=str(stream.fileno()),RECON_TRACE_FILTER=str(directory/'filter.txt'),RECON_TRACE_SEEN=str(directory/'seen.txt'))
        trace_prepare=time.perf_counter()-t
    setup=time.perf_counter()-begin
    t=time.perf_counter()
    try:r=subprocess.run([str(executable),*m['argv'][1:]],cwd=work,env=env,pass_fds=pass_fds,capture_output=True,timeout=5)
    finally:
        if stream:stream.close()
    execution=time.perf_counter()-t;t=time.perf_counter()
    (directory/'stdout.bin').write_bytes(r.stdout);(directory/'stderr.bin').write_bytes(r.stderr)
    assert (work/'legacy.obj').read_bytes()==data,'input changed'
    inventory=sorted(str(p.relative_to(work)) for p in work.rglob('*') if p.is_file())
    if args.unsupported_lines:
        assert r.returncode==125 and r.stdout==b'' and r.stderr==bytes.fromhex(variant['stderr_hex'])
        assert inventory==['legacy.obj'],'unsupported stale route emitted an artifact'
        expected_entries=variant['expected_application_entries'];normalization=None
    else:
        assert r.returncode==m['exit']==0 and r.stdout==stdout and r.stderr==stderr
        assert inventory==['converted.obj','legacy.obj']
        output=(work/'converted.obj').read_bytes();assert len(output)==len(expected)
        assert m['normalization']==[dict(offset=4,size=4,reason='COFF header TimeDateStamp from original time(NULL); no other fields ignored')]
        normalize=lambda b:b[:4]+bytes(4)+b[8:]
        assert normalize(output)==normalize(expected),'COFF non-clock bytes differ'
        assert [struct.unpack_from('<IH',output,314+i*6) for i in range(2)]==[(0,0),(0,0)]
        expected_entries=m['expected_application_entries'];normalization=[dict(offset=4,size=4)]
    oracle=time.perf_counter()-t;t=time.perf_counter();entries=None
    if args.trace:
        entries=(directory/'trace.txt').read_text().splitlines();assert entries==expected_entries,'missing/unexpected application entry'
        assert entries.count('omf_convert_debug_symbols')==1
        assert '__wrap_omf_convert_debug_symbols' not in entries
        if args.unsupported_lines:assert 'omf_finish_debug_types' not in entries
    trace=time.perf_counter()-t
    receipt=dict(status='pass',case='direct-cvtomf-codeview-'+('unsupported-lines' if args.unsupported_lines else 'no-lines'),trace=args.trace,entries=entries,exit=r.returncode,stdout_bytes=len(r.stdout),stderr_bytes=len(r.stderr),inventory=inventory,normalization=normalization,executable_sha256=sha(executable.read_bytes()),manifest_sha256=sha(args.manifest.read_bytes()),timing=dict(setup_seconds=setup,trace_prepare_seconds=trace_prepare,execution_seconds=execution,oracle_seconds=oracle,trace_seconds=trace,total_seconds=time.perf_counter()-begin),scope='Only native ELF; cached original references exclusively. Direct parser-owned boundary, no public LINK CLI claim. Unsupported line variant status125 is native policy, not original behavior.')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status='pass',case=receipt['case'],trace=args.trace,receipt=str(directory/'receipt.json'),seconds=receipt['timing']['total_seconds'])))
if __name__=='__main__':main()
