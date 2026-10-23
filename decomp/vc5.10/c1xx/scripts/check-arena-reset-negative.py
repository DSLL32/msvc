#!/usr/bin/env python3
"""Affected repeated receiver/arithmetic/API/reset controls; no binary execution."""
import argparse,importlib.util,json,re,shutil,subprocess,tempfile,time
from pathlib import Path


def load(name,file):
    s=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--original-only',action='store_true');args=parser.parse_args()
    d=load('reset_negative_decoder','check-callback.py');h=load('reset_negative_helpers','check-projection-negative.py');packet=json.loads((d.s.PROJECT/'cases/minimal/arena-reset-trials.json').read_text());row=packet['trials'][0];original,native=Path(row['original']),Path(row['native'])
    d.s.require(d.s.sha(original/'receipt.json')==row['original_receipt_sha256'] and d.s.sha(native/'receipt.json')==row['native_receipt_sha256'],'not immutable declared pair');d.s.HISTORICAL_SNAPSHOT=native;d.check(original,native,require_audit=False)
    cases=['missing-last-call','last-Reserve-this','last-descriptor','last-FP-live80','last-QWORD-high','last-rounded-result','regular-API-kind','last-API-stack','last-owned-size','last-Reset-this','last-Reset-formals','last-Reset-return-stack','last-Reset-nonvolatile','last-Reset-field','next-formals','next-caller','original-late-FP','original-last-Reset-unknown-BSS','final-last-arena']
    files={'missing-last-call':'arena-reset-returns','last-Reserve-this':'arena-member-entries','last-descriptor':'scaler-entries','last-FP-live80':'scaler-fpu','last-QWORD-high':'scaler-fpu','last-rounded-result':'scaler-returns','regular-API-kind':'arena-api-entries','last-API-stack':'arena-api-returns','last-owned-size':'arena-api-returns','last-Reset-this':'arena-reset-entries','last-Reset-formals':'arena-reset-entries','last-Reset-return-stack':'arena-reset-returns','last-Reset-nonvolatile':'arena-reset-returns','last-Reset-field':'arena-reset-returns','next-formals':'arena-pool-entries','next-caller':'arena-pool-entries','final-last-arena':'state'}
    if args.original_only:cases=[case for case in cases if case.startswith('original-')]
    core=load('reset_semantic_core','arena_reset_state.py')
    trial=json.loads((original/'trial.json').read_text());text=(original/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M);final='%03d'%int(traps[-1][0]);symbols=subprocess.check_output(['nm','-S','--defined-only',str(native/'native-executable.elf')],text=True)
    started=time.perf_counter();passed=[]
    with tempfile.TemporaryDirectory(prefix='reset-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
        original_receipt=(o/'receipt.json').read_bytes();native_receipt=(n/'receipt.json').read_bytes()
        for case in cases:
            if case.startswith('original-'):
                name='scaler-fp-067.bin'if case=='original-late-FP'else 'reset-return-bss-008.bin';f=o/name;old=f.read_bytes();changed=bytearray(old);changed[4 if 'FP'in case else -1]^=1;f.write_bytes(changed)
            else:
                name=files[case]+'.json';f=n/name;old=f.read_bytes()
                def change(j):
                    if case=='missing-last-call':j.pop()
                    elif case=='last-Reserve-this':j[-1]['ecx']-=32
                    elif case=='last-descriptor':j[-1]['descriptor']-=20
                    elif case=='last-FP-live80':j[85]['fp']['live'][0]['raw80']='0'*20
                    elif case=='last-QWORD-high':j[89]['qword_bytes']=j[89]['qword_bytes'][:8]+'00000000'
                    elif case=='last-rounded-result':j[-1]['eax']+=32768
                    elif case=='regular-API-kind':j[-1]['args'][2]=0x102000
                    elif case=='last-API-stack':j[-1]['esp']+=4
                    elif case=='last-owned-size':j[-1]['owned_reservations'][0]['bytes']-=4096
                    elif case=='last-Reset-this':j[-1]['ecx']-=32
                    elif case=='last-Reset-formals':j[-1]['formal_inventory']=[]
                    elif case=='last-Reset-return-stack':j[-1]['esp']+=4
                    elif case=='last-Reset-nonvolatile':j[-1]['esi']+=4
                    elif case=='last-Reset-field':j[-1]['guard']['bss']=j[-1]['guard']['bss'][:-2]+'01'
                    elif case=='next-formals':j[0]['formal_inventory']=[{'name':'invented','type':'int'}]
                    elif case=='next-caller':j[0]['return_pc']+=1
                    elif case=='final-last-arena':j['frontend']['arenas'][-8]=0
                    else:raise ValueError(case)
                h.edit(f,change);h.rehash(n,name)
            if case.startswith('original-'):
                # Keep the verified original receipt unchanged: these controls target
                # the raw semantic predicates, rather than failing profile closure
                # early merely because changing a receipt changes its reference SHA.
                h.rejected(case,lambda:core.check(d.s,d.d,o,n,trial,text,json.loads((n/'state.json').read_text()),final,symbols,traps))
            else:h.rejected(case,lambda:d.check(o,n,require_audit=False))
            passed.append(case)
            f.write_bytes(old);(o/'receipt.json').write_bytes(original_receipt);(n/'receipt.json').write_bytes(native_receipt)
    print(json.dumps({'scope':'fresh repeated loop/Reset/nextfrontier raw controls only; strict full graph separately mandatory; prior unchanged profile/protocol controls reused','result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
