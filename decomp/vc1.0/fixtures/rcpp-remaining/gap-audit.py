#!/usr/bin/env python3
"""Static classification of the pre-suffix accepted-code complement.

Analysis only: no mask acceptance, target mutation or failure experiments.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_IMM

HERE=Path(__file__).resolve().parent
RCPP=HERE.parent.parent/'rcpp'
sys.path.insert(0,str(RCPP/'scripts'))
from binary import PE
from compare import imports

# Actual complete bodies from return/branch-boundary analysis. Shared-entry
# string code is one union extent, not multiple aliases counted twice.
BODIES=[
 ('option_contains_character',0x1007830,0x1007855,'native-unused'),
 ('__isctype',0x10078d0,0x100798f,'fixed-C-locale'),
 ('_strdup',0x10079b8,0x10079df,'option-copy-disabled'),
 ('_atoi',0x1007aa0,0x1007aae,'unused-wrapper'),
 ('_rc_fast_exit',0x1007b10,0x1007b22,'CRT-exception-only'),
 ('_cexit',0x1007b30,0x1007b3f,'unused-wrapper'),
 ('_c_exit',0x1007b40,0x1007b4f,'unused-wrapper'),
 ('__commit',0x1008230,0x100828f,'commit-disabled'),
 ('__open_osfhandle',0x1008430,0x10084c4,'unused-CRT-interface'),
 ('__amsg_exit',0x1008ee0,0x1008f01,'startup-failure-only'),
 ('__FF_MSGBANNER',0x1008f50,0x1008f76,'startup-failure-only'),
 ('__NMSG_WRITE',0x1008f80,0x1008fd8,'startup-failure-only'),
 ('__XcptFilter',0x1008ff0,0x100911d,'CRT-exception-only'),
 ('_xcptlookup',0x1009120,0x100914b,'CRT-exception-only'),
 ('__except_handler3',0x1009168,0x1009233,'CRT-exception-only'),
 ('__global_unwind2',0x1009233,0x1009253,'CRT-exception-only'),
 ('__unwind_handler',0x1009253,0x1009279,'CRT-exception-only'),
 ('__local_unwind2',0x1009279,0x10092df,'CRT-exception-only'),
 ('__abnormal_termination',0x10092df,0x100930a,'unused-CRT-interface'),
 ('__setjmp',0x100930a,0x1009349,'unused-CRT-interface'),
 ('_longjmp',0x1009349,0x100939c,'unused-CRT-interface'),
 ('__seh_longjmp_unwind4',0x100939c,0x10093d7,'unused-CRT-interface'),
 ('_rc_long_kind@4',0x100cb80,0x100cb91,'selected-successful-input'),
 ('_rc_unsigned_kind@4',0x100cba0,0x100cbb5,'selected-successful-input'),
 ('_getc',0x100e8f0,0x100e8fe,'unused-wrapper'),
 ('__open',0x100eef0,0x100ef0a,'unused-wrapper'),
 ('__chsize',0x100f300,0x100f449,'fixed-file-modes'),
 ('__setmode',0x100f470,0x100f501,'fixed-file-modes'),
 ('_putc',0x100f550,0x100f563,'unused-wrapper'),
 ('strcat-strcpy-shared-entries',0x1010a08,0x1010a46,'option-copy-disabled'),
 ('_strlen',0x1010a48,0x1010a5f,'option-copy-disabled'),
 ('__toupper_ascii',0x1010ae0,0x1010ae8,'unused-wrapper')]


def union(ranges):
    result=[]
    for start,end in sorted(ranges):
        if result and start<=result[-1][1]:result[-1][1]=max(end,result[-1][1])
        else:result.append([start,end])
    return result


def main():
    cfg=json.loads((RCPP/'functions.json').read_text())
    coverage=json.loads((HERE/'baseline-coverage.json').read_text())
    assert coverage['accepted_union_bytes']==35515,'This audit pins the pre-suffix checkpoint.'
    pe=PE(RCPP/'../../../toolchains/1.0-nt/bin/RCPP.EXE')
    assert hashlib.sha256(pe.data).hexdigest()==cfg['target']['sha256']
    accepted=coverage['accepted_clipped_union']
    section=next(s for s in pe.sections if s['name']=='.text')
    start=pe.base+section['rva'];end=start+section['virtual_size']
    gaps=[];cursor=start
    for a,b in accepted:
        if a>cursor:gaps.append([cursor,a])
        cursor=max(cursor,b)
    if cursor<end:gaps.append([cursor,end])
    accepted_names={f['name']for f in cfg['functions'] if any(a<=f['start']<f['end']<=b for a,b in accepted)}
    bodies=[dict(name=name,start=a,end=b,bytes=b-a,reachability=kind)for name,a,b,kind in BODIES]
    for record in imports(pe):
        if not any(a<=record['start']<record['end']<=b for a,b in accepted):
            bodies.append(dict(record,bytes=6,reachability='import-dependent-on-callers'))
    bodies.sort(key=lambda r:r['start'])
    for left,right in zip(bodies,bodies[1:]):assert left['end']<=right['start']
    for record in bodies:
        assert any(a<=record['start']<record['end']<=b for a,b in gaps),record
    known=union((r['start'],r['end'])for r in bodies)
    padding=[]
    for a,b in gaps:
        cursor=a
        for x,y in known:
            if y<=a or x>=b:continue
            if x>cursor:padding.append([cursor,x])
            cursor=y
        if cursor<b:padding.append([cursor,b])
    # Every byte outside identified complete bodies is conventional alignment
    # or zero-fill. Nothing arbitrary is silently counted as another function.
    patterns=[bytes.fromhex(x)for x in ['8da42400000000','8d9b00000000','0500000000','8d642400','8d4900','8bff','90','00']]
    for a,b in padding:
        data=pe.read(a,b-a);offset=0
        while offset<len(data):
            match=next((p for p in patterns if data.startswith(p,offset)),None)
            assert match is not None,('unclassified bytes',hex(a+offset),data[offset:offset+16].hex())
            offset+=len(match)
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
    owners=[f for f in cfg['functions']if f['name']in accepted_names]+bodies
    incoming={r['start']:[] for r in bodies}
    for owner in owners:
        for instruction in cs.disasm(pe.read(owner['start'],owner['end']-owner['start']),owner['start']):
            if instruction.mnemonic in ['call','jmp'] and instruction.operands[0].type==X86_OP_IMM:
                target=instruction.operands[0].imm&0xffffffff
                # strcpy is a distinct entry into the shared body.
                key=0x1010a08 if target==0x1010a0c else target
                if key in incoming and not owner['start']<=target<owner['end']:
                    incoming[key].append(dict(owner=owner['name'],instruction=instruction.address,target=target))
    for record in bodies:
        record['direct_callers_and_tail_jumps']=incoming[record['start']]
        addresses=[record['start']]+([0x1010a0c]if record['name']=='strcat-strcpy-shared-entries' else [])
        refs=[]
        for address in addresses:
            needle=struct.pack('<I',address);offset=0
            while True:
                offset=pe.data.find(needle,offset)
                if offset<0:break
                refs.append(dict(file_offset=offset,addressed_entry=address));offset+=1
        record['stored_address_references']=refs
    options=[]
    for index in range(100):
        address=0x101a108+12*index;data=pe.read(address,12)
        pattern=struct.unpack_from('<I',data)[0];action=data[9]
        if not action:break
        options.append(dict(pattern=pe.string(pattern),action=action))
    assert not any(r['action']==0x27 for r in options)
    result=dict(target_sha256=cfg['target']['sha256'],baseline_accepted_code_bytes=35515,total_code_bytes=40448,
                unaccepted_complement_bytes=sum(b-a for a,b in gaps),
                identified_body_union_bytes=sum(b-a for a,b in known),
                alignment_or_zero_fill_bytes=sum(b-a for a,b in padding),
                complete_bodies=bodies,
                option_table=dict(address=0x101a108,entries=options,copy_argument=0,
                                  sole_native_call=0x1007054,copy_zero_push=0x1007042),
                selected_cases=[dict(id='long-suffix',function='_rc_long_kind@4',additional_extent_bytes=17),
                                dict(id='unsigned-suffix',function='_rc_unsigned_kind@4',additional_extent_bytes=21)],
                caveats=['Complete body extents include internal alignment, but inter-function padding and aliases are excluded.',
                         'strcat/strcpy shared code is unioned once; placeholder one-byte commit extent is replaced by its actual 95-byte body for analysis only.',
                         'CRT exception/startup failure code is statically identified only; no fault, dispatch or resource-failure experiment was run.',
                         'No ordinary successful public route identified is not a claim of universal mathematical unreachability.'])
    (HERE/'gap-map.json').write_text(json.dumps(result,indent=2)+'\n')
    print('%d unaccepted bytes = %d complete-body/import bytes + %d alignment/zero-fill bytes.'%(result['unaccepted_complement_bytes'],result['identified_body_union_bytes'],result['alignment_or_zero_fill_bytes']))


if __name__=='__main__':main()
