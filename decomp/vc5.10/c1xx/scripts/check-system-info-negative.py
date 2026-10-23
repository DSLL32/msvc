#!/usr/bin/env python3
"""Affected CRT namespace, GetSystemInfo capability, ABI and output controls."""
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import tempfile
import time


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module


def main():
    d=load('system_decoder','check-callback.py');h=load('system_negative','check-projection-negative.py');history=load('system_history','check-history.py');packet=json.loads((d.s.PROJECT/'cases/minimal/system-info-trials.json').read_text());started=time.perf_counter();passed=[]
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        cases=['unbound-getenv','Invoke-argc-as-CRT','nonNULL-getenv','wrong-environment-identity','wrong-CRT-argv-identity','changed-CRT-argument','changed-CRT-environment','wrong-API-pointer','wrong-API-formal','wrong-API-RET4','wrong-API-register','second-API-response','memory-capability','changed-profile-byte','changed-native-output','changed-native-neighbor','changed-saved-snapshot'] if row['seed'] is None else ['wrong-before-input','partial-output','native-neighbor']
        for case in cases:
            with tempfile.TemporaryDirectory(prefix='system-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temp:
                o,n=Path(temp)/'original',Path(temp)/'native';shutil.copytree(original,o);shutil.copytree(native,n)
                if case in ('changed-CRT-argument','changed-CRT-environment','changed-profile-byte'):
                    name='native-crt-argv.bin' if case=='changed-CRT-argument' else 'native-crt-environment.bin' if case=='changed-CRT-environment' else 'system-info-profile.bin';f=n/name;raw=bytearray(f.read_bytes());raw[16 if name.endswith('argv.bin') or name.endswith('environment.bin') else -1]^=1;f.write_bytes(raw);h.rehash(n,name)
                else:
                    name='target-getenv-entries.json' if case in ('unbound-getenv','Invoke-argc-as-CRT','wrong-environment-identity','wrong-CRT-argv-identity') else 'target-getenv-returns.json' if case=='nonNULL-getenv' else 'system-info-entries.json' if case in ('wrong-API-pointer','wrong-API-formal','memory-capability') else 'system-info-returns.json' if case in ('wrong-API-RET4','wrong-API-register','second-API-response') else 'system-info-entry.json' if case=='wrong-before-input' else 'state.json'
                    def change(j):
                        if case=='unbound-getenv':j[0]['bound']=0
                        elif case=='Invoke-argc-as-CRT':j[0]['argc']=26
                        elif case=='nonNULL-getenv':j[0]['eax']=1
                        elif case=='wrong-environment-identity':j[0]['environment']['address']+=4
                        elif case=='wrong-CRT-argv-identity':j[0]['argv']['address']+=4
                        elif case=='wrong-API-pointer':j[0]['argument']+=4
                        elif case=='wrong-API-formal':j[0]['formal_inventory']=[]
                        elif case=='wrong-API-RET4':j[0]['esp']+=4
                        elif case=='wrong-API-register':j[0]['edi']+=1
                        elif case=='second-API-response':j[0]['profile']['system_calls']=2
                        elif case=='memory-capability':j[0]['profile']['bound']=1
                        elif case in ('changed-native-output','partial-output','wrong-before-input'):j['frontend']['system_info'][1]+=1
                        elif case=='changed-saved-snapshot':j['frontend']['saved_system_info'][1]+=1
                        elif case in ('changed-native-neighbor','native-neighbor'):j['frontend']['source_signature']^=1
                        else:raise ValueError(case)
                    h.edit(n/name,change);h.rehash(n,name)
                h.rejected(case,lambda:d.check(o,n,audit_path=audit));passed.append({'seed':row['seed'],'rejected':case})
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__=='__main__':main()
