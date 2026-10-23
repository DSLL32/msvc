#!/usr/bin/env python3
"""Focused repeated pool ABI, real commit protection/bytes and sequence guards."""
import argparse,copy,importlib.util,json,re,shutil,tempfile,time
from pathlib import Path
import pool_cycle_state,default_pool_state

def main():
    p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter()
    spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial);trial.check(a.original,a.native)
    state=json.loads((a.native/'state.json').read_text());candidate=json.loads((a.original/'candidate.json').read_text());fields=json.loads((Path(__file__).resolve().parents[1]/candidate['field_map']).read_text())
    events=(a.original/'events.txt').read_text();module=int(re.findall(r'^module name=c1n32_l.dll actual=([0-9a-f]+)',events,re.M)[0],16);sequence=int(re.findall(r'^trap seq=(\d+)',events,re.M)[-1])
    tests=['missing-event','reordered-events','missing-ninth-binding','ninth-receiver','ninth-output-pointer','missing-return','wrong-return-pointer','callee-saved-register','premature-pool-cursor','wrong-committed-limit','wrong-committed-count','wrong-final-pool-cursor','wrong-parser-argv','missing-parser-formal','missing-api','api-kind','api-size','api-address','api-return','api-stack-cleanup','api-error','api-fp-status','api-formal','api-private-owner','api-writable-tail','original-provider','original-region-protection','original-region-tail','original-ninth-receiver','original-parser-argv','original-committed-first','original-committed-last','native-committed-last','native-final-byte','original-final-byte','observer-point-plan']
    with tempfile.TemporaryDirectory(prefix='pool-cycle-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
        root=Path(temporary);o=root/'original';n=root/'native';shutil.copytree(a.original,o);shutil.copytree(a.native,n)
        tracked=[n/name for name in ['pool-cycle-events.json','pool-arena-entries.json','arena-page-entries.json','pool-cycle-returns.json','pool-commit-api-entries.json','pool-commit-api-returns.json','pool-committed-000.bin','pool-final-committed-000.bin']]+[o/name for name in ['events.txt','pool-committed-000.bin','pool-final-committed-000.bin']];saved={f:f.read_bytes()for f in tracked}
        for test in tests:
            for file,raw in saved.items():file.write_bytes(raw)
            value=copy.deepcopy(state);plan=copy.deepcopy(candidate)
            file=None;rows=None
            if test in ('missing-event','reordered-events'):
                file=n/'pool-cycle-events.json';rows=json.loads(file.read_text())
                if test=='missing-event':rows.pop()
                else:rows[-1],rows[-2]=rows[-2],rows[-1]
            elif test in ('missing-ninth-binding','ninth-receiver'):
                file=n/'pool-arena-entries.json';rows=json.loads(file.read_text())
                if test=='missing-ninth-binding':rows.pop()
                else:rows[8]['ecx']+=16
            elif test=='ninth-output-pointer':
                file=n/'arena-page-entries.json';rows=json.loads(file.read_text());rows[8]['available_bytes']+=4
            elif test in ('missing-return','wrong-return-pointer','callee-saved-register','premature-pool-cursor'):
                file=n/'pool-cycle-returns.json';rows=json.loads(file.read_text())
                if test=='missing-return':rows.pop()
                elif test=='wrong-return-pointer':rows[-3]['eax']+=4096
                elif test=='callee-saved-register':rows[-3]['preserved']['ebx']^=1
                else:rows[1]['compiler_state']['bss']['pools'][8]['cursor']=1
            elif test.startswith('wrong-committed-'):value['compiler_state']['bss']['arenas'][0]['committed_limit'if test.endswith('limit')else 'committed_bytes']+=4096
            elif test=='wrong-final-pool-cursor':value['compiler_state']['bss']['pools'][6]['cursor']+=4096
            elif test=='wrong-parser-argv':value['args']['argv']+=4
            elif test=='missing-parser-formal':value['formal_inventory'].pop()
            elif test in ('missing-api','api-kind','api-size','api-address','api-formal'):
                file=n/'pool-commit-api-entries.json';rows=json.loads(file.read_text())
                if test=='missing-api':rows=[]
                elif test=='api-formal':rows[0]['formal_inventory'].pop()
                else:rows[0]['args'][{'api-address':0,'api-size':1,'api-kind':2}[test]]+=1
            elif test in ('api-return','api-stack-cleanup','api-error','api-fp-status','api-private-owner','api-writable-tail'):
                file=n/'pool-commit-api-returns.json';rows=json.loads(file.read_text())
                if test=='api-return':rows[0]['eax']=0
                elif test=='api-stack-cleanup':rows[0]['esp']-=4
                elif test=='api-error':rows[0]['last_error']=5
                elif test=='api-fp-status':rows[0]['fp']['sw']^=1
                elif test=='api-private-owner':rows[0]['allocations']=[]
                else:rows[0]['maps']=rows[0]['maps'].replace('---p','rw-p')
            elif test in ('original-provider','original-region-protection','original-region-tail','original-ninth-receiver','original-parser-argv'):
                file=o/'events.txt';text=file.read_text()
                if test=='original-provider':text=re.sub(r'^(pool-commit-before .*provider=)[0-9a-f]+',r'\g<1>00000001',text,flags=re.M)
                elif test=='original-region-protection':text=re.sub(r'^(pool-commit-region part=0 .* protect=)[0-9a-f]+',r'\g<1>00000000',text,flags=re.M)
                elif test=='original-region-tail':text=re.sub(r'^(pool-commit-region part=1 .* bytes=)[0-9a-f]+',r'\g<1>00001000',text,flags=re.M)
                elif test=='original-ninth-receiver':
                    matches=list(re.finditer(r'^trap seq=\d+ rva=00012b30 .*$',text,re.M));m=matches[-1];row=re.sub(r'ecx=([0-9a-f]+)',lambda x:'ecx=%08x'%(int(x[1],16)+16),m[0]);text=text[:m.start()]+row+text[m.end():]
                else:
                    m=re.search(r'^trap seq=%d rva=00012b49 .*stack=([0-9a-f,]+)$'%sequence,text,re.M);assert m;words=m[1].split(',');words[2]='%08x'%(int(words[2],16)+4);text=text[:m.start(1)]+','.join(words)+text[m.end(1):]
                file.write_text(text)
            elif test=='observer-point-plan':plan['pool_cycle_observers']['rvas'][-1]+=1
            else:
                side=o if test.startswith('original-')else n;name='pool-final-committed-000.bin'if 'final' in test else 'pool-committed-000.bin';file=side/name;raw=bytearray(file.read_bytes());raw[0 if test.endswith('first')else -1]=1;file.write_bytes(raw)
            if rows is not None:file.write_text(json.dumps(rows))
            try:
                entry,at=default_pool_state.boundary(o,n,value,sequence,module)
                pool_cycle_state.validate(o,n,fields,entry,value,at,sequence,module,plan)
            except(AssertionError,KeyError,ValueError,IndexError):continue
            raise AssertionError('accepted '+test)
    print(json.dumps(dict(status='PASS',fresh_target_controls=len(tests),rejected=tests,seconds=time.perf_counter()-started,scope='Fresh restore positive and affected full nine-member/commit/protection/zero-data guards; normal positive separately fresh; unchanged prefix campaigns reused.'),indent=2))
if __name__=='__main__':main()
