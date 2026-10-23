#!/usr/bin/env python3
"""Accept only reviewed97 from complete constructor returns + genuine progression."""
import argparse,json,re,shutil,subprocess,time
from pathlib import Path
from state import BINARY,ROOT,load,sha
from accept import validate_snapshot
from reference_completion import compare as compare_completion
from progression import compare as compare_progression
from seams import approved,compare_returns,check_policy_snapshot

def validate_pair(config,pe,old,new,mode):
    assert mode in('completion','progression'),'unknown slice mode'
    old,new=Path(old),Path(new);captured=json.loads((old/'receipt.json').read_text());receipts=json.loads((new/'receipt.json').read_text());assert len(receipts)==1;receipt=receipts[0];native=new/'hello-debug-dll'
    validate_snapshot(config,BINARY/'functions.json',old,native,captured,receipt,ROOT/config['target']['path'],BINARY/'build/native/CVPACK.EXE')
    assert captured['case']==receipt['case']=='hello-debug-dll'and captured['candidates']==['create_symbol_reference']and not captured['unmasked']and not captured['guard_probe']and not captured['stop_at'],'unapproved original capture'
    assert bool(captured.get('reference_completion'))==(mode=='completion')and bool(captured.get('entry_progress_only'))==(mode=='progression'),'capture slice mode mismatch'
    assert bool(receipt.get('reference_completion'))==(mode=='completion'),'native capture slice mode mismatch'
    policy=captured['conditional_support_sha256'];assert receipt['conditional_support_sha256']==policy;check_policy_snapshot(old,policy);check_policy_snapshot(native,policy)
    events=(old/'events').read_text();function=next(r for r in config['functions']if r['name']=='create_symbol_reference')
    seams=approved(config,pe,json.loads((BINARY/'runtime.json').read_text()),'hello-debug-dll');crt=compare_returns(events,json.loads((native/'crt-returns.json').read_text()),seams,function['required_conditional_calls_by_case']['hello-debug-dll'])
    for stream in('stdout','stderr'):
        assert(old/stream).read_bytes()==(native/('debug-'+stream if mode=='completion'else stream)).read_bytes()==b'','slice streams differ'
    work=native/('debug-work'if mode=='completion'else 'work');assert {p.name for p in(old/'work').iterdir()}=={p.name for p in work.iterdir()},'slice artifact sets differ'
    for path in(old/'work').iterdir():assert path.read_bytes()==(work/path.name).read_bytes(),'slice complete artifact bytes differ'
    from inputs import verify
    case=json.loads((BINARY/'cases/hello-debug-dll/case.json').read_text());verify(case,work)
    assert 'exit 99\n'in events,'capture did not stop at intended observation/trap'
    return captured,receipt,crt

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in('completion-original','completion-native','progression-original','progression-native'):parser.add_argument('--'+name,required=True,type=Path)
    parser.add_argument('--check-only',action='store_true');args=parser.parse_args();start=time.perf_counter();config,pe=load();f=next(r for r in config['functions']if r['name']=='create_symbol_reference');assert f['status']=='candidate'and len(config['accepted_order'])==96,'only reviewed97 candidate may use this acceptance mode'
    captures={};crt={}
    for name in('completion','progression'):captures[name],_,crt[name]=validate_pair(config,pe,getattr(args,name+'_original'),getattr(args,name+'_native'),name)
    completion=compare_completion(config,pe,args.completion_original,args.completion_native/'hello-debug-dll')
    native=args.progression_native/'hello-debug-dll';events=(args.progression_original/'events').read_text();state=json.loads((native/'gdb.json').read_text());progression=compare_progression(config,pe,events,state,(native/'trace').read_text().splitlines())
    # Completion prefix consists solely of actual entries. No observer return is
    # an application function or additional coverage.
    apps={r['original']:r for r in config['functions']};prefix=[apps[int(a,16)]['native']for a in re.findall(r'^runtime ([0-9a-f]+) [0-9a-f]+$',(args.completion_original/'events').read_text(),re.M)if int(a,16)in apps]
    assert prefix==progression['entry_trace'][:len(prefix)]and prefix.count('create_symbol_reference')==1,'actual accepted application entry prefix differs'
    runtime=json.loads((BINARY/'runtime.json').read_text());identities={(r['start'],r['end']):r for r in runtime['functions']+runtime['imports']};support={}
    for source in(args.completion_original,args.progression_original):
        for a,b in re.findall(r'^runtime ([0-9a-f]+) ([0-9a-f]+)$',(source/'events').read_text(),re.M):
            a,b=int(a,16),int(b,16)
            if a in apps:assert apps[a]['status']=='accepted'or apps[a]['name']=='create_symbol_reference','unaccepted app range restored'
            else:
                identity=identities[(a,b)];support[(a,b)]=dict(name=identity['name'],start=a,end=b)
    subprocess.run(['python3','scripts/audit.py'],cwd=BINARY,check=True)
    if args.check_only:print('Reviewed97 completion + genuine progression proof passes; not accepted');return
    directory=BINARY/'cases/hello-debug-dll/evidence/0097-create_symbol_reference';directory.mkdir(exist_ok=False)
    for label,source in(('completion-original',args.completion_original),('completion-native',args.completion_native/'hello-debug-dll'),('progression-original',args.progression_original),('progression-native',native)):
        target=directory/label;target.mkdir()
        for name in('receipt.json','events','configuration.json','runtime-support.json','runtime-plan','stdout','stderr','gdb.json','commands.gdb','gdb.stdout','gdb.stderr','debug-stdout','debug-stderr','trace','crt-returns.json','read-returns.json','pdb-read-returns.json','pdb-consumed.json','read-progress.json','symbol-output.json','type-pages.json','buffer-storage.json','resolver-returns.json','recursive-arena.json','packed-arena.json','symbol-blocks.json','module-address.json','symbol-references.json'):
            if(source/name).is_file():shutil.copyfile(source/name,target/name)
        if label.endswith('native'):shutil.copyfile(source.parent/'receipt.json',target/'receipt.json')
    shutil.copyfile(BINARY/'walks.json',directory/'walks.json')
    proof=dict(function=f['name'],case='hello-debug-dll',sequence=97,extent=f['extent'],classification='bounded97-constructor-completion-and-separate-genuine-entry-progression',completion=completion,progression=progression,conditional_CRT_support=crt,application_entries=progression['entry_trace'],later_module_heap_equality=False,policy='Root-approved exact97 only; actual caller-return checkpoint has zero extra application-function/coverage credit. Original source remains normal Win32 C.',runtime_unmasked=list(support.values()),elapsed=time.perf_counter()-start)
    proofpath=str((directory/'proof.json').relative_to(BINARY));(directory/'proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    prior={(r['start'],r['end'])for r in config.get('support_unmasked',[])}
    for interval,record in support.items():
        if interval not in prior:config.setdefault('support_unmasked',[]).append(dict(record,evidence=proofpath))
    f.update(status='accepted',evidence=proofpath);config['accepted_order'].append(dict(function=f['name'],case='hello-debug-dll',kind='application',sequence=97));(BINARY/'functions.json').write_text(json.dumps(config,indent=2)+'\n');print('Accepted97 create_symbol_reference; genuine next begin_symbol_block/module10. No later heap equality claim.')

if __name__=='__main__':main()
