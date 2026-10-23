#!/usr/bin/env python3
"""Explicit prospective rebuilt-address refresh; never invoked by fixtures.

Original semantic witnesses remain unchanged. Previously accepted frontiers own
exact earlier packet/checker/object bytes under full-link-proof/.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from binary import PE,COFF
from member_pointers import CONTRACTS,check_static

BINARY=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--write',action='store_true',required=True)
parser.add_argument('--receipt',type=Path,required=True)
args=parser.parse_args();sha=lambda b:hashlib.sha256(b).hexdigest()
original=PE(BINARY.parents[2]/'toolchains/5.10/bin/LINK.EXE');rebuilt=PE(BINARY/'build/link.exe')
map_bytes=(BINARY/'build/link.map').read_bytes();text=map_bytes.decode('latin1')
publics={name:int(address,16)for name,address in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',text,re.M)}
configuration=json.loads((BINARY/'functions.json').read_text());check_static(original,rebuilt,text,configuration)
cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
packet_path=BINARY/'cases/hello-win32/full-link-import-preservation.json';old_packet=packet_path.read_bytes();packet=json.loads(old_packet)
changes=[]
for function in packet['function_contracts']:
    for region in function['original_ranges']:
        assert sha(original.read(region['begin'],region['end']-region['begin']))==region['sha256']
    counterpart=function['rebuilt'];start=publics[counterpart['symbol']];instructions=[]
    for instruction in cs.disasm(rebuilt.read(start,256),start):
        instructions.append(instruction)
        if instruction.mnemonic=='ret':break
    assert instructions[-1].mnemonic=='ret'
    old_instructions=list(cs.disasm(bytes.fromhex(counterpart['bytes']),0))
    assert instructions[-1].op_str==old_instructions[-1].op_str,'actual counterpart RET ABI changed'
    if counterpart['classification']=='named trapping boundary':
        assert any(i.mnemonic=='int3'for i in instructions),'callee reconstructed: independently refresh its actual counterpart ABI proof first'
    for register in function['preserves']:
        aliases={'esi':{'esi','si','sil'},'edi':{'edi','di','dil'}}[register]
        own=[i for i in instructions if aliases & {i.reg_name(r)for r in i.regs_access()[1]}and not(i.mnemonic=='pop'and i.op_str==register)]
        assert not own,'new compiled nonvolatile-register write requires actual callee review'
    data=rebuilt.read(start,sum(i.size for i in instructions));before=counterpart['bytes'];counterpart['bytes']=data.hex();counterpart['own_writes']=[]
    if before!=counterpart['bytes']:changes.append(function['name'])
# All original/provider/ABI fields must remain exactly the previously reviewed
# bytes/values. Only the compiled counterpart subrecords above may differ.
assert {k:v for k,v in packet.items()if k!='function_contracts'}=={k:v for k,v in json.loads(old_packet).items()if k!='function_contracts'}
for before,after in zip(json.loads(old_packet)['function_contracts'],packet['function_contracts']):
    assert {k:v for k,v in before.items()if k!='rebuilt'}=={k:v for k,v in after.items()if k!='rebuilt'}
closed_path=BINARY/'cases/hello-win32/full-link-closed-slot.json';old_closed=closed_path.read_bytes();closed=json.loads(old_closed)
assert closed['original']['target_sha256']==sha(original.data)
header_slot=publics[closed['rebuilt']['symbol']];needle=b'\xff\x15'+header_slot.to_bytes(4,'little')
owner=publics['?FullLink@LinkContext@@QAEHPAE@Z'];obj=COFF((BINARY/'build/full-link.obj').read_bytes());symbol=next(s for s in obj.symbols.values()if s['name']=='?FullLink@LinkContext@@QAEHPAE@Z');section=obj.sections[symbol['section']-1]
call_sites=[]
for offset,reference,kind in section['relocs']:
    if reference['name']==closed['rebuilt']['symbol']:
        assert kind==6 and section['data'][offset-2:offset]==b'\xff\x15'
        call_sites.append(owner+offset-symbol['value']-2)
assert len(call_sites)==1 and rebuilt.read(call_sites[0],6)==needle
code=[];debug=[]
for path in sorted((BINARY/'build').glob('*.obj')):
    try:obj=COFF(path.read_bytes())
    except (AssertionError,IndexError,ValueError):continue
    for section in obj.sections:
        for offset,reference,kind in section['relocs']:
            if reference['name']!=closed['rebuilt']['symbol']:continue
            item=dict(object=str(path.relative_to(BINARY)),object_sha256=sha(path.read_bytes()),section=section['name'],offset=offset,relocation_kind=kind)
            if section['flags']&0x20:
                prefix=section['data'][offset-2:offset];assert kind==6 and prefix in [b'\xff\x15',b'\xc7\x05']
                item.update(form='call absolute memory'if prefix==b'\xff\x15'else 'store immediate function pointer',prefix=prefix.hex());code.append(item)
            else:
                assert section['name']=='.debug$S'and section['flags']==0x42100048 and kind in [10,11],'new loaded-data alias requires review'
                item['section_flags']=section['flags'];debug.append(item)
assert len(code)==3 and len(debug)==4
closed['rebuilt'].update(pe_sha256=sha(rebuilt.data),map_sha256=sha(map_bytes),slot=header_slot,required_preservation_call=call_sites[0],code_references=code,debug_metadata_references=debug)
for source in closed['sources']:
    assert sha((BINARY/source['path']).read_bytes())==source['sha256'],'closed-slot source logic changed; review required'
assert closed['original']==json.loads(old_closed)['original']
packet_path.write_text(json.dumps(packet,indent=2)+'\n');closed_path.write_text(json.dumps(closed,indent=2)+'\n')
metadata_path=BINARY/'cases/hello-win32/full-link-audit-trial/metadata.json';old_metadata=metadata_path.read_bytes();metadata=json.loads(old_metadata)
# Metadata from the current coherent original-compiler counterpart, with the
# reviewed exact register contracts. No target identity is inferred at a callback.
base=json.loads((BINARY/'build/graph.json').read_text());base['functions'].setdefault('?FullLink@LinkContext@@QAEHPAE@Z',{})['register_imports']={'require_all_register_calls_resolved':True}
base['register_imports']=metadata['register_imports']
for contract in base['register_imports']['preservation_contracts']:contract['evidence']['sha256']=sha(packet_path.read_bytes())
callback,=base['register_imports']['callback_preservation_contracts'];callback['rebuilt']=dict(site=call_sites[0],slot=header_slot);callback['evidence']['sha256']=sha(closed_path.read_bytes())
metadata_path.write_text(json.dumps(base,indent=2)+'\n')
args.receipt.write_text(json.dumps(dict(kind='explicit prospective counterpart-only binding refresh; no original execution',original_semantics_unchanged=True,callee_counterpart_bytes_changed=changes,old_sha256=dict(ordinary=sha(old_packet),callback=sha(old_closed),metadata=sha(old_metadata)),new_sha256=dict(ordinary=sha(packet_path.read_bytes()),callback=sha(closed_path.read_bytes()),metadata=sha(metadata_path.read_bytes())),original_sha256=sha(original.data),rebuilt_sha256=sha(rebuilt.data),map_sha256=sha(map_bytes),qualification='Existing original body/provider/SDK/closed-target semantics retained exactly. Current genuine trapping/free-wrapper counterparts have unchanged RET ABI and no own contracted-register write. New/changed nonvolatile or reconstructed bodies fail this refresh and need concrete proof.'),indent=2)+'\n')
print('Prospective FullLink counterpart bindings refreshed; original semantics unchanged.')
