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
    assert record['case']in('hello-debug-dll','dos-codeview16') and record['name']=='__sopen'
    guard=record['guard']
    if record['case']=='dos-codeview16':
        assert guard==dict(mode=0x8002,share=0x40,name='dos-cv.exe',every_invocation=True)
        assert record['phase']==1 and record['input']==dict(path='decomp/vc1.0/cvpack/cases/dos-codeview16/input/dos-cv.exe',name='dos-cv.exe',bytes=5577,sha256='5dd0e8f3fc09ad257ea86b7dcaf2112b50d0d939c8f2a249f8f78bf3f7633c47'),'unapproved DOS input/phase'
        access=0xc0000000
    elif guard['name']=='hello-debug.dll':
        assert guard==dict(mode=0x8002,share=0x40,name='hello-debug.dll',every_invocation=True)
        assert record['phase']in(1,3),'unapproved DLL open phase'
        if record['phase']==1:assert record['input']==dict(path='decomp/vc1.0/cvpack/cases/hello-debug-dll/input/hello-debug.dll',name='hello-debug.dll',bytes=2097152,sha256='81a603fcfffc6229f408dc056a54441d3806b09c8526cab8b3a1b5de74959036'),'unapproved initial DLL input'
        if record['phase']==3:assert record['input']==dict(path='decomp/vc1.0/cvpack/cases/runtime-variants/checksum-input.dll',name='hello-debug.dll',bytes=71800,sha256='e08a1752ab0990c401b98c889ddf5ffc0d071ea9fb5865025cf415bbe1e52c1b'),'unapproved postcleanup input'
        access=0xc0000000
    else:
        proof=json.loads((BINARY/'cases/abi/pdb-input.json').read_text())['inputs'][0]
        assert guard==dict(mode=0x8000,share=0x40,name=proof['windows'],every_invocation=True),'unapproved PDB guard'
        assert record['input']==dict(path='decomp/vc1.0/cvpack/'+proof['owned'],name=proof['windows'],bytes=proof['bytes'],sha256=proof['sha256'])
        assert record['phase']==2,'PDB is only the second open phase'
        access=0x80000000
    assert record['access']==access and record['guard_version']==2,'conditional access policy differs'
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
    assert [r['phase']for r in result if r['case']=='hello-debug-dll']==[1,2,3],'conditional DLL phase inventory/order differs'
    assert [r['phase']for r in result if r['case']=='dos-codeview16']==[1],'conditional DOS phase inventory/order differs'
    return [r for r in result if case is None or r['case']==case]


def compare_returns(events,native,records,required=None):
    original=[];current=None;policy=None
    for line in events.splitlines():
        if not line.startswith('crt-'):continue
        fields=line.split();operation,address=fields[0],int(fields[1],16)
        if operation=='crt-entry':
            assert current is None,'nested conditional runtime entry'
            mode,share,permission=(int(value,16)for value in fields[2:5])
            name=bytes.fromhex(fields[5]).decode('latin1')
            matches=[r for r in records if r['phase']==len(original)+1 and r['start']==address and r['guard']==dict(mode=mode,share=share,name=name,every_invocation=True)]
            assert len(matches)==1,'unapproved conditional runtime invocation'
            policy=matches[0]
            current=dict(mode=mode,share=share,name=name,permission=permission,stage='entry')
        elif operation=='crt-restored':
            assert current and current['stage']=='entry' and int(fields[2],16)==policy['end']
            current['stage']='restored'
        elif operation=='crt-rearm':
            assert current and current['stage']=='restored' and int(fields[2],16)==address+3
            current['stage']='rearmed'
        elif operation=='crt-createfile':
            assert current and current['stage']=='rearmed'
            match=re.fullmatch(r'crt-createfile [0-9a-f]+ access ([0-9a-f]+) share (\d+) disposition (\d+) attrs (\d+) inherit (\d+) name ([0-9a-f]+)',line)
            assert match,'invalid actual CreateFile argument proof'
            call=dict(access=int(match[1],16),share=int(match[2]),disposition=int(match[3]),attrs=int(match[4]),inherit=bool(int(match[5])),name=bytes.fromhex(match[6]).decode('latin1'))
            assert call==dict(access=policy['access'],share=3,disposition=3,attrs=128,inherit=True,name=current['name']),'original actual CreateFile args differ'
            current['createfile']=call;current['stage']='createfile'
        elif operation=='crt-createfile-rearm':
            assert current and current['stage']=='createfile' and fields[2]=='0002be52'
            current['stage']='createfile-rearmed'
        elif operation=='crt-return':
            assert current and current['stage']=='createfile-rearmed'
            match=re.fullmatch(r'crt-return [0-9a-f]+ fd (\d+) errno (\d+) osfile ([0-9a-f]+) type (\d+) position (\d+) owned (\d+) input (\d+) last_error (\d+)',line)
            assert match,'invalid conditional runtime return proof'
            current['return']=dict(descriptor=int(match[1]),errno=int(match[2]),osfile=int(match[3],16),type=int(match[4]),position=int(match[5]),owned=bool(int(match[6])),input_matches=bool(int(match[7])),last_error=int(match[8]))
            del current['stage'];original.append(current);current=None
        else:raise AssertionError('rejected/unknown conditional runtime event: '+line)
    assert current is None and len(original)==len(native),'incomplete conditional runtime proof'
    if required is not None:
        assert [{key:r[key]for key in ('name','mode','share')}for r in original]==required,'mandatory conditional CRT invocation inventory differs'
    proof=[]
    for phase,(old,new)in enumerate(zip(original,native),1):
        assert not new.get('error'),new
        assert all(old[key]==new[key]for key in ('name','mode','share'))
        assert old['return']=={key:new['return'][key]for key in old['return']},('conditional CRT return state differs',old,new)
        assert old['return']['osfile']==1 and old['return']['type']==1 and old['return']['position']==0 and old['return']['owned'] and old['return']['input_matches']
        policy=next(r for r in records if r['phase']==phase and r['guard']['name']==old['name'] and r['guard']['mode']==old['mode'] and r['guard']['share']==old['share'])
        call=dict(old['createfile']);call['inherit']=False
        assert new['createfile']==call,'native actual CreateFile args differ'
        assert new['return']['access_mode']==(0 if old['mode']==0x8000 else 2),'native file access differs'
        assert new['return']['input_sha256']==policy['input']['sha256'],'conditional CRT input hash differs'
        proof.append(dict(original=old,native=new,permission='inactive with exact binary existing modes8000/8002; both raw words retained',descriptor_identity='logical CRT fd; host HANDLE/fd mapped to full owned case input',inheritance='originalTRUE/nativeFALSE is retained; approved singleprocess/nochild scope only'))
    return proof


