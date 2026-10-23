"""Case-specific reviewed CRT variants; never exact-LIBC or application owners."""
import hashlib
import json
import re
from state import BINARY,ROOT,sha
from graph import graph

def snapshot_policy(directory,path=None):
    path=path or BINARY/'runtime-support.json'
    if not path.exists():return None
    snapshot=directory/'runtime-support.json'
    snapshot.write_bytes(path.read_bytes())
    return sha(snapshot)

def check_policy_snapshot(directory,expected,path=None):
    path=path or BINARY/'runtime-support.json'
    current=sha(path)if path.exists()else None
    saved=directory/'runtime-support.json'
    assert current==expected==(sha(saved)if saved.exists()else None),'conditional support changed during run or capture'


def validate(config,pe,runtime,record):
    assert record['classification']=='reviewed-case-specific-CRT-variant'
    assert record['application_coverage'] is False and record['unconditionally_unmask'] is False
    assert record['case']=='hello-debug-dll' and record['name']=='__sopen'
    assert record['guard']==dict(mode=0x8002,share=0x40,name='hello-debug.dll',every_invocation=True)
    assert record['start']==0x2dcb0 and record['end']==0x2e0a0
    assert record['target_sha256']==config['target']['sha256']
    assert hashlib.sha256(pe.read(record['start'],record['end']-record['start'])).hexdigest()==record['body_sha256'],'changed conditional CRT body'
    proof=BINARY/record['evidence']
    assert proof.is_file() and hashlib.sha256(proof.read_bytes()).hexdigest()==record['evidence_sha256'],'changed conditional CRT evidence'
    analysis=json.loads(proof.read_text())
    cfg=graph(pe,record['start'],record['end'])
    assert cfg['returns']=={0} and set(cfg['edges'])=={r['address']for r in analysis['original_callees']}
    identities={(r['start'],r['name'])for r in runtime['functions']+runtime['imports']}
    assert all((r['address'],r['identity'])in identities for r in analysis['original_callees']),'unidentified conditional CRT callee'
    assert analysis['original_sha256']==record['body_sha256']
    target=ROOT/record['input']['path']
    assert target.stat().st_size==record['input']['bytes']
    assert hashlib.sha256(target.read_bytes()).hexdigest()==record['input']['sha256']
    # The normal ownership validator can check geometry without advertising the
    # variant as an exact-library identity or adding it to any unmask closure.
    from audit import validate_ownership
    ownership=dict(config,support_unmasked=[*config.get('support_unmasked',[]),dict(name=record['name'],start=record['start'],end=record['end'])])
    catalogue=dict(runtime,functions=[*runtime['functions'],dict(name=record['name'],start=record['start'],end=record['end'])])
    validate_ownership(ownership,pe,catalogue,json.loads((BINARY/'data.json').read_text()))
    return record


def approved(config,pe,runtime,case=None):
    path=BINARY/'runtime-support.json'
    records=json.loads(path.read_text())if path.exists()else []
    result=[validate(config,pe,runtime,record)for record in records]
    return [r for r in result if case is None or r['case']==case]


