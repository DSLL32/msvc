#!/usr/bin/env python3
"""Actual page-count/output-pointer frontier denials after a full immutable pair."""
import importlib.util,json,re,shutil,tempfile,time
from pathlib import Path


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    d=load('refill_decoder','check-callback.py');history=load('refill_history','check-history.py');core=load('refill_core','pool_bind_state.py');h=load('refill_controls','check-projection-negative.py')
    row=json.loads((d.s.PROJECT/'cases/minimal/pool-refill-trial.json').read_text());o,n=Path(row['original']),Path(row['native']);audit=history.configure(d,o,n,saved_record=row);d.check(o,n,audit_path=audit)
    text=(o/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M);final='%03d'%int(traps[-1][0]);trial=json.loads((o/'trial.json').read_text());state=json.loads((n/'state.json').read_text());cases=['pages','output-pointer','output-initial-value','formal-inventory','arena-receiver','caller-return','native-state-write','original-output-pointer','original-unknown-BSS','named-output-alias'];passed=[];started=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='refill-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        original,native=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(o,original);shutil.copytree(n,native);path=native/'arena-page-entries.json';saved=path.read_bytes();raw_path=original/'pool-refill-bss-frontier.bin';saved_raw=raw_path.read_bytes()
        for case in cases:
            records=json.loads(saved);altered_state=json.loads(json.dumps(state));altered_text=text
            if case=='pages':records[0]['args'][0]=2
            elif case=='output-pointer':records[0]['args'][1]+=4
            elif case=='output-initial-value':records[0]['output_value']=17
            elif case=='formal-inventory':records[0]['formal_inventory'].pop()
            elif case=='arena-receiver':records[0]['ecx']+=32
            elif case=='caller-return':records[0]['return_pc']+=1
            elif case=='native-state-write':records[0]['guard']['bss']=records[0]['guard']['bss'][:-2]+'01'
            elif case=='original-output-pointer':altered_text=re.sub(r'(rva=0001ee58 .*stack=1041ee52,00000001,)104c0814',r'\g<1>104c0818',text)
            elif case=='original-unknown-BSS':raw=bytearray(saved_raw);raw[-1]^=1;raw_path.write_bytes(raw)
            elif case=='named-output-alias':altered_state['args']['remaining_bytes']+=4
            path.write_text(json.dumps(records))
            # Full raw/provenance/CFG above, then deliberately isolate each new
            # semantic argument/pointee/whole-state predicate without forging receipts.
            h.rejected(case,lambda:core.check(d.s,d.d,original,native,trial,altered_text,altered_state,final,traps));passed.append(case);path.write_bytes(saved);raw_path.write_bytes(saved_raw)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'scope':'ten affected page/out-pointer semantic controls after mandatory full paired strict graph/provenance validation; no binary execution','rejected':passed},indent=2))


if __name__=='__main__':main()