def check_guard_references():
    """Check explicitly captured original guard probes; never launch a process."""
    base=BINARY/'cases/runtime-variants/sopen-guard-tests'
    manifest=json.loads((base/'manifest.json').read_text())
    assert sha(BINARY/manifest['source']['path'])==manifest['source']['sha256']
    previous=BINARY/'cases/runtime-variants/sopen-guard-tests-v2/previous-guard.c'
    assert guard_source_digest(previous.read_text(),version=1)==manifest['guard_source']['sha256'],'historical guard source differs'
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

    current=BINARY/'cases/runtime-variants/sopen-guard-tests-v2'
    manifest=json.loads((current/'manifest.json').read_text())
    assert manifest['guard_sha256']==guard_source_digest((BINARY/'scripts/debug-run.c').read_text()),'current guard changed since explicit controls'
    for source in manifest['source']:assert sha(BINARY/source['path'])==source['sha256'],'guard controller source changed'
    pdb=json.loads((BINARY/'cases/abi/pdb-input.json').read_text())['inputs'][0]
    for item in manifest['cases']:
        directory=current/item['name'];receipt=json.loads((directory/'receipt.json').read_text())
        assert receipt['guard_probe']==item['name']and receipt['helper_status']==item['helper_status']
        assert sha(directory/'configuration.json')==receipt['configuration_sha256']and sha(directory/'runtime-support.json')==receipt['conditional_support_sha256']
        lines=[line for line in (directory/'events').read_text().splitlines()if line.startswith('crt-')]
        expected=['crt-entry','crt-restored','crt-rearm','crt-createfile','crt-createfile-rearm','crt-return']*item['restored']
        if item['denied']:expected+=['crt-entry','crt-denied']
        assert [line.split()[0]for line in lines]==expected,'guard restored unsupported body after denial'
        entries=[line.split()for line in lines if line.startswith('crt-entry ')]
        mode=0x8000 if item['name'].startswith('pdb-')else 0x8002
        name=pdb['windows']if mode==0x8000 else'hello-debug.dll'
        for entry in entries[:item['restored']]:assert entry[2:4]==['%08x'%mode,'00000040']and bytes.fromhex(entry[5]).decode('latin1')==name
        if item['denied']:
            assert lines[-1]=='crt-denied 0002dcb0 '+item['denied']
            print('rejected: current guard '+item['name']+' before unsupported body execution')
        else:assert (directory/'events').read_text().splitlines()[-1]=='exit 3'


def guard_source_digest(text,version=2):
    parts=[]
    names=['identical_files','denied_sopen','begin_sopen','rearm_sopen','finish_sopen']
    if version==2:names+=['begin_sopen_create','rearm_sopen_create']
    for name in names:
        start=text.index('static int '+name+'(')
        end=text.index('\n}\n',start)+3
        parts.append(name+'\0'+text[start:end])
    if version==2:
        a=text.index('static DWORD sopen_start');b=text.index('static int identical_files',a);parts.append(text[a:b])
        a=text.index('    if (GetEnvironmentVariableA("CVPACK_CAPTURE_SOPEN_GUARD"');b=text.index('    if (GetEnvironmentVariableA("CVPACK_CAPTURE_READS"',a);parts.append(text[a:b])
    return hashlib.sha256('\0'.join(parts).encode()).hexdigest()
