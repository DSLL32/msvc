#!/usr/bin/env python3
"""Reject token-prefix, discriminator, original-tail and native-layout divergence."""
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile


def load(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    d=load('token_decoder','check-callback.py');h=load('token_helpers','check-projection-negative.py');history=load('token_history','check-history.py')
    packet=json.loads((d.s.PROJECT/'cases/minimal/token-copy-trials.json').read_text());cases=('source-low-word','source-high-word','cache-low-word','cache-high-word','kind-not-set','wrong-old-kind','old-cache-not-distinct','nonzero-reset-missing','BSS-not-cleared','not-union','wrong-pointer-width','native-placeholder-tail','native-source-cache-alias','wrong-sentinel-layout','native-own-padding-changed','original-short-payload','original-tail-not-copied','original-source-tail-mutated','original-header-gap-mutated','false-entry-PC','false-frontier-PC')
    tested=[]
    for variant in ('positive','negative'):
        row=next(r for r in packet['trials'] if r['integer_arm']==variant);original,native=Path(row['original']),Path(row['native']);audit=history.configure(d,original,native,saved_record=row);d.check(original,native,audit_path=audit)
        for case in cases:
            with tempfile.TemporaryDirectory(prefix='token-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temporary:
                root=Path(temporary);o=root/'original';n=root/'native';shutil.copytree(original,o);shutil.copytree(native,n)
                if case.startswith('original-'):
                    traps=d.re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',(o/'events.txt').read_text(),d.re.M);suffix='%03d'%int(traps[-1][0])
                    name=('token-source-' if case=='original-source-tail-mutated' else 'sentinel-header-' if case=='original-header-gap-mutated' else 'sentinel-payload-')+suffix+'.bin'
                    path=o/name;raw=bytearray(path.read_bytes())
                    if case=='original-short-payload':raw=raw[:32]
                    else:raw[1 if case=='original-header-gap-mutated' else 8]^=1
                    path.write_bytes(raw);h.rehash(o,name)
                else:
                    name='value-copy-entry.json' if case in ('wrong-old-kind','old-cache-not-distinct','false-entry-PC') else 'pre.json' if case=='nonzero-reset-missing' else 'phase.json' if case=='BSS-not-cleared' else 'state.json'
                    def change(j):
                        f=j['frontend']
                        if case=='source-low-word':f['token_value']^=1
                        elif case=='source-high-word':f['token_value']^=1<<32
                        elif case=='cache-low-word':f['sentinel_token']['value']^=1
                        elif case=='cache-high-word':f['sentinel_token']['value']^=1<<32
                        elif case=='kind-not-set':f['sentinel_token']['kind']=0
                        elif case=='wrong-old-kind':f['sentinel_token']['kind']=1
                        elif case=='old-cache-not-distinct':f['sentinel_token']['value']=f['token_value']
                        elif case=='nonzero-reset-missing':f['token_value']=0
                        elif case=='BSS-not-cleared':f['sentinel_token']['kind']=165
                        elif case=='not-union':f['token_value_layout']['is_union']=False
                        elif case=='wrong-pointer-width':f['token_value_layout']['fields']['pointer'][1]=8
                        elif case=='native-placeholder-tail':f['declared_fields']['bss'].append('unknown_payload')
                        elif case=='native-source-cache-alias':f['storage']['sentinel_token.value'][0]=f['storage']['token_value'][0]
                        elif case=='wrong-sentinel-layout':f['sentinel_native_layout']['fields']['value'][0]=8
                        elif case=='native-own-padding-changed':f['sentinel_padding']='a5a5a5'
                        else:j['pc']+=1
                    h.edit(n/name,change);h.rehash(n,name)
                h.rejected(variant+'/'+case,lambda:d.check(o,n,audit_path=audit));tested.append(variant+'/'+case)
    print(json.dumps({'result':'PASS','count':len(tested),'rejected':tested},indent=2))


if __name__=='__main__':main()
