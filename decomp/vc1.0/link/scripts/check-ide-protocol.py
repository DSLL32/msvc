#!/usr/bin/env python3
"""Cached-only native direct IDE fixture. No original/DLL/debugger execution."""
import argparse,hashlib,importlib.util,json,os,struct,subprocess,tempfile,time
from pathlib import Path
from case_paths import ROOT,SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/ide-callback-protocol'
sha=lambda b:hashlib.sha256(b).hexdigest()
def captured(record):
    data=(CASE/record['path']).read_bytes()
    assert sha(data)==record['sha256']
    if 'bytes' in record:assert len(data)==record['bytes']
    return data

def validate_protocol(data):
    assert len(data)==1392 and struct.unpack_from('<8I',data)==(0x31454449,1,1,2,7,77,0,0)
    messages=[b'@I0',b'@I1entry',b'@I2source.obj',b'@I3library.lib',b'@I4symbol_name',b'@I54275878552',b'@I6305419896'];offset=0;buffer=bytearray(b'\xa5'*1024)
    for i,message in enumerate(messages):
        assert struct.unpack_from('<5I',data,32+20*i)==(7,len(message),len(message),offset,1)
        offset+=len(message);buffer[:len(message)+1]=message+b'\0'
    assert data[172:249]==b''.join(messages) and data[281:1305]==buffer
    assert data[249:281]==data[1305:1337]==b'\xa5'*32
    assert struct.unpack_from('<9I',data,1337)==(7,3,8,13,14,14,13,12,1)
    assert data[1373:]==b'alpha.obj,beta.lib\0'

def validate_trace(actual,expected):
    assert expected==['initialize_ide_api','emit_ide_message','read_ide_flags'] and actual==expected,'IDE first-entry sequence'

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--trace',action='store_true');args=ap.parse_args();start=time.perf_counter()
    variant='native-trace' if args.trace else 'native';exe=BINARY/'build'/variant/'ide-protocol';assert exe.read_bytes()[:5]==b'\x7fELF\x01'
    metadata=json.loads((CASE/'manifest.json').read_text());config=json.loads((BINARY/'functions.json').read_text());assert metadata['original_sha256']==config['target']['sha256']
    original=json.loads(captured(metadata['original_entry_receipt']));raw=captured(metadata['original_events']);events=[json.loads(x) for x in raw.decode().splitlines()]
    assert original['mode']=='masked' and original['trial']==metadata['trial_candidate']=='read_ide_flags'
    ceiling=original['accepted_count'];assert ceiling==metadata['last']-1 and config['accepted'][ceiling]=='_read_ide_flags'
    assert original['accepted_sha256']==sha(json.dumps(config['accepted'][:ceiling]).encode())
    assert original['summary']['failed']==0 and original['summary']['stopped']==1 and original['summary']['exit']==0
    mapping={r['start']:r['name'][1:] for r in config['driver']};observed=list(dict.fromkeys(mapping[x['eip']] for x in events if x['event']=='target_entry'))
    validate_trace(observed,metadata['expected_application_entries'])
    assert config['accepted'][metadata['accepted_prefix']:metadata['last']]==['_'+n for n in metadata['expected_application_entries']]
    protocol=captured(metadata['protocol']);validate_protocol(protocol);stdout=captured(metadata['stdout']);stderr=captured(metadata['stderr']);captured(metadata['dependency'])
    directory=Path(tempfile.mkdtemp(prefix='ide-cached-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(directory,r'S:\vc1.0\link\ide-callback-protocol')
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT',**metadata['environment'])
    fd=None;pass_fds=();trace_prepare=0
    if args.trace:
        begin=time.perf_counter();spec=importlib.util.spec_from_file_location('ide_trace_names',BINARY/'scripts/native-traces.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);cache=module.load(BINARY/'build/native-trace');app=cache['application_definitions']
        names=metadata['expected_application_entries'];assert all(app[n]['logical']=='_'+n for n in names)
        seen=[n for n in cache['accepted_application'] if config['accepted'].index(app[n]['logical'])<metadata['accepted_prefix']]
        assert not set(seen)&set(names)
        filter_path=directory/'filter.txt';seen_path=directory/'seen.txt';filter_path.write_text(''.join(n+'\n' for n in sorted(app)));seen_path.write_text(''.join(n+'\n' for n in seen))
        fd=(directory/'trace.txt').open('wb');pass_fds=(fd.fileno(),);env.update(RECON_TRACE_FD=str(fd.fileno()),RECON_TRACE_FILTER=str(filter_path),RECON_TRACE_SEEN=str(seen_path));trace_prepare=time.perf_counter()-begin
    setup=time.perf_counter()-start;begin=time.perf_counter();result=subprocess.run([str(exe),str(directory/'protocol.bin')],cwd=work,env=env,pass_fds=pass_fds,capture_output=True,timeout=5);execution=time.perf_counter()-begin
    if fd:fd.close()
    begin=time.perf_counter();assert result.returncode==metadata['expected_driver_exit'] and result.stdout==stdout and result.stderr==stderr
    actual=(directory/'protocol.bin').read_bytes();validate_protocol(actual);assert actual==protocol,'full protocol artifact differs'
    assert sorted(str(p.relative_to(work)) for p in work.rglob('*') if p.is_file())==metadata['expected_target_files'];oracle=time.perf_counter()-begin
    begin=time.perf_counter();entries=None
    if args.trace:entries=(directory/'trace.txt').read_text().splitlines();validate_trace(entries,metadata['expected_application_entries'])
    trace=time.perf_counter()-begin
    receipt=dict(status='pass',case=metadata['case'],trace=args.trace,entries=entries,driver_exit=result.returncode,stdout_bytes=len(result.stdout),stderr_bytes=len(result.stderr),protocol_bytes=len(actual),protocol_sha256=sha(actual),elf_sha256=sha(exe.read_bytes()),manifest_sha256=sha((CASE/'manifest.json').read_bytes()),timing=dict(setup_seconds=setup,trace_prepare_seconds=trace_prepare,execution_seconds=execution,oracle_seconds=oracle,trace_seconds=trace,total_seconds=time.perf_counter()-start),scope='Direct external test; no public CLI/callback application-vertex claim; no original/DLL/debugger runs')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status='pass',case=metadata['case'],trace=args.trace,seconds=receipt['timing']['total_seconds'],receipt=str(directory/'receipt.json'))))
if __name__=='__main__':main()
