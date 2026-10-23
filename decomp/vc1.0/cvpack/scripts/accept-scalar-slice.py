#!/usr/bin/env python3
"""Accept only root-reviewed98 from actual scalar callback return + real progression."""
import argparse,json,re,shutil,subprocess,tempfile,time
from pathlib import Path
from state import BINARY,ROOT,load,sha,mask
from accept import validate_snapshot
from scalar_block import validate,compare,progression
from seams import approved,compare_returns,check_policy_snapshot

def pair(config,pe,old,new,completion):
    old,new=Path(old),Path(new);captured=json.loads((old/'receipt.json').read_text());receipts=json.loads((new/'receipt.json').read_text());assert len(receipts)==1;receipt=receipts[0];native=new/'hello-debug-dll'
    validate_snapshot(config,BINARY/'functions.json',old,native,captured,receipt,ROOT/config['target']['path'],BINARY/'build/native/CVPACK.EXE')
    assert captured['case']==receipt['case']=='hello-debug-dll'and captured['candidates']==['begin_symbol_block']and not captured['unmasked']and not captured['guard_probe']and not captured['stop_at']and captured['discover_runtime'],'unapproved98 capture'
    assert captured['entry_progress_only']is True and not captured.get('reference_completion')and captured['scalar_completion']is completion and bool(receipt.get('scalar_completion'))is completion,'98 proof mode mismatch'
    assert receipt['terminal']==('observed-scalar-return'if completion else 'trap'),'98 native observation mode mismatch'
    assert captured['conditional_support_sha256']==receipt['conditional_support_sha256'];check_policy_snapshot(old,captured['conditional_support_sha256']);check_policy_snapshot(native,captured['conditional_support_sha256'])
    runtime=json.loads((BINARY/'runtime.json').read_text());function=next(r for r in config['functions']if r['name']=='begin_symbol_block');crt=compare_returns((old/'events').read_text(),json.loads((native/'crt-returns.json').read_text()),approved(config,pe,runtime,'hello-debug-dll'),function['required_conditional_calls_by_case']['hello-debug-dll'])
    app=[r for r in config['functions']if r['status']=='accepted'or r['name']=='begin_symbol_block'];ranges={(r['start'],r['end'],pe.offset(r['start']))for r in runtime['functions']+runtime['imports']}|{(*r['extent'],pe.offset(r['original']))for r in app}
    assert set(tuple(int(x,16)for x in line.split())for line in(old/'runtime-plan').read_text().splitlines())==ranges,'runtime restoration plan broadened'
    with tempfile.TemporaryDirectory(dir=Path.home()/'.ai-tmp/vc1.0-cvpack')as work:
        expected=Path(work)/'mask.exe';mask(config,pe,expected,['begin_symbol_block']);image=bytearray(expected.read_bytes())
        for f in app:image[pe.offset(f['original'])]=0xcc
        assert bytes(image)==(old/'CVPACK.EXE').read_bytes(),'98 executed original mask differs from exact candidate'
    work=native/('debug-work'if completion else 'work');assert {p.name for p in(old/'work').iterdir()}=={p.name for p in work.iterdir()},'98 artifact inventory differs'
    for path in(old/'work').iterdir():assert path.read_bytes()==(work/path.name).read_bytes(),'98 complete artifacts differ'
    from inputs import verify
    verify(json.loads((BINARY/'cases/hello-debug-dll/case.json').read_text()),work)
    for stream in('stdout','stderr'):assert(old/stream).read_bytes()==(native/('debug-'+stream if completion else stream)).read_bytes()==b'','98 streams differ'
    assert 'exit 99\n'in(old/'events').read_text(),'original did not stop at reviewed observation/real trap'
    return crt

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in('scalar-original','scalar-native','progression-original','progression-native'):parser.add_argument('--'+key,required=True,type=Path)
    parser.add_argument('--check-only',action='store_true');args=parser.parse_args();start=time.perf_counter();config,pe=load();p=validate(config,pe);f=next(r for r in config['functions']if r['name']=='begin_symbol_block');assert f['status']=='candidate'and len(config['accepted_order'])==97,'only reviewed98 candidate'
    crt=dict(scalar=pair(config,pe,args.scalar_original,args.scalar_native,True),progression=pair(config,pe,args.progression_original,args.progression_native,False))
    scalar=compare(config,pe,(args.scalar_original/'events').read_text(),json.loads((args.scalar_native/'hello-debug-dll/scalar-block.json').read_text()));native=args.progression_native/'hello-debug-dll';progress=progression(config,pe,(args.progression_original/'events').read_text(),json.loads((native/'gdb.json').read_text()),(native/'trace').read_text().splitlines())
    apps={r['original']:r for r in config['functions']};prefix=[apps[int(a,16)]['native']for a in re.findall(r'^runtime ([0-9a-f]+) [0-9a-f]+$',(args.scalar_original/'events').read_text(),re.M)if int(a,16)in apps];assert prefix==progress['entry_trace'][:len(prefix)]and prefix[-1]=='begin_symbol_block','slice actual entry prefix differs'
    subprocess.run(['python3','scripts/audit.py'],cwd=BINARY,check=True)
    if args.check_only:print('Reviewed98 scalar callback + genuine next ABI proof passes; not accepted');return
    directory=BINARY/'cases/hello-debug-dll/evidence/0098-begin_symbol_block';directory.mkdir(exist_ok=False)
    for label,source in(('scalar-original',args.scalar_original),('scalar-native',args.scalar_native/'hello-debug-dll'),('progression-original',args.progression_original),('progression-native',native)):
        target=directory/label;target.mkdir()
        for name in('receipt.json','events','configuration.json','runtime-support.json','runtime-plan','stdout','stderr','gdb.json','commands.gdb','gdb.stdout','gdb.stderr','debug-stdout','debug-stderr','trace','crt-returns.json','scalar-block.json'):
            if(source/name).is_file():shutil.copyfile(source/name,target/name)
        if label.endswith('native'):shutil.copyfile(source.parent/'receipt.json',target/'receipt.json')
    proof=dict(function=f['name'],case='hello-debug-dll',sequence=98,extent=f['extent'],classification='exact98-first-zero-suppression-callback-slice-and-genuine-next-entry',scalar=scalar,progression=progress,prior_full_module3=p['prior_module3'],conditional_CRT_support=crt,application_entries=progress['entry_trace'],later_module_heap_equality=False,elapsed=time.perf_counter()-start)
    path=directory/'proof.json';path.write_text(json.dumps(proof,indent=2)+'\n');f.update(status='accepted',evidence=str(path.relative_to(BINARY)));config['accepted_order'].append(dict(function=f['name'],case='hello-debug-dll',kind='application',sequence=98));(BINARY/'functions.json').write_text(json.dumps(config,indent=2)+'\n');print('Accepted98 begin_symbol_block; genuine next strip_debug(handle15), later symbol heap unobserved.')

if __name__=='__main__':main()
