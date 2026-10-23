"""Host-only mutation checks for the shared CFG and retained data policies.

Mutations use disposable copies, never the original or canonical image. This is
an auditor test, not reconstruction evidence or an original-executable replay.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
from binary import PE
from case_paths import BINARY, CAPTURE_ROOT
from graph_metadata import metadata
from win32_indexes import publics

spec=importlib.util.spec_from_file_location('c1xx_audit', BINARY/'scripts/audit-graph.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    frozen=[BINARY/'functions.json', BINARY/'runtime.json', BINARY/'build/runtime.json',
            BINARY/'build/c1xx.exe', BINARY/'build/c1xx.map', audit.TOOL,
            BINARY.parents[2]/'toolchains/1.0-nt/bin/C1XX3232.EXE']
    hashes={str(path):sha(path) for path in frozen}
    cfg=json.loads((BINARY/'functions.json').read_text())
    names=publics();image=PE(BINARY/'build/c1xx.exe')
    CAPTURE_ROOT.mkdir(parents=True,exist_ok=True)
    records=[]
    started=time.monotonic()
    with tempfile.TemporaryDirectory(prefix='graph-negative-',dir=CAPTURE_ROOT) as scratch:
        directory=Path(scratch)
        def run(label, additions=(), success=True, contains=None, invocation=None, cwd=BINARY):
            command=list(invocation or audit.command())+list(additions)
            begin=time.monotonic()
            result=subprocess.run(command,cwd=cwd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            output=result.stdout.decode(errors='replace');error=result.stderr.decode(errors='replace')
            record=dict(label=label,command=command,status=result.returncode,
                seconds=time.monotonic()-begin,stdout=output,stderr=error)
            records.append(record)
            assert (result.returncode==0)==success,record
            if contains:assert contains.lower() in (output+error).lower(),record
            return result
        run('canonical-cfg-positive')
        # Exercise exactly the candidate adapter used by acceptance, with the
        # last accepted source body proposed against its frozen earlier prefix.
        # A runtime seam may be the final acceptance and has no driver body.
        proposal=json.loads(json.dumps(cfg));drivers={r['name'] for r in cfg['driver']}
        index=max(i for i,name in enumerate(cfg['accepted']) if name in drivers)
        candidate=cfg['accepted'][index];proposal['accepted']=cfg['accepted'][:index]
        path=directory/'candidate-config.json';path.write_text(json.dumps(proposal))
        begin=time.monotonic();checked=audit.verify_candidate(candidate,path)
        records.append(dict(label='candidate-adapter-positive',seconds=time.monotonic()-begin,
            candidate=candidate,status=0,tool_sha256=checked['tool_sha256']))

        def mutated(label, data):
            path=directory/(label+'.exe');path.write_bytes(data);return path
        # Locate the sole already-proved call by target, not by instruction
        # heuristics. The full method_alias body and ABI are independently
        # recovered; these tests only alter that known call/RET boundary.
        begin=names['_method_alias@4'];body=image.read(begin,32)
        offsets=[i for i in range(len(body)-4) if body[i]==0xe8 and
            begin+i+5+struct.unpack_from('<i',body,i+1)[0]==names['_find_hash_value@8']]
        assert len(offsets)==1
        call=begin+offsets[0]
        changed=bytearray(image.data)
        struct.pack_into('<i',changed,image.offset(call)+1,names['_set_hash_value@12']-call-5)
        path=mutated('call-edge',changed)
        run('reject-changed-call-edge',['--rebuilt',str(path)],False,'method_alias')
        assert body.count(b'\xc2\x04\x00')==1
        changed=bytearray(image.data);changed[image.offset(begin+body.index(b'\xc2\x04\x00'))+1]=8
        path=mutated('ret-cleanup',changed)
        run('reject-changed-ret-cleanup',['--rebuilt',str(path)],False,'ret')
        changed_cfg=json.loads(json.dumps(cfg))
        record=next(r for r in changed_cfg['driver'] if r['name']=='_method_alias@4')
        assert record['start']==0x561c0 and record['end']==0x561d3
        record['end']=0x561d1
        path=directory/'split-ret.json';path.write_text(json.dumps(changed_cfg))
        result=run('reject-split-ret-extent',['--config',str(path)],False,'561d0')
        assert b'instruction' in result.stderr, 'split extent did not identify the offending instruction'
        table=next(t for t in cfg['tables'] if t['name']=='_hash_functions')
        address=names[table['name']];offset=image.offset(address)
        values=list(struct.unpack('<%dI'%len(table['functions']),image.read(address,4*len(table['functions']))))
        assert values[-1]==0 and all(values[:-1]) and values[0]!=values[1]
        changed=bytearray(image.data)
        struct.pack_into('<II',changed,offset,values[1],values[0])
        path=mutated('callback-order',changed)
        run('reject-reordered-callbacks',['--rebuilt',str(path)],False,'callback')
        # The shared policy deliberately compares non-null callback order.
        # Exact sentinel position is retained in C1XX's independent data audit.
        changed=bytearray(image.data)
        struct.pack_into('<%dI'%len(values),changed,offset,*([0]+values[:-1]))
        path=mutated('sentinel-position',changed)
        run('nonnull-callback-policy-positive',['--rebuilt',str(path)])
        snapshot=directory/'data-policy';(snapshot/'build').mkdir(parents=True)
        for name in ['functions.json','runtime.json']:(snapshot/name).symlink_to(BINARY/name)
        for name in ['c1xx.map','runtime.json']:(snapshot/'build'/name).symlink_to(BINARY/'build'/name)
        (snapshot/'build/c1xx.exe').symlink_to(path)
        run('reject-moved-callback-sentinel',success=False,contains='callback identity',
            invocation=[sys.executable,str(BINARY/'scripts/audit-data.py')],cwd=snapshot)
        # The encountered lexical property is a logical membership table,
        # independently checked against the original property's 0x40 bit.
        table=next(t for t in cfg['tables'] if t['kind']=='token_membership')
        changed=bytearray(image.data);offset=image.offset(names[table['name']])
        assert changed[offset]==0x3a
        changed[offset]=2
        path=mutated('operator-membership',changed)
        (snapshot/'build/c1xx.exe').unlink();(snapshot/'build/c1xx.exe').symlink_to(path)
        run('reject-changed-operator-membership',success=False,contains='lexical property membership differs',
            invocation=[sys.executable,str(BINARY/'scripts/audit-data.py')],cwd=snapshot)
    assert {str(path):sha(path) for path in frozen}==hashes,'audit inputs changed during checks'
    receipt=dict(schema=1,policy='shared Rust CFG plus exact contextual Python data policy',
        frozen=hashes,flags=audit.FLAGS,checks=records,seconds=time.monotonic()-started)
    destination=BINARY/'build/graph-negative-validation.json'
    destination.write_text(json.dumps(receipt,indent=2)+'\n')
    print('%d graph/data policy checks passed in %.6fs; canonical inputs unchanged.'%(len(records),receipt['seconds']))


if __name__=='__main__':main()
