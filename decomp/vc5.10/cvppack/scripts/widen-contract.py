#!/usr/bin/env python3
"""Decode caller-side MSPDB observations into an encountered-only typed contract."""
import json
from pathlib import Path
import struct
from state import BINARY,sha


def verify_provider(receipt,provider_artifact=None):
    provider=receipt['actual_loaded_mspdb']
    canonical='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'
    assert provider['sha256']==canonical
    assert sha(Path(provider_artifact) if provider_artifact is not None else Path(provider['path']))==canonical,'actual loaded MSPDB hFile bytes differ'


def decode(directory,provider_artifact=None):
    receipt=json.loads((directory/'receipt.json').read_text())
    verify_provider(receipt,provider_artifact)
    for item in receipt['files']:assert sha(directory/item['name'])==item['sha256']
    rows=(directory/'events').read_text().splitlines();records=[];last_error=None;error_states=[]
    for i,line in enumerate(rows):
        if line.startswith('last-error '):last_error=int(line.split()[1])
        if not line.startswith('widen '):continue
        memory={}
        for row in rows[i+1:]:
            if not row.startswith('memory '):break
            _,address,raw=row.split();memory[int(address,16)]=bytes.fromhex(raw)
        records.append((line.split()[1:],memory))
        error_states.append(last_error)
    assert len(records)==147
    instances=[]
    for index in range(21):
        group=records[index*7:index*7+7]
        assert [r[0][0] for r in group]==['create-call','create-return','method-call','method-return','method-call','method-return','destroy']
        create,returned,query,qret,emit,eret,destroy=[r[0] for r in group]
        assert create[2:]==['256','0'] and returned[1]=='1'
        obj=int(returned[2],16);table=int(returned[3],16)
        assert group[1][1][obj]==struct.pack('<I',table) and len(group[1][1][table])==40
        assert query[1]=='32' and emit[1]=='36'
        assert all(int(r[2],16)==obj for r in (query,qret,emit,eret)) and int(destroy[1],16)==obj
        assert qret[3]==eret[3]=='1'
        assert query[3:]==emit[3:]
        description,buffer,size,flags=[int(v,16) for v in query[3:]]
        assert flags==0
        before=struct.unpack('<4I',group[2][1][description]);assert before==(0,0,0,0)
        sizes=struct.unpack('<4I',group[3][1][description]);assert sizes[2:]==(0,0)
        emission=struct.unpack('<4I',group[4][1][description]);assert emission[:2]==sizes[:2] and all(emission[2:])
        after=struct.unpack('<4I',group[5][1][description]);assert after[2:]==emission[2:]
        input_bytes=group[2][1][buffer];assert len(input_bytes)==size and input_bytes==group[4][1][buffer]
        input_after=group[5][1][buffer];assert input_after==input_bytes,'emit changed initialized input'
        output_before=group[4][1][emission[2]];fixups_before=group[4][1][emission[3]]
        output_after=group[5][1][after[2]];fixups_after=group[5][1][after[3]]
        assert len(output_before)==len(output_after)==sizes[0] and len(fixups_before)==len(fixups_after)==sizes[1]*8
        assert after[0]<=sizes[0] and after[1]<=sizes[1]
        assert output_before[after[0]:]==output_after[after[0]:],'emit changed output capacity tail'
        assert fixups_before[after[1]*8:]==fixups_after[after[1]*8:],'emit changed fixup capacity tail'
        output_bytes=output_after[:after[0]];fixups=fixups_after[:after[1]*8]
        instances.append(dict(instance=index+1,raw_object_address=obj,raw_description_address=description,
            last_error_sequence=error_states[index*7:index*7+7],
            factory=dict(capacity=256,flags=0,result=1),
            query=dict(slot=32,flags=0,input=input_bytes.hex(),input_bytes=size,initial_description=[0,0,0,0],output_capacity=sizes[0],fixup_capacity=sizes[1],result=1),
            emit=dict(slot=36,flags=0,result=1,written_bytes=after[0],written_fixups=after[1],output=output_bytes.hex(),fixups=fixups.hex(),
                pointers_preserved=True,input_unchanged=input_after==input_bytes,input_after=input_after.hex(),
                output_before=output_before.hex(),output_after=output_after.hex(),fixups_before=fixups_before.hex(),fixups_after=fixups_after.hex(),
                capacity_tails_unchanged=True),destroy=dict(slot=0)))
    return dict(schema=1,provider='MSPDB50.DLL',provider_sha256=receipt['actual_loaded_mspdb']['sha256'],
        source_receipt=str((directory/'receipt.json').relative_to(BINARY)),source_receipt_sha256=sha(directory/'receipt.json'),
        raw_events_sha256=sha(directory/'events'),input_dll_sha256=sha(directory/'input.dll'),output_dll_sha256=sha(directory/'output.dll'),
        scope='Only this frozen input, sequence and complete symbol-buffer requests. Not a PDB implementation.',
        abi=dict(factory='cdecl (WidenTi **out, uint32 capacity, int32 flags), int32 result',
            query='thiscall ECX opaque object, callee pops16: (description *, const bytes *, uint32 size, uint32 flags)',
            emit='same ABI as query; description is {uint32 bytes, uint32 fixup_count, bytes *output, pair32 *fixups}',
            destroy='thiscall ECX opaque object, no stack arguments; return unused by caller'),
        ownership='21 sequential objects: create, query, emit, destroy. All original instances reuse the same address after destruction. No simultaneous live objects.',
        private_state='CVPACK reads only object.vptr in its virtual-call wrappers. Source must retain opaque fields; dependency-private data and original function addresses are never native implementations.',
        slot_inventory=dict(factory=21,slot0=21,slot12=0,slot32=21,slot36=21),instances=instances)


if __name__=='__main__':
    binding=json.loads((BINARY/'cases/hello-debug-dll/providers/binding.json').read_text())
    assert sha(BINARY/binding['source_receipt'])==binding['source_receipt_sha256']
    contract=decode(BINARY/'cases/hello-debug-dll/original-probe-0003',provider_artifact=BINARY/binding['owned_artifact'])
    output=BINARY/'cases/hello-debug-dll/widen-contract.json'
    output.write_text(json.dumps(contract,indent=2)+'\n')
    print(json.dumps(dict(instances=len(contract['instances']),slot_inventory=contract['slot_inventory'],bytes=output.stat().st_size),indent=2))
