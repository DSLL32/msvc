#!/usr/bin/env python3
"""Linux-only debugger capture. No original runner or compiler dependency."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import shutil
import time
HERE=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--argv0',required=True);p.add_argument('--module-path',required=True);p.add_argument('--negative-module-sha');p.add_argument('--configuration',type=Path);p.add_argument('--reference',type=Path);p.add_argument('--original-inputs',type=Path);p.add_argument('--compiler-frame',action='store_true')
    a=p.parse_args();out=a.output.resolve()
    if out.exists():p.error('output must be new')
    out.mkdir(parents=True)
    from protocol import candidate,validation_versions,read_configuration,native_name
    selected=a.configuration or HERE/'functions.json'
    execution=candidate();execution['files']['decomp/vc5.10/rcdll/functions.json']=sha(selected)
    (out/'candidate.json').write_text(json.dumps(execution,indent=2)+'\n')
    (out/'validation-versions.json').write_text(json.dumps(validation_versions(),indent=2)+'\n')
    config=selected;native=HERE/'build/native/rcdll-host'
    (out/'captured-native.elf').write_bytes(native.read_bytes())
    (out/'compiled-source.json').write_bytes((HERE/'build/native-source.json').read_bytes())
    (out/'functions.json').write_bytes(config.read_bytes())
    (out/'help-resources.bin').write_bytes((HERE/'build/help-resources.bin').read_bytes())
    (out/'data.json').write_bytes((HERE/'data.json').read_bytes())
    env=os.environ.copy();env.update(RCDLL_NATIVE_SNAPSHOT=str(out/'snapshot.json'),RCDLL_NATIVE_CONFIG=str(out/'functions.json'),RCDLL_NATIVE_SCRIPT=str(HERE/'scripts/native-gdb.py'),RECON_RC_ARGV0=a.argv0,RECON_RC_DLL_PATH=a.module_path,RECON_RC_RESOURCES=str(HERE/'build/help-resources.bin'))
    if a.negative_module_sha:env['RCDLL_NEGATIVE_MODULE_SHA']=a.negative_module_sha
    else:env.pop('RCDLL_NEGATIVE_MODULE_SHA',None)
    for k in ['WINEPREFIX','WINEARCH','WINEDLLOVERRIDES','WINEPATH','RECON_RC_ENVIRONMENT']:env.pop(k,None)
    cwd=HERE;target_args=[];actual=None
    if a.reference:
        reference=a.reference.resolve();manifest=json.loads((reference/'manifest.json').read_text())
        assert manifest['case']=='resources-rich'
        for name,value in manifest['files'].items():assert sha(reference/name)==value,name
        for name,info in manifest['source_inputs'].items():assert sha(HERE/'cases/resources-rich/source'/name)==info['sha256'],name
        if a.original_inputs:
            actual=json.loads((a.original_inputs/'receipt.json').read_text())
            assert actual['case']=='resources-rich' and actual['args']==manifest['args']
            for name,info in manifest['source_inputs'].items():assert actual['case_files'][name]==info,name
            manifest=dict(manifest,child_cwd=actual['child_cwd'],argv0=actual['argv0'])
            assert a.argv0==manifest['argv0']
        drive=out/'drive-s';relative=manifest['child_cwd'][3:].replace('\\','/')
        assert manifest['child_cwd'].startswith('S:\\vc5.10\\rcdll\\')
        cwd=drive/relative;cwd.parent.mkdir(parents=True,exist_ok=True)
        shutil.copytree(HERE/'cases/resources-rich/source',cwd)
        profile_path=HERE/'cases/resources-rich/api-profile.json'
        profile=json.loads(profile_path.read_text());assert profile['ansi_codepage']==1252
        assert sha(HERE/'cases/resources-rich'/profile['source'])==profile['source_sha256']
        env['RECON_ACP']=str(profile['ansi_codepage'])
        env.update(RECON_DRIVE_R=str(HERE.parents[2]),RECON_DRIVE_S=str(drive),RECON_CANONICAL_CWD=manifest['child_cwd'])
        environment=((a.original_inputs or reference)/'environment-utf16.bin').read_bytes().decode('utf-16le')
        (out/'case-environment-utf16.bin').write_bytes(environment.encode('utf-16le'))
        env['RECON_RC_ENVIRONMENT']=str(out/'case-environment-utf16.bin')
        values=dict(value.split('=',1)for value in environment.rstrip('\0').split('\0')if value and not value.startswith('='))
        for name in ['INCLUDE','LIB','RCCODEPAGE']:
            if name in values:env[name]=values[name]
            else:env.pop(name,None)
        if a.original_inputs:
            import re
            events=(a.original_inputs/'events.txt').read_text()
            names=re.findall(r'^(?:input|output)-temporary \d+ ([0-9a-f]+)$',events,re.M)
            if names:
                suffixes=[bytes.fromhex(name)[:-1].decode('ascii').rsplit('\\',1)[-1][-6:]for name in names]
                assert len(suffixes)==2 and len(set(suffixes))==1,'unreviewed unequal temporary suffixes'
                env['RECON_MKTEMP_SUFFIX']=suffixes[0]
        target_args=manifest['args']
        (out/'case-reference.json').write_text(json.dumps(dict(manifest=str(reference/'manifest.json'),sha256=sha(reference/'manifest.json'),profile_sha256=sha(profile_path),original_inputs=str(a.original_inputs)if a.original_inputs else None,original_receipt_sha256=sha(a.original_inputs/'receipt.json')if a.original_inputs else None),indent=2)+'\n')
    command=['gdb','--batch','-q',str(native),'-ex','set pagination off','-ex','handle SIGTRAP stop print nopass','-ex','set breakpoint pending on','-ex','source '+str(HERE/'scripts/native-observers.py')]
    script=HERE/'scripts/native-gdb.py'
    if a.compiler_frame:
        command+=['-ex','break recon_IsDBCSLeadByteEx']
        script=out/'compiler-frame-gdb.py';shutil.copyfile(HERE/'scripts/compiler-frame-gdb.py',script)
    command+=['-ex','run '+str(out/'events.txt')+(' '+' '.join(target_args) if target_args else '')+' > '+str(out/'stdout.bin')+' 2> '+str(out/'stderr.bin'),'-ex','source '+str(script)]
    active=read_configuration(config);names=[f['name']for f in active['functions']if f['kind']=='application' and f['name'] in active['accepted']+[active.get('trial_function')]]
    (out/'trace-filter.txt').write_text(''.join(native_name(name)+'\n'for name in names));(out/'trace-seen.txt').write_text('')
    with (out/'trace.txt').open('wb') as trace:
        env.update(RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(out/'trace-filter.txt'),RECON_TRACE_SEEN=str(out/'trace-seen.txt'))
        started=time.perf_counter();result=subprocess.run(command,cwd=cwd,env=env,capture_output=True,timeout=15,pass_fds=(trace.fileno(),));elapsed=time.perf_counter()-started
    (out/'gdb.stdout').write_bytes(result.stdout);(out/'gdb.stderr').write_bytes(result.stderr)
    report=dict(status=result.returncode,elapsed_seconds=elapsed,native_sha256=sha(native),candidate_sha256=sha(config),command=command,files={f.name:dict(bytes=f.stat().st_size,sha256=sha(f)) for f in out.iterdir() if f.is_file()})
    if a.reference:
        report['case_cwd']=str(cwd);report['canonical_cwd']=manifest['child_cwd']
        report['case_files']={str(f.relative_to(cwd)):dict(bytes=f.stat().st_size,sha256=sha(f))for f in cwd.rglob('*')if f.is_file()}
    (out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    if not (out/'snapshot.json').is_file():
        print(json.dumps(dict(kind='failure',gdb_status=result.returncode,stderr=str(out/'stderr.bin'),gdb_stderr=str(out/'gdb.stderr')),indent=2));return 125
    print(json.dumps(json.loads((out/'snapshot.json').read_text()),indent=2));return result.returncode
if __name__=='__main__':raise SystemExit(main())
