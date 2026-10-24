#!/usr/bin/env python3
"""Cached direct image-helper lifecycle; native ELF only, no capture/debugger."""
import argparse,hashlib,importlib.util,json,os,shutil,struct,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-image-checksum'
sha=lambda b:hashlib.sha256(b).hexdigest()
def captured(item):
    p=Path(item['path']);assert not p.is_absolute() and '..' not in p.parts
    b=(CASE/p).read_bytes();assert len(b)==item['bytes'] and sha(b)==item['sha256'];return b
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--trace',action='store_true');a=p.parse_args();begin=time.perf_counter();m=json.loads((CASE/'manifest.json').read_text());cfg=json.loads((BINARY/'functions.json').read_text());r=json.loads(captured(m['original_receipt']));events=[json.loads(l)for l in captured(m['original_events']).decode().splitlines()]
    assert r['mode']=='original' and r['summary']==dict(event='summary',failed=0,stopped=1,exit=0,stdout_bytes=0,stderr_bytes=0)
    assert r['original_sha256']==cfg['target']['sha256']==m['original_sha256'] and m['normalization']==[]
    roots={0x4ec70:'checksum_image_file',0x4f4f0:'make_import_section_read_only'};entries=list(dict.fromkeys(roots[e['target']]for e in events if e.get('event')=='entry' and e.get('target') in roots));assert entries==m['expected_application_entries']
    assert [n[1:]for n in cfg['accepted'][m['prefix_accepted_count']:m['accepted_last']] if not n.startswith('__')]==entries
    report=b''
    for e in events:
        if e['event']=='return':
            previous,computed=struct.unpack('<4I',bytes.fromhex(e['outputs']))[1::2];report+=struct.pack('<7I',e['phase'],0 if e['phase']==2 else e['eax'],previous,computed,*e['errors'])
    assert report==captured(m['external_report']) and len(report)==280
    source=captured(m['inputs'][0]);refs={Path(x['path']).name:captured(x)for x in m['reference_files']};assert len(refs)==7
    spec=importlib.util.spec_from_file_location('ed',BINARY/'scripts/artifacts-editor.py');ed=importlib.util.module_from_spec(spec);spec.loader.exec_module(ed);pe,opt=ed.layout(source);n=struct.unpack_from('<H',source,pe+6)[0];start=opt+struct.unpack_from('<H',source,pe+20)[0];at=next(start+i*40 for i in range(n)if source[start+i*40:start+i*40+8].rstrip(b'\0')==b'.idata');changed=bytearray(source);flags=struct.unpack_from('<I',changed,at+36)[0];struct.pack_into('<I',changed,at+36,(flags&0x3fffffff)|0x40000000);final=bytearray(changed);struct.pack_into('<I',final,opt+64,ed.checksum(final))
    assert refs['phase0.exe']==source
    for i in [4,5]:assert refs['phase%d.exe'%i]==changed
    for i in [6,7,8,9]:assert refs['phase%d.exe'%i]==final
    assert struct.unpack_from('<I',final,opt+64)[0]==ed.checksum(final)
    d=Path(tempfile.mkdtemp(prefix='image-checksum-cached-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,m['cwd']);(work/'client.exe').write_bytes(source);(d/'stages').mkdir();env={k:v for k,v in os.environ.items()if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,TZ='PST8PDT',LC_ALL='C')
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(k,None)
    exe=BINARY/'build'/('native-trace'if a.trace else'native')/'image-checksum';assert exe.read_bytes()[:5]==b'\x7fELF\x01';stream=None;fds=();prepare=0
    if a.trace:
        t=time.perf_counter();spec=importlib.util.spec_from_file_location('tr',BINARY/'scripts/native-traces.py');tr=importlib.util.module_from_spec(spec);spec.loader.exec_module(tr);cache=tr.load(BINARY/'build/native-trace');app=cache['application_definitions'];seen=[name for name in cache['accepted_application']if cfg['accepted'].index(app[name]['logical'])<m['prefix_accepted_count']];assert not set(seen)&set(entries)
        (d/'filter.txt').write_text(''.join(n+'\n'for n in sorted(app)));(d/'seen.txt').write_text(''.join(n+'\n'for n in seen));stream=(d/'trace.txt').open('wb');fds=(stream.fileno(),);env.update(RECON_TRACE_FD=str(stream.fileno()),RECON_TRACE_FILTER=str(d/'filter.txt'),RECON_TRACE_SEEN=str(d/'seen.txt'));prepare=time.perf_counter()-t
    setup=time.perf_counter()-begin;t=time.perf_counter()
    try:run=subprocess.run([str(exe),str(d/'stages'),str(d/'results.bin')],cwd=work,env=env,capture_output=True,pass_fds=fds,timeout=5)
    finally:
        if stream:stream.close()
    execution=time.perf_counter()-t;t=time.perf_counter();assert run.returncode==0 and run.stdout==captured(m['stdout'])==b'' and run.stderr==captured(m['stderr'])==b'';assert(d/'results.bin').read_bytes()==report
    assert sorted(p.name for p in work.iterdir())==['client.exe']and(work/'client.exe').read_bytes()==final;assert sorted(p.name for p in(d/'stages').iterdir())==sorted(refs)
    for name,data in refs.items():assert(d/'stages'/name).read_bytes()==data,name
    oracle=time.perf_counter()-t;t=time.perf_counter();actual=None
    if a.trace:actual=(d/'trace.txt').read_text().splitlines();assert actual==entries,actual
    traced=time.perf_counter()-t;row=dict(status='pass',case='direct-image-checksum',trace=a.trace,entries=actual,exit=run.returncode,normalization=[],timing=dict(setup_seconds=setup,trace_prepare_seconds=prepare,execution_seconds=execution,oracle_seconds=oracle,trace_seconds=traced,total_seconds=time.perf_counter()-begin),elf_sha256=sha(exe.read_bytes()),manifest_sha256=sha((CASE/'manifest.json').read_bytes()),scope='Captured references only; actual native prior-stage files, scalar/errorreport, whole-file checksums and raw7stage equality; no Wine/debugger/compiler/originalrefresh.')
    (d/'receipt.json').write_text(json.dumps(row,indent=2)+'\n');print(json.dumps(dict(status='pass',case=row['case'],trace=a.trace,receipt=str(d/'receipt.json'),seconds=row['timing']['total_seconds'])))
if __name__=='__main__':main()
