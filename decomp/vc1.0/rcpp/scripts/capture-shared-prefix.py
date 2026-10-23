#!/usr/bin/env python3
"""Explicit two-run original recapture into RCPP-local fixed-drive references.
Never imported or invoked by native fixtures. Replacements register only after
all original runs, exact-repeat checks and application-entry checks succeed.
"""
import fcntl
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import time
from case_paths import CAPTURES, PREFIX, ROOT, capture_lock, windows
from reference import HERE, wt, imports, INTERRUPT
from binary import PE


def sha(data): return hashlib.sha256(data).hexdigest()
def record(base,path):
    data=path.read_bytes()
    return dict(path=path.relative_to(base).as_posix(),bytes=len(data),sha256=sha(data))
def write(path,value): path.write_text(json.dumps(value,indent=2)+'\n')


def main():
    start=time.perf_counter()
    cfg=json.loads((HERE/'functions.json').read_text())
    cache=json.loads((HERE/'cases/native-traces.json').read_text())
    spec=importlib.util.spec_from_file_location('native_specs',HERE/'scripts/check-native.py')
    native=importlib.util.module_from_spec(spec);spec.loader.exec_module(native)
    specs={name:native.load_spec(name) for name in cfg['cases']}
    target=ROOT/cfg['target']['path'];pe=PE(target)
    assert sha(target.read_bytes())==cfg['target']['sha256']
    records={native_name(f['name']):f for f in cfg['functions'] if f['name'].startswith('_')}
    totals=[];registrations={};destination=HERE/'cases/shared-prefix'
    with capture_lock():
        for name,old in specs.items():
            case_start=time.perf_counter();fixture=destination/name
            assert not fixture.exists(), 'Explicit recapture refuses to overwrite registered/raw evidence'
            fixture.mkdir(parents=True)
            shutil.copytree(old['fixture']/'source',fixture/'source')
            for path,data in old['extras'].items():(fixture/'source'/path).write_bytes(data)
            args=list(old['arguments'])
            command=subprocess.list2cmdline(args)
            oracle=None;proofs=[]
            for number in range(2):
                capture=CAPTURES/name/('shared-%d'%number)
                if capture.exists():shutil.rmtree(capture)
                work=capture/'work';shutil.copytree(fixture/'source',work)
                before={p.relative_to(work).as_posix():p.read_bytes()for p in work.rglob('*')if p.is_file()}
                stem=capture/'original';run_start=time.perf_counter()
                if name=='console-interrupt':
                    thunks={f['name']:f['start']-pe.base for f in imports(pe)}
                    rvas=[thunks['import:SetConsoleCtrlHandler'],thunks['import:ReadFile'],records['rc_control_handler']['start']-pe.base]
                    events=INTERRUPT.execute(target,command,work,stem,rvas,'shared')
                    observed=['rc_control_handler'];assert observed==cache['cases'][name]['expected']
                else:
                    plan=capture/'entries.txt';entry=cache['cases'][name]
                    pending=cache['accepted_application'][entry['seen_count']:]
                    names=entry['expected']+[n for n in pending if n not in entry['expected']]
                    plan.write_text(''.join('%x %02x %s\n'%(records[n]['start']-pe.base,pe.read(records[n]['start'],1)[0],n) for n in names))
                    result=subprocess.run(['wine',windows(HERE/'build/debug-run.exe'),windows(target),command,
                        windows(str(stem)+'.stdout'),windows(str(stem)+'.stderr'),windows(str(stem)+'.events'),windows(work),windows(plan)],
                        env=wt.environment(),stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=90)
                    (capture/'host.stdout').write_bytes(result.stdout);(capture/'host.stderr').write_bytes(result.stderr)
                    assert result.returncode==0,(name,result.returncode,result.stderr)
                    events=Path(str(stem)+'.events').read_text()
                    observed=[line.split()[1]for line in events.splitlines()if line.startswith('entry ')]
                    assert observed==entry['expected'],(name,observed,entry['expected'])
                execution=time.perf_counter()-run_start
                assert events.splitlines()[-1]=='exit %d'%old['exit'],(name,events)
                for path,data in before.items():assert(work/path).read_bytes()==data
                products={p.relative_to(work).as_posix():p.read_bytes()for p in work.rglob('*')if p.is_file()}
                outputs={n:d for n,d in products.items()if n not in before}
                assert outputs.keys()==old['outputs'].keys(),(name,outputs.keys())
                streams={kind:Path(str(stem)+'.'+kind).read_bytes()for kind in ['stdout','stderr']}
                value=(streams,outputs)
                if oracle is None:oracle=value
                else:assert value==oracle,(name,'original repeats differ')
                evidence=fixture/'evidence'/str(number);evidence.mkdir(parents=True)
                for kind in ['stdout','stderr','events']:
                    shutil.copyfile(Path(str(stem)+'.'+kind),evidence/kind)
                if name=='console-interrupt':shutil.copyfile(Path(str(stem)+'.proof'),evidence/'console.proof')
                for n,d in outputs.items():out=evidence/n;out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes(d)
                proof=dict(run=number,prefix=str(PREFIX),fixed_drives={'R:':str(ROOT),'S:':str(CAPTURES.parents[1])},
                    working_directory=windows(work),arguments=args,target_sha256=cfg['target']['sha256'],
                    execution_seconds=execution,expected_entries=cache['cases'][name]['expected'],observed_entries=observed,
                    entry_method='real Windows console handler breakpoint'if name=='console-interrupt'else'original one-byte entry traps, restore and EIP rewind; accepted prefix seeded',
                    raw_evidence=[record(fixture,p)for p in sorted(evidence.rglob('*'))if p.is_file()])
                write(evidence/'receipt.json',proof);proofs.append(proof)
            oracle_dir=fixture/'oracle';oracle_dir.mkdir()
            for n,d in oracle[0].items():(oracle_dir/(n+'.bin')).write_bytes(d)
            for n,d in oracle[1].items():p=oracle_dir/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(d)
            case=dict(id=name,arguments=args,expected_exit=old['exit'],stdout=record(fixture,oracle_dir/'stdout.bin'),
                stderr=record(fixture,oracle_dir/'stderr.bin'),outputs=[dict(record(fixture,oracle_dir/n),output_name=n)for n in oracle[1]])
            if name=='console-interrupt':case['runner']='real-console'
            manifest=dict(target=cfg['target']['path'],target_sha256=cfg['target']['sha256'],toolchain='1.0-nt',normalization='none',
                source_inputs=[record(fixture,p)for p in sorted((fixture/'source').rglob('*'))if p.is_file()],
                toolchain_dependencies=[dict(path='toolchains/1.0-nt/bin/C1.ERR',sha256=sha((wt.TC/'bin/C1.ERR').read_bytes()))],
                cases=[case],original_capture_runs=2,original_entry_proofs=[record(fixture,fixture/'evidence'/str(i)/'receipt.json')for i in range(2)],
                previous_fixture=cfg['cases'][name]['fixture'],changed_output_names=[n for n,d in oracle[1].items()if d!=old['outputs'][n]])
            write(fixture/'manifest.json',manifest)
            registrations[name]=dict(fixture=fixture.relative_to(HERE).as_posix(),manifest_case=name)
            totals.append(dict(case=name,total_seconds=time.perf_counter()-case_start,execution_seconds=sum(p['execution_seconds']for p in proofs),outputs_changed=manifest['changed_output_names']))
            print(name+': two raw original captures and entry proofs agree',flush=True)
    cfg['cases']=registrations
    write(HERE/'functions.json',cfg)
    write(destination/'capture-validation.json',dict(status='passed',capture_seconds=time.perf_counter()-start,cases=totals,
        no_normalization=True,no_implicit_fixture_recapture=True,old_raw_captures_relabelled=False))


def native_name(name):
    import re
    return re.sub(r'@\d+$','',name[1:])

if __name__=='__main__':main()
