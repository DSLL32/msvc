#!/usr/bin/env python3
"""Affected ten-pool/commit/return/frontier denials; no binary execution."""
import copy,importlib.util,json,re,shutil,struct,tempfile,time
from pathlib import Path


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    d=load('cycle_decoder','check-callback.py');history=load('cycle_history','check-history.py');core=load('cycle_core','pool_cycle_state.py');h=load('cycle_controls','check-projection-negative.py')
    packet=Path('cases/minimal/arena-commit-trial.json');record=json.loads(packet.read_text());o,n=Path(record['original']),Path(record['native']);audit=history.configure(d,o,n,saved_record=record);d.check(o,n,audit_path=audit)
    text=(o/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M);final='%03d'%int(traps[-1][0]);trial=json.loads((o/'trial.json').read_text());state=json.loads((n/'state.json').read_text())
    cases=['chronology','missing-tenth-call','later-this','later-output-alias','commit-formals','return-stack','return-nonvolatile','original-nonvolatile','native-neighbor','original-unknown-BSS','API-kind','API-return-stack','owned-reservation','native-protection','original-region','committed-payload','next-argv','next-formals','serialized-pool','original-initialized-neighbor','observer-range'];passed=[];started=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='pool-cycle-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        original,native=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(o,original);shutil.copytree(n,native)
        for case in cases:
            changed=[];altered_state=copy.deepcopy(state);altered_trial=copy.deepcopy(trial);altered_text=text
            def change_json(name,change):
                path=native/(name+'.json');old=path.read_bytes();value=json.loads(old);change(value);path.write_text(json.dumps(value));changed.append((path,old))
            def change_raw(path,offset):
                old=path.read_bytes();value=bytearray(old);value[offset]^=1;path.write_bytes(value);changed.append((path,old))
            if case=='chronology':change_json('pool-cycle-events',lambda v:v.__setitem__(slice(0,2),v[:2][::-1]))
            elif case=='missing-tenth-call':change_json('pool-bind-entries',lambda v:v.pop())
            elif case=='later-this':change_json('pool-bind-entries',lambda v:v[8].update(ecx=v[8]['ecx']+16))
            elif case=='later-output-alias':change_json('arena-page-entries',lambda v:v[7]['args'].__setitem__(1,v[7]['args'][1]+4))
            elif case=='commit-formals':change_json('arena-commit-entries',lambda v:v[0]['formal_inventory'].append(dict(name='invented',type='int')))
            elif case=='return-stack':change_json('pool-cycle-returns',lambda v:v[-1].update(esp=v[-1]['esp']+4))
            elif case=='return-nonvolatile':change_json('pool-cycle-returns',lambda v:v[-1].update(ebp=v[-1]['ebp']+4))
            elif case=='original-nonvolatile':
                altered_text=re.sub(r'(pool-cycle point=14 sample=32 .*ebp=)([0-9a-f]+)',lambda m:m[1]+'%08x'%(int(m[2],16)+4),text);assert altered_text!=text
            elif case=='native-neighbor':change_json('pool-cycle-returns',lambda v:v[12]['guard'].update(bss=v[12]['guard']['bss'][:-2]+('01'if v[12]['guard']['bss'][-2:]!='01'else'02')))
            elif case=='original-unknown-BSS':change_raw(original/'pool-cycle-bss-010.bin',63131)
            elif case=='API-kind':change_json('pool-commit-api-entries',lambda v:v[0]['args'].__setitem__(2,0x2000))
            elif case=='API-return-stack':change_json('pool-commit-api-returns',lambda v:v[0].update(esp=v[0]['esp']+4))
            elif case=='owned-reservation':change_json('pool-commit-api-returns',lambda v:v[0]['owned_reservations'].pop())
            elif case=='native-protection':change_json('pool-commit-api-returns',lambda v:v[0].update(maps=v[0]['maps'].replace('rw-p','r--p')))
            elif case=='original-region':change_raw(original/'pool-commit-region-0.bin',12)
            elif case=='committed-payload':change_raw(native/'pool-committed.bin',65535)
            elif case=='next-argv':change_json('pool-cli-entries',lambda v:v[0]['args'].__setitem__(1,v[0]['args'][1]+4))
            elif case=='next-formals':change_json('pool-cli-entries',lambda v:v[0]['formal_inventory'].pop())
            elif case=='serialized-pool':altered_state['frontend']['pools'][8]['remaining']+=1
            elif case=='original-initialized-neighbor':change_raw(original/'pool-cycle-data-010.bin',0)
            elif case=='observer-range':altered_trial['pool_cycle_observers']['rvas'][-1]+=1
            else:raise RuntimeError(case)
            # Full raw/source/CFG proof is checked above. These copies isolate
            # the affected semantic predicates without forging raw receipts.
            h.rejected(case,lambda:core.check(d.s,d.d,original,native,altered_trial,altered_text,altered_state,final,traps));passed.append(case)
            for path,old in changed:path.write_bytes(old)
    print(json.dumps(dict(result='PASS',count=len(passed),seconds=time.perf_counter()-started,scope='fresh affected semantic controls after complete paired strict audit; no execution; unchanged earlier protocol/profile controls reused by pinned evidence reference',rejected=passed),indent=2))


if __name__=='__main__':main()
