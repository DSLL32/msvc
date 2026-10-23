#!/usr/bin/env python3
"""Meaningful negative checks for acceptance/identity failure paths."""
from graph import graph, names
from runtime import identify
from state import mask
from state import ROOT
from state import BINARY
from binary import PE
from audit import publics,validate_ownership
from accept import validate_snapshot
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from pathlib import Path
from objects import compare_walks,original_walks
import struct
import subprocess
import tempfile
import copy
import json
from state import load,sha

class Bytes:
    def __init__(self,data):self.data=data
    def read(self,address,size):return self.data[address:address+size]

def reject(label, action):
    try:action()
    except (AssertionError,ValueError,IndexError,KeyError):
        print('rejected:',label)
        return
    raise AssertionError('accepted invalid '+label)

def main():
    reject('truncated RET cleanup',lambda:graph(Bytes(b'\xc2\x04\x00'),0,2))
    reject('truncated immediate',lambda:graph(Bytes(b'\xb8\0\0\0\0\xc3'),0,3))
    reject('missing terminal',lambda:graph(Bytes(b'\x90\xc3'),0,1))
    reject('branch into operand',lambda:graph(Bytes(b'\x75\x01\xb8\0\0\0\0\xc3'),0,8))
    reject('undeclared indirect jump',lambda:graph(Bytes(b'\xff\xe0'),0,2))
    reject('conditional outside function',lambda:graph(Bytes(b'\x75\x20\xc3'),0,3))
    reject('branch into text data',lambda:graph(Bytes(b'\xeb\0\xc3'),0,3,data=[(2,3)]))
    reject('unresolved application call',lambda:names({0x12345678},{}))
    positive=graph(Bytes(b'\x74\x01\x90\xc2\x08\x00'),0,6)
    assert positive['returns']=={8} and positive['end']==6
    # A tiny wrapper with identical nonrelocation bytes must fail when its
    # external `_main` target is mapped to the wrong application identity.
    target=ROOT/'toolchains/1.0-nt/bin/CVPACK.EXE'
    good=identify(target,{'_main':0x17000})
    bad=identify(target,{'_main':0x17001})
    assert any(r['name']=='_mainCRTStartup' and r['start']==0x2c260 for r in good['functions'])
    assert not any(r['name']=='_mainCRTStartup' and r['start']==0x2c260 for r in bad['functions'])
    print('rejected: CRT startup with incorrect relocated application target')
    config,original=load()
    context=json.loads((BINARY/'data.json').read_text())
    validate_ownership(config,original,good,context)
    ownership_mutations={}
    changed=copy.deepcopy(config)
    next(r for r in changed['functions']if r['name']=='parse_options')['text_data'].append([0x1765c,0x176bc])
    ownership_mutations['duplicate text-data ownership']=changed
    changed=copy.deepcopy(config)
    next(r for r in changed['functions']if r['name']=='parse_options')['text_data'].append([0x17000,0x17001])
    ownership_mutations['text-data overlaps another application body']=changed
    changed=copy.deepcopy(config)
    next(r for r in changed['functions']if r['name']=='cvpack_main')['extent'][1]=0x174c1
    ownership_mutations['application code includes another function entry']=changed
    changed=copy.deepcopy(config)
    stub=next(r for r in changed['functions']if r['status']=='stub')
    next(r for r in changed['functions']if r['name']=='parse_options')['text_data'].append([stub['original'],stub['original']+1])
    ownership_mutations['text-data masks an unreconstructed function entry']=changed
    changed=copy.deepcopy(config)
    next(r for r in changed['functions']if r['name']=='parse_options')['text_data'].append([0x38cf8,0x38cfc])
    ownership_mutations['unmask outside original file-backed code']=changed
    changed=copy.deepcopy(config)
    changed['support_unmasked'].append(copy.deepcopy(changed['support_unmasked'][0]))
    ownership_mutations['duplicate runtime acceptance ownership']=changed
    for label,changed in ownership_mutations.items():
        reject(label,lambda changed=changed:validate_ownership(changed,original,good,context))
    # A runtime identity must never become an application owner, even when the
    # runtime catalogue itself also identifies those bytes.
    changed=copy.deepcopy(config)
    accepted=next(r for r in changed['functions']if r['status']=='accepted')
    runtime_record=dict(name='test-runtime',start=accepted['extent'][0],end=accepted['extent'][1])
    changed['support_unmasked'].append(runtime_record)
    synthetic=copy.deepcopy(good)
    synthetic['functions'].append(runtime_record)
    reject('runtime overlaps accepted application code',lambda:validate_ownership(changed,original,synthetic,context))
    changed_context=[*context,dict(name='unidentified-text-data',original=config['support_unmasked'][0]['start'],size=1)]
    reject('runtime unmask overlaps unrelated contextual data',lambda:validate_ownership(config,original,good,changed_context))
    # Exact same-range runtime aliases require explicit identity and evidence;
    # different extents and duplicate names never qualify as aliases.
    changed=copy.deepcopy(config)
    alias=copy.deepcopy(changed['support_unmasked'][0]);alias['name']='test-alias'
    changed['support_unmasked'].append(alias)
    synthetic=copy.deepcopy(good);synthetic['functions'].append(alias)
    reject('runtime alias without explicit declaration',lambda:validate_ownership(changed,original,synthetic,context))
    changed['runtime_aliases']=[dict(start=alias['start'],end=alias['end'],names=[config['support_unmasked'][0]['name'],alias['name']],evidence='cases/startup/proof.json')]
    validate_ownership(changed,original,synthetic,context)
    scratch=Path.home()/'.ai-tmp/vc1.0-cvpack'
    scratch.mkdir(exist_ok=True,parents=True)
    with tempfile.TemporaryDirectory(prefix='snapshot-rejections-',dir=scratch) as temporary:
        directory=Path(temporary)
        old=directory/'original';new=directory/'native';old.mkdir();new.mkdir()
        current=directory/'functions.json';current.write_bytes((BINARY/'functions.json').read_bytes())
        for destination in (old,new):(destination/'configuration.json').write_bytes(current.read_bytes())
        masked=old/'CVPACK.EXE';masked.write_bytes(b'captured masked image')
        executable=directory/'native.exe';executable.write_bytes(b'native executable')
        captured=dict(configuration_sha256=sha(current),original_sha256=sha(target),executable_sha256=sha(masked))
        native=dict(configuration_sha256=sha(current),original_sha256=sha(target),executable_sha256=sha(executable))
        def snapshot(a=captured,b=native):validate_snapshot(config,current,old,new,a,b,target,executable)
        snapshot()
        stale=copy.deepcopy(config);stale['review']['snapshot-negative-test']=True
        for destination,receipt in ((old,captured),(new,native)):
            (destination/'configuration.json').write_text(json.dumps(stale))
            receipt['configuration_sha256']=sha(destination/'configuration.json')
        reject('two mutually matching stale configurations',snapshot)
        for destination,receipt in ((old,captured),(new,native)):
            (destination/'configuration.json').write_bytes(current.read_bytes())
            receipt['configuration_sha256']=sha(current)
        changed=dict(native,configuration_sha256='0'*64)
        reject('native/original configuration receipt mismatch',lambda:snapshot(b=changed))
        changed=dict(captured,original_sha256='0'*64)
        reject('captured original target hash mismatch',lambda:snapshot(a=changed))
        changed=dict(native,original_sha256='0'*64)
        reject('native original-target identity mismatch',lambda:snapshot(b=changed))
        masked.write_bytes(b'changed masked image')
        reject('captured masked image changed after execution',snapshot)
        masked.write_bytes(b'captured masked image')
        executable.write_bytes(b'changed native executable')
        reject('native executable changed after execution',snapshot)
    specifications=[dict(name='pool',original=0x4000,pointer_fields=[4])]
    left={'pool':[dict(address=0x1000,bytes=struct.pack('<III',7,0x1010,11).hex()),dict(address=0x1010,bytes=struct.pack('<III',9,0,13).hex())]}
    right={'pool':[dict(address=0x9000,bytes=struct.pack('<III',7,0x9010,11).hex()),dict(address=0x9010,bytes=struct.pack('<III',9,0,13).hex())]}
    compare_walks(specifications,left,right)
    changed={'pool':[dict(address=0x9000,bytes=struct.pack('<III',8,0x9010,11).hex()),right['pool'][1]]}
    reject('changed observed object byte',lambda:compare_walks(specifications,left,changed))
    changed={'pool':[dict(address=0x9000,bytes=struct.pack('<III',7,0x9000,11).hex()),right['pool'][1]]}
    reject('wrong observed object pointer identity',lambda:compare_walks(specifications,left,changed))
    changed={'pool':[dict(address=0x9000,bytes=struct.pack('<III',7,0xffff,11).hex()),right['pool'][1]]}
    reject('unresolved observed object pointer',lambda:compare_walks(specifications,left,changed))
    reject('unterminated original object list',lambda:original_walks('walk-end 00004000 0 00001000\n',specifications))
    anchors=good['callee_identity_anchors']
    if anchors:
        scratch=Path.home()/'.ai-tmp/vc1.0-cvpack'
        scratch.mkdir(exist_ok=True,parents=True)
        with tempfile.TemporaryDirectory(prefix='anchor-mask-',dir=scratch) as directory:
            image=Path(directory)/'exact-closure.exe'
            # Even unmasking EVERY exact runtime/import identity must leave a
            # reviewed semantic variant entirely trapped, with no coverage gain.
            closure=dict(functions=[],support_unmasked=good['functions']+good['imports'])
            pe=PE(target)
            mask(closure,pe,image)
            masked=PE(image)
            for anchor in anchors:
                assert not any(r['start']<=anchor['start']<r['end'] for r in good['functions']+good['imports'])
                assert masked.read(anchor['start'],anchor['end']-anchor['start'])==b'\xcc'*(anchor['end']-anchor['start'])
        print('rejected: reviewed variant unmask through exact runtime closure')
    # Mutate actual compiler output, leaving originals/source/build products intact.
    rebuilt=BINARY/'build/CVPACK.EXE'
    if rebuilt.is_file():
        pe=PE(rebuilt)
        symbols=publics()
        start=symbols['_main']
        end=min(address for address in symbols.values() if address>start)
        instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.read(start,end-start),start))
        call=next(i for i in instructions if i.mnemonic=='call' and i.op_str=='0x%x'%symbols['_parse_options'])
        terminal=next(i for i in reversed(instructions) if i.mnemonic=='ret')
        assert terminal.address+3<=end
        mutations={}
        image=bytearray(pe.data)
        struct.pack_into('<i',image,pe.offset(call.address)+1,symbols['_print_banner']-call.address-5)
        mutations['redirected application callee']=image
        image=bytearray(pe.data)
        image[pe.offset(terminal.address):pe.offset(terminal.address)+3]=b'\xc2\x04\x00'
        mutations['changed callee stack cleanup']=image
        image=bytearray(pe.data)
        image[pe.offset(symbols['_show_logo'])]^=1
        mutations['changed contextual global initializer']=image
        scratch=Path.home()/'.ai-tmp/vc1.0-cvpack'
        scratch.mkdir(exist_ok=True,parents=True)
        with tempfile.TemporaryDirectory(prefix='audit-rejections-',dir=scratch) as directory:
            for label,image in mutations.items():
                modified=Path(directory)/'modified.exe'
                modified.write_bytes(image)
                result=subprocess.run(['python3',str(BINARY/'scripts/audit.py'),'--canonical-only','--rebuilt',str(modified)],capture_output=True)
                assert result.returncode!=0,'accepted '+label
                assert b'AssertionError' in result.stderr,result.stderr
                print('rejected:',label)

if __name__=='__main__':main()
