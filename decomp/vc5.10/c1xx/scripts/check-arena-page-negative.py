#!/usr/bin/env python3
"""Two actual page-allocation writes and the pending CommitExtent input ABI."""
import importlib.util,json,re,shutil,struct,tempfile,time
from pathlib import Path


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    d=load('page_decoder','check-callback.py');history=load('page_history','check-history.py');core=load('page_core','pool_bind_state.py');h=load('page_controls','check-projection-negative.py')
    row=json.loads((d.s.PROJECT/'cases/minimal/arena-page-trial.json').read_text());o,n=Path(row['original']),Path(row['native']);audit=history.configure(d,o,n,saved_record=row);d.check(o,n,audit_path=audit)
    text=(o/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M);final='%03d'%int(traps[-1][0]);trial=json.loads((o/'trial.json').read_text());state=json.loads((n/'state.json').read_text());bss=state['frontend']['storage']['saved_state'][0];arena_offset=state['frontend']['storage']['arenas'][0]-bss;out_offset=state['frontend']['storage']['pool_10.remaining'][0]-bss
    cases=['commit-this','commit-formals','commit-caller','native-cursor-store','native-count-store','native-neighbor','original-cursor-store','original-count-store','original-unknown-BSS','serialized-count','serialized-cursor'];passed=[];started=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='arena-page-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        original,native=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(o,original);shutil.copytree(n,native);path=native/'arena-commit-entries.json';saved=path.read_bytes();raw_path=original/'arena-page-bss-frontier.bin';saved_raw=raw_path.read_bytes()
        for case in cases:
            records=json.loads(saved);altered_state=json.loads(json.dumps(state))
            if case=='commit-this':records[0]['ecx']+=32
            elif case=='commit-formals':records[0]['formal_inventory'].append({'name':'invented','type':'int'})
            elif case=='commit-caller':records[0]['return_pc']+=1
            elif case in ('native-cursor-store','native-count-store','native-neighbor'):
                raw=bytearray.fromhex(records[0]['guard']['bss'])
                if case=='native-cursor-store':struct.pack_into('<I',raw,arena_offset,struct.unpack_from('<I',raw,arena_offset)[0]-4096)
                elif case=='native-count-store':struct.pack_into('<I',raw,out_offset,0)
                else:raw[-1]^=1
                records[0]['guard']['bss']=raw.hex()
            elif case.startswith('original-'):
                raw=bytearray(saved_raw)
                if case=='original-cursor-store':struct.pack_into('<I',raw,0xc6500-0xb9000,struct.unpack_from('<I',raw,0xc6500-0xb9000)[0]-4096)
                elif case=='original-count-store':struct.pack_into('<I',raw,0xc0814-0xb9000,0)
                else:raw[-1]^=1
                raw_path.write_bytes(raw)
            elif case=='serialized-count':altered_state['frontend']['pools'][9]['remaining']+=1
            elif case=='serialized-cursor':altered_state['frontend']['arenas'][0]+=4
            path.write_text(json.dumps(records))
            # Baseline complete receipt/source/mask/strict audit above; isolate
            # actual pointer/count/ABI semantic denials without forged receipts.
            h.rejected(case,lambda:core.check(d.s,d.d,original,native,trial,text,altered_state,final,traps));passed.append(case);path.write_bytes(saved);raw_path.write_bytes(saved_raw)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'scope':'eleven affected two-field/next-ABI semantic controls after mandatory full paired strict graph/provenance check, no execution','rejected':passed},indent=2))


if __name__=='__main__':main()
