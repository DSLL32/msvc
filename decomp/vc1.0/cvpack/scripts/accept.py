#!/usr/bin/env python3
"""Accept a candidate only after fresh original/native frontier/state evidence."""
import argparse
import json
import re
import shutil
import subprocess
from pathlib import Path
from state import BINARY,load,sha
from objects import original_walks,compare_walks,pointer_identity

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--function',required=True)
    parser.add_argument('--case',required=True)
    parser.add_argument('--original',required=True,type=Path)
    parser.add_argument('--native',required=True,type=Path)
    args=parser.parse_args()
    config,pe=load()
    function=next(r for r in config['functions'] if r['name']==args.function)
    assert function['status']=='candidate'
    original=args.original.resolve()
    native=args.native.resolve()
    captured=json.loads((original/'receipt.json').read_text())
    native_receipt=json.loads((native/'receipt.json').read_text())[0]
    assert captured['case']==native_receipt['case']==args.case
    assert captured['candidates']==[args.function]
    assert native_receipt['executable_sha256']==sha(BINARY/'build/native/CVPACK.EXE')
    events=(original/'events').read_text()
    match=re.search(r'^exception ([0-9a-f]+) va ([0-9a-f]+)',events,re.M)
    completed=match is None
    assert not captured['unmasked'],'acceptance must use a masked original'
    if completed:
        case_record=json.loads((BINARY/'cases'/args.case/'case.json').read_text())
        expected=case_record['complete_reference']
        terminal=re.search(r'^exit (\d+)$',events,re.M)
        assert terminal and int(terminal[1])==expected['exit']==native_receipt['status']
        assert native_receipt['terminal']=='exit'
        address=None;frontier=None
        state=dict(argc=None,argv=[],parameters={})
    else:
        assert int(match[1],16)==0x80000003,'unknown original exception'
        state=json.loads((native/args.case/'gdb.json').read_text())
        address=int(match[2],16)
        frontier=next(r for r in config['functions'] if r['original']==address)
        assert frontier['status']=='stub','frontier is not a new named application function'
        assert state['frames'][0]==frontier['native'] and state['signal']==5
    argv=[bytes.fromhex(line.split()[2]).decode('latin1') for line in events.splitlines() if line.startswith('argv ')]
    if not completed:assert argv==state['argv'] and len(argv)==state['argc'],'argv/state mismatch'
    memory={}
    for line in events.splitlines():
        if line.startswith('memory '):
            _,a,value=line.split()
            start=int(a,16)
            memory.update((start+i,byte)for i,byte in enumerate(bytes.fromhex(value)))
    matched={}
    object_proof={}
    if not completed and (BINARY/'walks.json').exists():
        specifications=json.loads((BINARY/'walks.json').read_text())
        object_proof=compare_walks(specifications,original_walks(events,specifications),state['walks'])
    for item in ([] if completed else json.loads((BINARY/'data.json').read_text())):
        raw=bytes(memory[item['original']+i]for i in range(item['size']))
        actual=state['globals'][item['name']]
        if item.get('kind')=='string-pointer-array':
            expected=[]
            for index in range(item['size']//4):
                pointer=int.from_bytes(raw[index*4:index*4+4],'little')
                value=[]
                while memory[pointer]:value.append(memory[pointer]);pointer+=1
                expected.append(bytes(value).hex())
        elif isinstance(actual,dict):
            expected={'null':int.from_bytes(raw,'little')==0}
        else:expected=raw.hex()
        assert actual==expected,('application state differs',item['name'],expected,actual)
        matched[item['name']]=actual
    words={int(line.split()[1]):int(line.split()[2],16) for line in events.splitlines() if line.startswith('stack ')}
    descriptions=frontier.get('parameters',[]) if frontier else []
    assert {r['name'] for r in descriptions}==set(state['parameters']),'missing ABI parameter specification'
    for item in descriptions:
        value=words[item['slot']]
        actual=state['parameters'][item['name']]
        kind=item['kind']
        if kind=='integer':
            mask=(1<<item.get('bits',32))-1
            assert value&mask==actual&mask,('ABI integer differs',item['name'],value,actual)
        elif kind in ('pointer','cstring'):
            assert actual['null']==(value==0)
            if value:
                context=json.loads((BINARY/'data.json').read_text())
                owners=[dict(name=r['name'],offset=value-r['original']) for r in context if r['original']<=value<r['original']+r['size']]
                if owners:
                    assert len(owners)==1 and actual.get('global')==owners[0],('mismatched global pointer argument',item['name'],value,actual)
                else:
                    old_pointer=pointer_identity(original_walks(events,specifications),value)
                    new_pointer=pointer_identity(state['walks'],actual['address'])
                    assert old_pointer==new_pointer,('mismatched pool pointer argument',item['name'],old_pointer,new_pointer)
                if kind=='cstring':
                    text=[];pointer=value
                    while memory[pointer]:text.append(memory[pointer]);pointer+=1
                    assert bytes(text).hex()==actual['string'],('string argument differs',item['name'])
        elif kind=='argv':assert not actual['null']
        elif kind=='function':
            target=next(r for r in config['functions'] if r['original']==value)
            assert not actual['null'] and actual['function']==target['native']
        else:raise AssertionError('unsupported ABI proof kind: '+kind)
    for stream in ('stdout','stderr'):
        assert (original/stream).read_bytes()==(native/args.case/stream).read_bytes(),stream+' mismatch'
        if completed:
            assert (original/stream).read_bytes()==(BINARY/'cases'/args.case/expected[stream]).read_bytes()
    applications={r['original']:r for r in config['functions']}
    entries=[]
    runtime=json.loads((BINARY/'runtime.json').read_text())
    identities={(r['start'],r['end']):r for r in runtime['functions']+runtime['imports']}
    support=[]
    for line in events.splitlines():
        if not line.startswith('runtime '):continue
        _,a,b=line.split();a,b=int(a,16),int(b,16)
        if a in applications:entries.append(applications[a]['native'])
        else:
            identity=identities[(a,b)]
            support.append(dict(name=identity['name'],start=a,end=b))
    if frontier:entries.append(frontier['native'])
    native_entries=(native/args.case/'trace').read_text().splitlines()
    assert entries==native_entries,('entry trace mismatch',entries,native_entries)
    subprocess.run(['python3',str(BINARY/'scripts/audit.py')],check=True)
    sequence=len(config['accepted_order'])+1
    directory=BINARY/'cases'/args.case/'evidence'/('%04d-%s'%(sequence,args.function))
    directory.mkdir(parents=True,exist_ok=False)
    for name in ('events','receipt.json','stdout','stderr','runtime-plan','configuration.json'):
        shutil.copyfile(original/name,directory/('original-'+name))
    for name in ('gdb.json','gdb.stdout','gdb.stderr','commands.gdb','trace','stdout','stderr'):
        if (native/args.case/name).is_file():shutil.copyfile(native/args.case/name,directory/('native-'+name))
    shutil.copyfile(native/'receipt.json',directory/'native-receipt.json')
    proof_path=str(directory.relative_to(BINARY)/'proof.json')
    prior={(r['start'],r['end'])for r in config.get('support_unmasked',[])}
    for record in support:
        if (record['start'],record['end']) not in prior:
            record['evidence']=proof_path
            config.setdefault('support_unmasked',[]).append(record)
    proof=dict(function=args.function,case=args.case,sequence=sequence,extent=function['extent'],
        original_frontier=address,native_frontier=frontier['native'] if frontier else 'exit',application_entries=entries,
        argc=state['argc'],argv=argv,matched_application_globals=matched,
        matched_frontier_parameters=state['parameters'],runtime_unmasked=support,
        observed_objects=object_proof,
        process_boundary=native_receipt.get('process_boundary'),
        graph_ABI_data='Complete original/compiler CFG/callee/RET ABI and contextual initial data passed',
        seconds=native_receipt['seconds'])
    (directory/'proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    function['status']='accepted';function['evidence']=proof_path
    config['accepted_order'].append(dict(function=args.function,case=args.case,kind='application',sequence=sequence))
    (BINARY/'functions.json').write_text(json.dumps(config,indent=2)+'\n')
    print('Accepted',args.function,'as application function',sequence,'; next',frontier['name'] if frontier else 'case complete')

if __name__=='__main__':main()
