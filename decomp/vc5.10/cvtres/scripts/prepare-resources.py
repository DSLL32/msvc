#!/usr/bin/env python3
"""Verify two encountered cached .res entries and encode the shared provider."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import struct
import hashlib
BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
def sha(data):return hashlib.sha256(data).hexdigest()
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--configuration',type=Path);a=p.parse_args()
config=json.loads((a.configuration or (BINARY/'functions.json')).read_text())
base=BINARY/'cases/help/resources';proof=json.loads((base/'manifest.json').read_text());source=(BINARY/'src/messages.rc').read_bytes();res=(base/'messages.res').read_bytes()
assert proof['target_sha256']==json.loads((BINARY/'functions.json').read_text())['target']['sha256']
assert sha(source)==proof['source_sha256'] and sha(res)==proof['res_sha256'] and len(res)==proof['res_size']
assert res[:32]==bytes.fromhex('0000000020000000ffff0000ffff000000000000000000000000000000000000')
assert struct.unpack_from('<II',res,32)==(280,32) and struct.unpack_from('<HHHH',res,40)==(65535,6,65535,501)
assert struct.unpack_from('<H',res,54)[0]==1033
literals={int(id):json.loads('"'+text+'"') for id,text in re.findall(r'^\s*(800[34]) "(.*)"$',source.decode(),re.M)}
assert set(literals)=={8003,8004}
entries=[]
for item in proof['entries']:
 length=item['utf16_units'];offset=item['res_offset'];raw=res[offset:offset+2+length*2]
 assert sha(raw)==item['sha256'] and raw.hex()==item['raw_hex'] and struct.unpack_from('<H',raw)[0]==length
 text=raw[2:].decode('utf-16le');assert text==literals[item['id']]
 entries.append(dict(kind='string',id=item['id'],language=item['language'],text=text.encode('ascii')))
assert {e['id'] for e in entries}=={8003,8004}
for path in config.get('additional_resources',[]):
 manifest=BINARY/path;extra=json.loads(manifest.read_text());source=(BINARY/extra['source_path']).read_bytes();res=(manifest.parent/'messages.res').read_bytes()
 assert extra['target_sha256']==proof['target_sha256'] and sha(source)==extra['source_sha256'] and sha(res)==extra['res_sha256'] and len(res)==extra['res_size']
 literals={int(id):json.loads('"'+text+'"') for id,text in re.findall(r'^\s*([0-9]+) "(.*)"$',source.decode(),re.M)}
 assert set(literals)=={row['id'] for row in extra['entries']}
 for row in extra['entries']:
  offset=row['res_offset'];length=row['utf16_units'];raw=res[offset:offset+2+2*length]
  assert raw.hex()==row['raw_hex'] and sha(raw)==row['sha256'] and struct.unpack_from('<H',raw)[0]==length
  text=raw[2:].decode('utf-16le');assert text==literals[row['id']]
  assert all(item['id']!=row['id'] for item in entries),'duplicate encountered string'
  entries.append(dict(kind='string',id=row['id'],language=row['language'],text=text.encode('ascii')))
spec=importlib.util.spec_from_file_location('shared_resources',ROOT/'lib/recon/resources.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
a.output.parent.mkdir(parents=True,exist_ok=True);module.write_resources(a.output,proof['target_sha256'],entries)
print(json.dumps(dict(provider_sha256=sha(a.output.read_bytes()),entries=[e['id'] for e in entries])))