def compare_returns(events,native,records):
    expected={r['start']:r for r in records}
    original=[]
    current=None
    for line in events.splitlines():
        if not line.startswith('crt-'):continue
        fields=line.split()
        operation,address=fields[0],int(fields[1],16)
        assert address in expected,'unapproved conditional runtime entry'
        policy=expected[address]
        if operation=='crt-entry':
            assert current is None,'nested conditional runtime entry'
            mode,share,permission=(int(value,16)for value in fields[2:5])
            name=bytes.fromhex(fields[5]).decode('latin1')
            assert dict(mode=mode,share=share,name=name,every_invocation=True)==policy['guard']
            current=dict(mode=mode,share=share,name=name,permission=permission,stage='entry')
        elif operation=='crt-restored':
            assert current and current['stage']=='entry' and int(fields[2],16)==policy['end']
            current['stage']='restored'
        elif operation=='crt-rearm':
            assert current and current['stage']=='restored' and int(fields[2],16)==address+3
            current['stage']='rearmed'
        elif operation=='crt-return':
            assert current and current['stage']=='rearmed'
            match=re.fullmatch(r'crt-return [0-9a-f]+ fd (\d+) errno (\d+) osfile ([0-9a-f]+) type (\d+) position (\d+) owned (\d+) input (\d+)',line)
            assert match,'invalid conditional runtime return proof'
            current['return']=dict(descriptor=int(match[1]),errno=int(match[2]),osfile=int(match[3],16),type=int(match[4]),position=int(match[5]),owned=bool(int(match[6])),input_matches=bool(int(match[7])))
            del current['stage']
            original.append(current);current=None
        else:raise AssertionError('rejected/unknown conditional runtime event: '+line)
    assert current is None and len(original)==len(native),'incomplete conditional runtime proof'
    proof=[]
    for old,new in zip(original,native):
        assert not new.get('error'),new
        assert all(old[key]==new[key]for key in ('name','mode','share'))
        assert old['return']=={key:new['return'][key]for key in old['return']},('conditional CRT return state differs',old,new)
        assert old['return']['osfile']==1 and old['return']['type']==1 and old['return']['position']==0 and old['return']['owned'] and old['return']['input_matches']
        policy=next(r for r in records if r['guard']['name']==old['name'] and r['guard']['mode']==old['mode'] and r['guard']['share']==old['share'])
        assert new['return']['input_sha256']==policy['input']['sha256'],'conditional CRT input hash differs'
        proof.append(dict(original=old,native=new,permission='inactive for mode0x8002; both raw words retained; original actual-body proof in cases/abi/open-permission.json',descriptor_identity='logical CRT fd; host HANDLE/fd values mapped to owned case input, never numerically compared'))
    return proof


def check_guard_references():
    """Check explicitly captured original guard probes; never launch a process."""
    base=BINARY/'cases/runtime-variants/sopen-guard-tests'
    manifest=json.loads((base/'manifest.json').read_text())
    assert sha(BINARY/manifest['source']['path'])==manifest['source']['sha256']
    assert guard_source_digest((BINARY/'src/debug-run.c').read_text())==manifest['guard_source']['sha256'],'guard implementation changed since original controls'
    for record in manifest['cases']:
        directory=base/record['name']
        receipt=json.loads((directory/'receipt.json').read_text())
        assert receipt['guard_probe']==record['name']
        assert receipt['helper_status']==record['helper_status']
        assert sha(directory/'configuration.json')==receipt['configuration_sha256']
        assert sha(directory/'runtime-support.json')==receipt['conditional_support_sha256']
        lines=[line for line in (directory/'events').read_text().splitlines()if line.startswith('crt-')]
        expected=['crt-entry','crt-restored','crt-rearm','crt-return']*record['restored']
        if record['denied']:expected+=['crt-entry','crt-denied']
        assert [line.split()[0]for line in lines]==expected,'unsupported body executed after guard denial'
        entries=[line.split()for line in lines if line.startswith('crt-entry ')]
        for entry in entries[:record['restored']]:
            assert entry[1:4]==['0002dcb0','00008002','00000040']
            assert bytes.fromhex(entry[5])==b'hello-debug.dll'
        if record['denied']:
            assert lines[-1]=='crt-denied 0002dcb0 '+record['denied']
            assert entries[-1][2 if record['denied']=='mode'else 3]!=('00008002'if record['denied']=='mode'else '00000040')
            print('rejected: original guard '+record['name']+' before unsupported body execution')
        else:
            assert (directory/'events').read_text().splitlines()[-1]=='exit 3'
            print('verified: original guard rearmed across two successful calls')


def guard_source_digest(text):
    parts=[]
    for name in ('identical_files','denied_sopen','begin_sopen','rearm_sopen','finish_sopen'):
        start=text.index('static int '+name+'(')
        end=text.index('\n}\n',start)+3
        parts.append(name+'\0'+text[start:end])
    return hashlib.sha256('\0'.join(parts).encode()).hexdigest()
