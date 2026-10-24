#!/usr/bin/env python3
"""Explicit offline preparation of the completed timing oracle; never a fixture."""
import argparse,json,sys
from pathlib import Path
from clock_provider import PROJECT,response_packet,digest

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('original',type=Path)
args=parser.parse_args();case=PROJECT/'cases/link-timing';reference=case/'reference'
master=json.loads((reference/'fixture.json').read_bytes())
capture=(args.original/'capture.json').read_bytes()
assert digest(capture)==master['capture_sha256']
packet=response_packet(args.original)
assert packet['clock']['expected_calls']==2 and packet['parent']['status']==0
raw=(json.dumps(packet,indent=2)+'\n').encode()
packet_path=case/'dependency-packet.json'
if packet_path.exists():assert packet_path.read_bytes()==raw,'never replace a frozen packet'
else:packet_path.write_bytes(raw)
sources={}
for module in list(sys.modules.values()):
    filename=getattr(module,'__file__',None)
    if not filename:continue
    path=Path(filename).resolve()
    if path.parent==PROJECT/'scripts'and path.suffix=='.py':sources[path.name]=path
sources[Path(__file__).name]=Path(__file__).resolve()
records={};owned=reference/'packet-sources';owned.mkdir(exist_ok=True)
for name,path in sorted(sources.items()):
    content=path.read_bytes();target=owned/name
    if target.exists():assert target.read_bytes()==content,'never replace frozen decoder sources'
    else:target.write_bytes(content)
    records[name]={'file':'packet-sources/'+name,'sha256':digest(content),'bytes':len(content),
        'historical_source':'decomp/vc5.10/cl/scripts/'+name}
provenance={'schema':1,'kind':'explicit offline raw-derived clock response',
    'packet_sha256':digest(raw),'capture_sha256':digest(capture),'sources':records}
path=reference/'packet-provenance.json';content=(json.dumps(provenance,indent=2)+'\n').encode()
if path.exists():assert path.read_bytes()==content,'never replace frozen provenance'
else:path.write_bytes(content)
master['clock_response_sha256']=digest(raw)
master['clock_response_provenance']={'file':path.name,'sha256':digest(content)}
(reference/'fixture.json').write_text(json.dumps(master,indent=2)+'\n')
print(json.dumps({'packet_sha256':digest(raw),'provenance_sha256':digest(content),'decoder_sources':len(records)}))
