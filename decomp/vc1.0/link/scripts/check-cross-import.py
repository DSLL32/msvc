#!/usr/bin/env python3
"""Check cross-machine archive evidence, raw output flows and native denials."""
import argparse
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time
from case_paths import PROGRAM,LIBRARY,SCRATCH,native_work
import native_clock

BINARY=Path(__file__).resolve().parents[1]
BASE=BINARY/'cases/cross-machine-imports'
def load_module(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module
parser=load_module('cross_import_artifacts',BINARY/'scripts/artifacts-cross-import.py')
def digest(data):return hashlib.sha256(data).hexdigest()


def inspect(data,machine):
    members=parser.archive_members(data)
    objects=[data[m['offset']:m['offset']+m['bytes']] for m in members if m['name'] not in ('/','//')]
    infos=[parser.coff_info(obj) for obj in objects]
    assert all(info['machine']=={'i860':0x14d,'mips':0x162}[machine] for info in infos)
    names={name for info in infos for name in info['symbols']}
    assert {'Ping','Worker','Shared','__imp_Ping','__imp_Worker','__imp_Shared'}<=names
    assert all(section['name']!='.text' for info in infos for section in info['sections']), 'No function thunks claimed'
    helpers=[(obj,section) for obj,info in zip(objects,infos) for section in info['sections'] if section['name']=='.text$5']
    if machine=='i860':
        assert len(helpers)==1
        obj,section=helpers[0]
        assert obj[section['offset']:section['offset']+section['bytes']]==struct.pack('<4I',0x17fe0005,0x17dd0001,0x4000e800,0xa0000000)
        assert 'cross_THUNK_ROUTINE' in names
    else:assert not helpers
    exported=next((obj,info) for obj,info in zip(objects,infos) if '.edata' in [s['name'] for s in info['sections']])
    obj,info=exported;section=next(s for s in info['sections'] if s['name']=='.edata')
    raw=obj[section['offset']:section['offset']+section['bytes']]
    fields=struct.unpack_from('<IIHHIIIIIII',raw)
    assert fields[5:8]==(1,12,3)
    names_offset,ordinals_offset=fields[9:11]
    actual={}
    for i in range(fields[7]):
        offset=struct.unpack_from('<I',raw,names_offset+4*i)[0]
        name=raw[offset:raw.index(0,offset)].decode('ascii')
        actual[name]=fields[5]+struct.unpack_from('<H',raw,ordinals_offset+2*i)[0]
    assert actual=={'Ping':1,'Worker':7,'Shared':12}
    for info in infos:
        for section in info['sections']:
            for address,symbol,kind in section['relocations']:
                assert kind in ({7} if machine=='i860' else {0x22}), (machine,kind)
    return dict(member_count=len(members),object_count=len(objects),helper_members=len(helpers),symbol_names=sorted(names),export_ordinals=actual)


def flows(receipt):
    records={r['case']:r for r in receipt['cases']}
    count=0
    for machine in ('i860','mips'):
        creation='import-archive-'+machine+'-data'
        assert records[creation]['status']=='pass'
        for action,number in [('list',1),('dump',2)]:
            case='import-'+action+'-'+machine+'-data';row=records[case]
            assert row['status']=='pass'
            observations=row['native_input_flow'];assert len(observations)==number
            for flow in observations:
                assert flow['source_case']==creation
                output=next(r for r in records[creation]['outputs'] if r['name']==flow['output_name'])
                assert flow['sha256']==output['sha256'] and flow['bytes']==output['bytes']
                assert flow['source']=='actual preceding native output; no frozen input replacement'
                count+=1
    return count


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--native-receipt',type=Path)
    ap.add_argument('--run-denials',action='store_true')
    ap.add_argument('--output',type=Path)
    args=ap.parse_args();start=time.perf_counter();checks={};negatives=[]
    for machine in ('i860','mips'):
        clock=BASE/'clock'/(machine+'-data');proof_path=clock/'proof.json';raw=proof_path.read_bytes()
        record=dict(path=str(proof_path.relative_to(BINARY)),bytes=len(raw),sha256=digest(raw))
        native_clock.validate(record,'import-archive-'+machine+'-data')
        data=(clock/(machine+'.lib')).read_bytes();checks[machine]=inspect(data,machine)
        # A late digit and the static suffix/padding are not interchangeable.
        member=parser.archive_members(data)[0];offset=member['header']+26
        changed=bytearray(data);changed[offset]^=1
        try:parser.timestamp_fields(bytes(changed),'archive')
        except AssertionError:negatives.append(machine+'-archive-static-date-suffix')
        else:raise AssertionError('Changed static date suffix accepted')
        bad=dict(record,sha256='0'*64)
        try:native_clock.validate(bad,'import-archive-'+machine+'-data')
        except AssertionError:negatives.append(machine+'-changed-clock-proof')
        else:raise AssertionError('Changed captured clock evidence accepted')
        env={'RECON_FIXED_TIME':'0'}
        try:native_clock.install({},'import-archive-'+machine+'-data',env)
        except AssertionError:negatives.append(machine+'-unregistered-clock')
        else:raise AssertionError('Unregistered clock accepted')
    state=json.loads((BASE/'acceptance/931-write_helper_thunk_member/receipt.json').read_text())['import_machine_state']
    def machine_state(value):
        assert value['status']=='pass' and value['bytes']==36
        original,native=value['original'],value['native']
        assert original['bytes']==native['bytes']==36
        assert native['machine_is_i860'] and native['image_is_export_image']
        aliases={r['name']:r['address'] for r in json.loads((BINARY/'data-symbols.json').read_text())}
        assert original['argument']==aliases['import_i860']
        left,right=original['words'][:],native['words'][:]
        assert len(left)==len(right)==9
        for index,name in [(3,'import_jump_i860'),(5,'import_relocations_i860'),(7,'import_helper_i860')]:
            assert left[index]==aliases[name] and right[index]==native['references'][name]
            left[index]=right[index]=name
        assert left==right==[0x70007,0xb0007,10,'import_jump_i860',12,'import_relocations_i860',2,'import_helper_i860',16]
    machine_state(state);checks['original_native_machine_bytes']=36
    for label in ['old-layout','wrong-machine','changed-count','changed-helper-pointer','both-empty']:
        value=copy.deepcopy(state)
        if label=='old-layout':value['bytes']=40
        elif label=='wrong-machine':value['native']['machine_is_i860']=False
        elif label=='changed-count':value['native']['words'][6]=1
        elif label=='changed-helper-pointer':value['native']['words'][7]=0
        else:value['native']['words']=value['original']['words']=[]
        try:machine_state(value)
        except AssertionError:negatives.append(label)
        else:raise AssertionError('Invalid machine layout/state accepted: '+label)
    if args.native_receipt:
        receipt=json.loads(args.native_receipt.read_text());checks['actual_native_flows']=flows(receipt)
        assert checks['actual_native_flows']==6
        for machine in ('i860','mips'):
            case='import-archive-'+machine+'-data';record=next(c for c in receipt['cases'] if c['case']==case)
            work=next((Path(receipt['directory'])/case).rglob(machine+'.lib')).parent
            for ext in ('lib','exp'):
                actual=(work/(machine+'.'+ext)).read_bytes();expected=(BASE/'clock'/(machine+'-data')/(machine+'.'+ext)).read_bytes()
                assert actual==expected, 'Captured external clock must produce exact raw output'
            inspect((work/(machine+'.lib')).read_bytes(),machine)
        for label in ('missing-flow','changed-flow-source','changed-flow-bytes'):
            changed=copy.deepcopy(receipt);row=next(r for r in changed['cases'] if r['case']=='import-dump-i860-data')
            if label=='missing-flow':row['native_input_flow']=[]
            elif label=='changed-flow-source':row['native_input_flow'][0]['source_case']='import-archive-mips-data'
            else:row['native_input_flow'][0]['sha256']='0'*64
            try:flows(changed)
            except AssertionError:negatives.append(label)
            else:raise AssertionError('Incorrect actual native artifact flow accepted: '+label)
    denials=[]
    if args.run_denials:
        directory=Path(tempfile.mkdtemp(prefix='cross-import-denials-',dir=SCRATCH/'vc1.0/link/native'))
        for machine in ('i860','mips'):
            case='unsupported-buffered-function-'+machine
            canonical='S:\\vc1.0\\link\\'+case;work,environment=native_work(directory/case,canonical)
            shutil.copyfile(BASE/'inputs/cross.def',work/'cross.def')
            env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
            for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK_REPRO']:env.pop(key,None)
            env.update(environment,RECON_PROGRAM_PATH=PROGRAM,LIB=LIBRARY,LINK32_DB='27')
            before=time.perf_counter()
            spec=json.loads((BASE/(machine+'-spec.json')).read_text())
            result=subprocess.run([PROGRAM,*spec['arguments']],executable=BINARY/'build/native/LINK.EXE',cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=10)
            assert result.returncode==125
            assert b'LINK buffered function-export auxiliary leaks original heap pointer' in result.stderr
            (work/'stdout.bin').write_bytes(result.stdout);(work/'stderr.bin').write_bytes(result.stderr)
            denials.append(dict(case=case,status='unsupported-native125; original succeeds with leaked pointer bytes',exit=result.returncode,seconds=time.perf_counter()-before,directory=str(work)))
    report=dict(status='pass',evidence=checks,negative_controls=negatives,native_unsupported_denials=denials,seconds=time.perf_counter()-start,clock='Actual captured external time, no output mutation',limits='No cross-machine image execution or function-export auxiliary conformance')
    if args.output:args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
