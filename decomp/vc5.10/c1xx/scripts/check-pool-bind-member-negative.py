#!/usr/bin/env python3
"""One pointer-store and actual refill frontier controls, without execution."""
import importlib.util,json,re,shutil,struct,tempfile,time
from pathlib import Path


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    d=load('bind_member_decoder','check-callback.py');history=load('bind_member_history','check-history.py');core=load('bind_member_core','pool_bind_state.py');h=load('bind_member_controls','check-projection-negative.py')
    row=json.loads((d.s.PROJECT/'cases/minimal/pool-bind-member-trial.json').read_text());o,n=Path(row['original']),Path(row['native']);audit=history.configure(d,o,n,saved_record=row);d.check(o,n,audit_path=audit)
    text=(o/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M);final='%03d'%int(traps[-1][0]);trial=json.loads((o/'trial.json').read_text());state=json.loads((n/'state.json').read_text());offset=state['frontend']['storage']['pool_10.arena'][0]-state['frontend']['storage']['saved_state'][0]
    cases=['refill-this','refill-request','refill-formals','refill-caller','native-pointer-store','typed-arena-identity','original-unknown-BSS','original-pointer-store','original-refill-request'];passed=[];started=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='bind-member-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        original,native=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(o,original);shutil.copytree(n,native)
        path=native/'pool-refill-entries.json';saved=path.read_bytes();raw_path=original/'pool-bind-bss-frontier.bin';saved_raw=raw_path.read_bytes()
        for case in cases:
            records=json.loads(saved);altered_state=json.loads(json.dumps(state));altered_text=text
            if case=='refill-this':records[0]['ecx']+=16
            elif case=='refill-request':records[0]['args'][0]=0
            elif case=='refill-formals':records[0]['formal_inventory'].pop()
            elif case=='refill-caller':records[0]['return_pc']+=1
            elif case=='native-pointer-store':
                raw=bytearray.fromhex(records[0]['guard']['bss']);struct.pack_into('<I',raw,offset,0);records[0]['guard']['bss']=raw.hex()
            elif case=='typed-arena-identity':altered_state['frontend']['pools'][9]['arena']+=32
            elif case.startswith('original-')and case!='original-refill-request':
                raw=bytearray(saved_raw)
                if case=='original-unknown-BSS':raw[-1]^=1
                else:struct.pack_into('<I',raw,0xc0810-0xb9000+12,0x104c6520)
                raw_path.write_bytes(raw)
            elif case=='original-refill-request':altered_text=re.sub(r'(rva=0001ee29 .*stack=10438faa,)00000001',r'\g<1>00000002',text)
            path.write_text(json.dumps(records))
            # Full immutable positive protocol/source/mask/CFG above; raw-only
            # deliberate mutations below isolate the affected semantic checks.
            h.rejected(case,lambda:core.check(d.s,d.d,original,native,trial,altered_text,altered_state,final,traps));passed.append(case);path.write_bytes(saved);raw_path.write_bytes(saved_raw)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'scope':'nine focused pointer-write/next-ABI semantic denials after full immutable paired+strict audit, no binary execution','rejected':passed},indent=2))


if __name__=='__main__':main()
