#!/usr/bin/env python3
"""Fresh four-formal/owner/return checks, separate from cached fixtures."""
import argparse,importlib.util,json,re
from pathlib import Path

def check(original,native,frontier=False):
    payload=(Path(__file__).resolve().parents[1]/'cases/direct-rebase-fallback/inputs/payload.bin').read_bytes()
    spec=importlib.util.spec_from_file_location('oracle',Path(__file__).with_name('rebase-fallback-report.py'));oracle=importlib.util.module_from_spec(spec);spec.loader.exec_module(oracle)
    rows=oracle.expected_rows(payload)
    original=[e for e in original if e.get('event') in ('entry','return','frontier')]
    native=native['events']
    assert len(original)==len(native)==(1 if frontier else 8)
    for index,(left,right) in enumerate(zip(original,native)):
        event='frontier' if frontier else ('entry' if index%2==0 else 'return')
        phase=0 if frontier else index//2
        assert left['event']==right['event']==event
        assert left['name']==right['name']=='fallback_process_relocations'
        assert left['phase']==right['phase']==phase
        assert right['actual_caller']=='__wrap_main'
        for e in (left,right):
            a=e['arguments']
            assert a==[e['page_address'],(0,7,7,1)[phase],e['word_address']+(14 if phase==3 else 0),(0,0x18004,0xfffe7ffc,0)[phase]]
            assert e['page_address']!=e['word_address'] and not set(range(e['page_address']-16,e['page_address']+4112))&set(range(e['word_address']-16,e['word_address']+32))
            assert e['errors']==[0,0,0]
            page=payload[:4096] if event in ('entry','frontier') and phase==0 else rows[phase-(1 if event=='entry' else 0)][20:4116]
            assert e['page']==page.hex() and e['records']==payload[4096:].hex()
            assert e['fp_control']&0xffff==0x27f and e['fp_status']&0xffff==0 and e['fp_tag']&0xffff==0xffff
            if e['event']=='return':
                result=0 if phase==3 else e['word_address']+(0 if phase==0 else 14)
                assert e['eax']==result
            for k in ('page_guards','page_guard_after','record_guard_before','record_guard_after'):
                assert e[k]=='a5'*16
        for k in ('page','records','page_guards','page_guard_after','record_guard_before','record_guard_after'):
            assert left[k]==right[k],(index,k)
        # Architectural x87 payload and instruction/data pointers are all zero.
        raw=bytes.fromhex(left['fp_raw'])
        assert raw[12:16]==raw[20:24]==b'\0'*4 and raw[28:108]==b'\0'*80
        regs=re.findall(r'R[0-7]: Empty\s+0x([0-9a-f]+)',right['fp_raw'])
        assert len(regs)==8 and all(int(x,16)==0 for x in regs)
        assert 'Instruction Pointer: 0x00:0x00000000' in right['fp_raw'] and 'Operand Pointer:     0x00:0x00000000' in right['fp_raw']
        if event=='return':
            for events in (original,native):
                before=events[index-1];after=events[index]
                assert after['esp']==before['esp']+20 and after['caller']==before['caller']
                assert all(after[k]==before[k] for k in ('ebx','esi','edi','ebp'))
    return dict(status='pass',observations=len(original),formals=4,ABI='stdcall RET16; ESP+20/nonvolatiles',FP='CW027f/SW0000/tagsffff, eight zero empty payloads and zero IP/DP strict',qualification=[])

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--frontier',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    row=check([json.loads(l)for l in(a.original/'events.jsonl').read_text().splitlines()],json.loads((a.native/'state.json').read_text()),a.frontier)
    row.update(original=str(a.original),native=str(a.native))
    if a.output:a.output.write_text(json.dumps(row,indent=2)+'\n')
    print(json.dumps(row))
