#!/usr/bin/env python3
"""One-pointer arena write and genuine next cdecl input, no binary execution."""
import importlib.util,json,re,shutil,tempfile,time
from pathlib import Path


def load(name,file):
    s=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m


def main():
    d=load('arena_reserve_decoder','check-callback.py');h=load('arena_reserve_controls','check-projection-negative.py');row=json.loads((d.s.PROJECT/'cases/minimal/arena-reserve-trial.json').read_text());original,native=Path(row['original']),Path(row['native']);d.s.require(d.s.sha(original/'receipt.json')==row['original_receipt_sha256'] and d.s.sha(native/'receipt.json')==row['native_receipt_sha256'],'not immutable declared raw pair');d.s.HISTORICAL_SNAPSHOT=native;d.check(original,native,require_audit=False)
    cases=['native-descriptor-store','native-neighbor','native-calculator-argument','native-calculator-raw','native-calculator-formal','native-calculator-caller','original-descriptor-store','original-BSS-neighbor','original-calculator-argument'];passed=[];started=time.perf_counter()
    for case in cases:
        with tempfile.TemporaryDirectory(prefix='arena-reserve-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
            o,n=Path(tmp)/'original',Path(tmp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
            if case.startswith('native-'):
                f=n/('arena-size-entry.json' if 'calculator' in case else 'state.json')
                def change(j):
                    if case=='native-descriptor-store':j['frontend']['arenas'][4]+=20
                    elif case=='native-neighbor':j['frontend']['arenas'][5]=23
                    elif case=='native-calculator-argument':j['args']['descriptor']+=20
                    elif case=='native-calculator-raw':j['raw_entry_abi']['stack_arguments'][0]+=20
                    elif case=='native-calculator-formal':j['formal_inventory'].clear()
                    elif case=='native-calculator-caller':j['raw_entry_abi']['return_pc']+=1
                h.edit(f,change);h.rehash(n,f.name)
            else:
                if case=='original-calculator-argument':
                    f=o/'events.txt'
                    def change(m):
                        words=m[2].split(',');words[1]='104d5b0c';return m[1]+','.join(words)
                    text,count=re.subn(r'(^trap seq=\d+ rva=00038ed2[^\n]*stack=)([0-9a-f,]+)',change,f.read_text(),count=1,flags=re.M)
                    if count!=1:raise ValueError('missing original calculator input')
                    f.write_text(text)
                else:
                    f=o/('arenas-%03d.bin'%int(re.findall(r'^trap seq=(\d+) ',(o/'events.txt').read_text(),re.M)[-1]) if case=='original-descriptor-store' else 'arena-bss-size-frontier.bin');blob=bytearray(f.read_bytes());blob[16 if case=='original-descriptor-store' else -1]^=1;f.write_bytes(blob)
                h.rehash(o,f.name)
            h.rejected(case,lambda:d.check(o,n,require_audit=False));passed.append(case)
    print(json.dumps({'scope':'raw semantic/ABI mutations only; strict current audit required separately before promotion','result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
