#!/usr/bin/env python3
"""Focused known-field seed/output controls after a full immutable auxiliary check."""
import importlib.util,json,re,shutil,struct,subprocess,tempfile,time
from pathlib import Path


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    d=load('reset_seed_decoder','check-callback.py');history=load('reset_seed_history','check-history.py');core=load('reset_seed_core','arena_reset_state.py');helpers=load('reset_seed_helpers','check-projection-negative.py')
    row=json.loads((d.s.PROJECT/'cases/minimal/arena-reset-trials.json').read_text())['trials'][1];o,n=Path(row['original']),Path(row['native']);audit=history.configure(d,o,n,saved_record=row);d.check(o,n,audit_path=audit)
    trial=json.loads((o/'trial.json').read_text());text=(o/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M);final='%03d'%int(traps[-1][0]);symbols=subprocess.check_output(['nm','-S','--defined-only',str(n/'native-executable.elf')],text=True);state=json.loads((n/'state.json').read_text());base=state['frontend']['storage']['arenas'][0]-state['frontend']['storage']['saved_state'][0]+8*32
    cases={'incoming-cursor':('arena-reset-entries',0,0),'incoming-committed':('arena-reset-entries',12,0),'outgoing-cursor':('arena-reset-returns',0,0),'outgoing-limit':('arena-reset-returns',4,0),'outgoing-committed':('arena-reset-returns',12,17),'outgoing-reset-value':('arena-reset-returns',24,37)};passed=[];started=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='reset-seed-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        native=Path(tmp)/'native';shutil.copytree(n,native)
        for name,(stem,offset,value)in cases.items():
            path=native/(stem+'.json');saved=path.read_bytes();records=json.loads(saved);raw=bytearray.fromhex(records[-1]['guard']['bss']);struct.pack_into('<I',raw,base+offset,value);records[-1]['guard']['bss']=raw.hex();path.write_text(json.dumps(records))
            # Full protocol/provenance was checked above. Deliberately do not
            # forge receipt hashes: this call isolates semantic-field denials.
            helpers.rejected(name,lambda:core.check(d.s,d.d,o,native,trial,text,state,final,symbols,traps));passed.append(name);path.write_bytes(saved)
    print(json.dumps({'result':'PASS','scope':'six focused auxiliary semantic-field mutations after full immutable protocol+strict audit; no execution','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
