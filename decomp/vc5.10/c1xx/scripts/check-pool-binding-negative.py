#!/usr/bin/env python3
"""Affected binding-wrapper frontier controls after the full paired proof."""
import importlib.util,json,re,shutil,tempfile,time
from pathlib import Path


def load(name,file):
    s=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m


def main():
    d=load('binding_decoder','check-callback.py');history=load('binding_history','check-history.py');core=load('binding_core','pool_bind_state.py');h=load('binding_controls','check-projection-negative.py')
    row=json.loads((d.s.PROJECT/'cases/minimal/pool-binding-wrapper-trial.json').read_text());o,n=Path(row['original']),Path(row['native']);audit=history.configure(d,o,n,saved_record=row);d.check(o,n,audit_path=audit)
    text=(o/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M);final='%03d'%int(traps[-1][0]);trial=json.loads((o/'trial.json').read_text());state=json.loads((n/'state.json').read_text());cases=['receiver','stack-index','formal-inventory','caller-return','state-write','FP-status','named-stub-argument','original-receiver'];passed=[];started=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='binding-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        native=Path(tmp)/'native';shutil.copytree(n,native);path=native/'pool-bind-entries.json';saved=path.read_bytes()
        for case in cases:
            records=json.loads(saved);altered_state=json.loads(json.dumps(state));altered_text=text
            if case=='receiver':records[0]['ecx']+=16
            elif case=='stack-index':records[0]['args'][0]=1
            elif case=='formal-inventory':records[0]['formal_inventory'].pop()
            elif case=='caller-return':records[0]['return_pc']+=1
            elif case=='state-write':records[0]['guard']['bss']=records[0]['guard']['bss'][:-2]+'01'
            elif case=='FP-status':records[0]['fp']['sw']^=0x20
            elif case=='named-stub-argument':altered_state['args']['arena_index']=1
            elif case=='original-receiver':altered_text=re.sub(r'(rva=00038f94 .*ecx=)104c0810',r'\g<1>104c0b00',text)
            path.write_text(json.dumps(records))
            # This phase intentionally isolates semantic ABI predicates after
            # full immutable receipt/source/mask/strict-CFG validation above.
            h.rejected(case,lambda:core.check(d.s,d.d,o,native,trial,altered_text,altered_state,final,traps));passed.append(case);path.write_bytes(saved)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'scope':'focused semantic frontier controls after mandatory full paired and strict graph check; no binary execution','rejected':passed},indent=2))


if __name__=='__main__':main()
