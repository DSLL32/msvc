#!/usr/bin/env python3
"""New FP/profile/conversion/allocation controls; no execution or promotion."""
import importlib.util,json,re,shutil,tempfile,time
from pathlib import Path


def load(name,file):
    s=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m


def main():
    d=load('scaler_negative_decoder','check-callback.py');h=load('scaler_negative_helpers','check-projection-negative.py');packet=json.loads((d.s.PROJECT/'cases/minimal/scaler-trials.json').read_text());row=packet['trials'][0];original,native=Path(row['original']),Path(row['native'])
    d.s.require(d.s.sha(original/'receipt.json')==row['original_receipt_sha256'] and d.s.sha(native/'receipt.json')==row['native_receipt_sha256'],'not immutable declared fresh pair');d.s.HISTORICAL_SNAPSHOT=native;d.check(original,native,require_audit=False)
    cases=['profile-reference','profile-count','initial-status','initial-reason','initial-reference','FP-live80','FP-status','FP-tag','extra-operand','extra-depth','truncation-control','QWORD-high','QWORD-location','rounded-result','cdecl-return-stack','FP-state-write','API-argument','API-return-stack','API-error','API-nonvolatile','reservation-owner-size','reservation-protection','Reset-this','Reset-formals','original-provider','original-FP','original-unknown-BSS','original-final-BSS'];passed=[];started=time.perf_counter()
    files={'profile-reference':'scaler-profile','profile-count':'scaler-profile','initial-status':'scaler-initial','initial-reason':'scaler-initial','initial-reference':'scaler-initial','rounded-result':'scaler-returns','cdecl-return-stack':'scaler-returns','API-argument':'arena-api-entries','API-return-stack':'arena-api-returns','API-error':'arena-api-returns','API-nonvolatile':'arena-api-returns','reservation-owner-size':'arena-api-returns','reservation-protection':'arena-api-returns','Reset-this':'arena-reset-entries','Reset-formals':'arena-reset-entries'}
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='scaler-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
            o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
            if case.startswith('original-'):
                name={'original-provider':'ftol-provider-code.bin','original-FP':'scaler-fp-003.bin','original-unknown-BSS':'scaler-bss-004.bin','original-final-BSS':'arena-bss-reset-frontier.bin'}[case];f=o/name;blob=bytearray(f.read_bytes());blob[-1]^=1;f.write_bytes(blob);h.rehash(o,name)
            else:
                name=files.get(case,'scaler-fpu')+'.json';f=n/name
                def change(j):
                    if case=='profile-reference':j[0]['args'][0]='0'*64
                    elif case=='profile-count':j[0]['applications']=1
                    elif case=='initial-status':j[0]['fp']['sw']=0
                    elif case=='initial-reason':j[0]['args'][1]=0
                    elif case=='initial-reference':j[0]['profile']['reference']='0'*64
                    elif case=='FP-live80':j[5]['fp']['live'][0]['raw80']='0'*20
                    elif case=='FP-status':j[5]['fp']['sw']^=0x20
                    elif case=='FP-tag':j[5]['fp']['tag']=0xffff
                    elif case=='extra-operand':j[2]['fp']['registers'][0]='0'*20
                    elif case=='extra-depth':j[2]['fp']['live'].pop()
                    elif case=='truncation-control':j[7]['fp']['cw']=0x027f
                    elif case=='QWORD-high':j[-1]['qword_bytes']=j[-1]['qword_bytes'][:8]+'00000000'
                    elif case=='QWORD-location':j[-1]['qword_address']+=4
                    elif case=='rounded-result':j[0]['eax']+=65536
                    elif case=='cdecl-return-stack':j[0]['esp']+=4
                    elif case=='FP-state-write':j[3]['guard']['bss']=j[3]['guard']['bss'][:-2]+'01'
                    elif case=='API-argument':j[0]['args'][2]=0x2000
                    elif case=='API-return-stack':j[0]['esp']+=4
                    elif case=='API-error':j[0]['last_error']=0
                    elif case=='API-nonvolatile':j[0]['edi']+=4
                    elif case=='reservation-owner-size':j[0]['owned_reservations'][0]['bytes']-=65536
                    elif case=='reservation-protection':j[0]['maps']=j[0]['maps'].replace('---p','rw-p')
                    elif case=='Reset-this':j[0]['ecx']+=32
                    elif case=='Reset-formals':j[0]['formal_inventory'].append({'name':'invented','type':'unsigned int'})
                    else:raise ValueError(case)
                h.edit(f,change);h.rehash(n,name)
            h.rejected(case,lambda:d.check(o,n,require_audit=False));passed.append(case)
    print(json.dumps({'scope':'raw FP/profile/ABI/state/allocation mutations only; current strict full audit separately mandatory','result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
