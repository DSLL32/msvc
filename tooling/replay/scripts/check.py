"""Exercise real Wine execution, terminal modes and deliberate divergences."""
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import time

HERE=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(HERE))
import fast_replay as replay
import build as builder


class PE:
    def __init__(self,path):
        self.data=Path(path).read_bytes()
        pe=struct.unpack_from('<I',self.data,60)[0]
        optional=pe+24
        self.base=struct.unpack_from('<I',self.data,optional+28)[0]
        count=struct.unpack_from('<H',self.data,pe+6)[0]
        size=struct.unpack_from('<H',self.data,pe+20)[0]
        self.sections=[]
        for index in range(count):
            at=optional+size+40*index
            vs,rva,raw,off=struct.unpack_from('<4I',self.data,at+8)
            self.sections.append((rva,raw,off))

    def offset(self,address):
        for rva,size,off in self.sections:
            if rva<=address-self.base<rva+size:
                return off+address-self.base-rva
        raise ValueError('unbacked test address')

    def read(self,address,size):
        at=self.offset(address)
        return self.data[at:at+size]


def mask(pe,records,accepted,path):
    # Limit this test's frontier instrumentation to its two controlled leaves.
    # Production adapters mask whole code sections through their audited mask().
    data=bytearray(pe.data)
    for record in records:
        if record['name'] not in accepted:
            a,b=pe.offset(record['start']),pe.offset(record['end']-1)+1
            data[a:b]=b'\xcc'*(b-a)
    Path(path).write_bytes(data)


def main():
    walker=replay.build_helper()
    env=builder.environment()
    command=['wine',str(builder.TC/'bin/CL.EXE'),'/nologo','/Od','/W3',
             '/Fobuild/check-target.obj','/Febuild/check-target.exe',
             'src/check-target.c','/link','/map:build/check-target.map']
    subprocess.run(command,cwd=HERE,env=env,check=True)
    pe=PE(HERE/'build/check-target.exe')
    names={}
    for line in (HERE/'build/check-target.map').read_text().splitlines():
        match=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
        if match:names[match[1]]=int(match[2],16)
    records=[]
    for name in ('phase_a','phase_b'):
        start=names['_'+name]
        end=min(a for a in names.values() if a>start)
        records.append(dict(name=name,start=start,end=end))
    root=Path.home()/('.ai-tmp/vc1.0-fast-replay/check-%d-%d'%(os.getpid(),time.time_ns()))
    root.mkdir(parents=True)
    results=[]

    def prepare(label,mode='',first=0,last=2,terminal='exit:0',accepted=None,timeout=10):
        directory=root/label
        directory.mkdir()
        return replay.prepare_side(directory=directory,pe=pe,records=records,
            accepted=accepted or ['phase_a','phase_b'],first=first,last=last,
            terminal=terminal,cwd=replay.windows(directory),
            environment={'REPLAY_TEST_MODE':mode},wine_env=env,mask_image=mask,
            timeout=timeout,frozen_paths=[HERE/'build/check-target.exe'])

    def run(label,expected_failure=None,**kwargs):
        side=prepare(label,**kwargs)
        try:
            receipt=replay.run_side(side,walker)
        except RuntimeError:
            if not expected_failure:raise
            events=[json.loads(line) for line in side.receipt.read_text().splitlines()]
            failure=next(e for e in events if e['event']=='failure')
            assert expected_failure in failure['reason'],failure
            assert events[-1]['event']=='failure'
            assert 'registers' in failure
            results.append(dict(test=label,result='rejected',reason=failure['reason']))
        else:
            assert expected_failure is None,label
            results.append(dict(test=label,result='pass',terminal=receipt['terminal']))
        return side

    normal=run('normal')
    assert normal.stdout.read_bytes()==b'ok\r\n' and not normal.stderr.read_bytes()
    run('prefix',first=1)
    run('zero-step',first=2)
    run('terminal-frontier',last=1,terminal='phase_b')
    run('stop-before-first',last=0,terminal='phase_a')
    run('wrong-entry',accepted=['phase_b','phase_a'],expected_failure='wrong frontier')
    run('early-exit',mode='early',expected_failure='premature')
    run('exception',mode='fault',expected_failure='unexpected target exception')
    run('thread',mode='thread',expected_failure='unsupported application thread')
    run('child',mode='child',expected_failure='unsupported child process')
    run('timeout',mode='wait',timeout=1,expected_failure='timeout')
    run('wrong-exit',terminal='exit:7',expected_failure='wrong target exit')
    bad=prepare('bad-restore')
    data=bytearray(bad.plan.read_bytes())
    offset=28
    for unused in range(6):
        length=struct.unpack_from('<I',data,offset)[0];offset+=4+length
    def skip_record(at):
        length=struct.unpack_from('<I',data,at)[0];at+=4+length
        entries,chunks=struct.unpack_from('<II',data,at);at+=8+4*entries
        for unused in range(chunks):
            rva,size=struct.unpack_from('<II',data,at);at+=8+size
        return at
    offset=skip_record(offset)
    length=struct.unpack_from('<I',data,offset)[0];offset+=4+length
    entries,chunks=struct.unpack_from('<II',data,offset);offset+=8+4*entries
    assert chunks
    # A corrupted restore payload produces a real illegal-instruction exception.
    rva,size=struct.unpack_from('<II',data,offset);assert size>=2
    data[offset+8:offset+10]=b'\x0f\x0b'
    bad.plan.write_bytes(data)
    bad.frozen[str(bad.plan)]=replay.sha256(bad.plan)
    try:replay.run_side(bad,walker)
    except RuntimeError:
        event=json.loads(bad.receipt.read_text().splitlines()[-1])
        assert event['event']=='failure' and event['reason']=='unexpected target exception'
        results.append(dict(test='bad-restore',result='rejected'))
    else:raise AssertionError('corrupt restore accepted')
    altered=run('bad-output',mode='bad-output')
    try:assert altered.stdout.read_bytes()==b'ok\r\n'
    except AssertionError:results.append(dict(test='final-output-oracle',result='rejected'))
    else:raise AssertionError('wrong concrete output accepted')
    changed=prepare('changed-frozen-plan')
    changed.plan.write_bytes(changed.plan.read_bytes()+b'X')
    try:replay.run_side(changed,walker)
    except RuntimeError as error:
        assert 'frozen' in str(error)
        results.append(dict(test='changed-frozen-plan',result='rejected-before-launch'))
    else:raise AssertionError('changed plan accepted')
    left,right=prepare('paired-left'),prepare('paired-right')
    report=replay.run_pair(left,right)
    assert report['frontiers']==2
    results.append(dict(test='paired',result='pass'))
    (root/'results.json').write_text(json.dumps(results,indent=2)+'\n')
    print('Shared replay checks passed:',len(results),'Evidence:',root/'results.json')


if __name__=='__main__':main()
