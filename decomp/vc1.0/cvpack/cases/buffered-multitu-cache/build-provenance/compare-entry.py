from pathlib import Path
import sys,json,re,copy
root=Path('/root/msvc2/decomp/vc1.0/cvpack');sys.path.insert(0,str(root/'scripts'));from objects import original_walks
from state import sha
out=root/'cases/buffered-multitu-cache/evidence/0186-entry-owners';o=Path('/root/.ai-tmp/vc1.0-cvpack/0186-entry-owners-original');n=Path('/root/.ai-tmp/vc1.0-cvpack/0186-entry-owners-native');events=(o/'events').read_text();spec=next(x for x in json.loads((root/'walks.json').read_text())if x['name']=='used_pages');old=original_walks(events,[spec])['used_pages'];new=json.loads((n/'entry.json').read_text());data=json.loads((root/'data.json').read_text());defs={x['name']:x for x in data}
def scalar(name):
 address=defs[name]['original'];rows=re.findall(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M);assert len(rows)==1 and len(bytes.fromhex(rows[0]))==4;return int.from_bytes(bytes.fromhex(rows[0]),'little')
oldroots={k:scalar(k)for k in('free_pages','used_pages','oldest_page','newest_page','page_count')}
def summarize(nodes,roots):
 assert len(nodes)==roots['page_count']==16 and roots['free_pages']==0 and roots['used_pages']==nodes[0]['address']
 ids={r['address']:i for i,r in enumerate(nodes)};assert len(ids)==16
 identify=lambda address:None if address==0 else ids[address]
 result=[]
 for index,r in enumerate(nodes):
  raw=bytes.fromhex(r['bytes']);assert len(raw)==4136
  word=lambda offset:int.from_bytes(raw[offset:offset+4],'little')
  assert word(4124)==(nodes[index+1]['address']if index+1<len(nodes) else 0)
  assert 24<=word(4120)-r['address']<=4120
  result.append(dict(owner=index,defined_header=raw[:24].hex(),cursor_offset=word(4120)-r['address'],next=identify(word(4124)),older=identify(word(4128)),newer=identify(word(4132)),flags=word(20)))
 oldest,newest=identify(roots['oldest_page']),identify(roots['newest_page']);chain=[];p=oldest;previous=None
 while p is not None:
  assert p not in chain and result[p]['older']==previous
  chain.append(p);previous,p=p,result[p]['newer']
 assert previous==newest and len(chain)==16
 eligible=[p for p in chain if not(result[p]['flags']&16)];assert eligible
 return dict(owners=result,oldest=oldest,newest=newest,lru_chain=chain,selected=eligible[0])
a,b=summarize(old,oldroots),summarize(new['nodes'],new['roots']);assert a==b,'definedPage input graph differs'
controls=[]
m=copy.deepcopy(new)
m['nodes'][0]['bytes']=m['nodes'][0]['bytes'][:4128*2]+'00'*4+m['nodes'][0]['bytes'][4132*2:]
try:assert summarize(m['nodes'],m['roots'])==a
except(AssertionError,KeyError):controls.append('changed-defined-link')
else:raise AssertionError('changed link accepted')
m=copy.deepcopy(new)
for r in m['nodes']:
 raw=bytearray.fromhex(r['bytes']);flags=int.from_bytes(raw[20:24],'little')|16;raw[20:24]=flags.to_bytes(4,'little');r['bytes']=raw.hex()
try:summarize(m['nodes'],m['roots'])
except AssertionError:controls.append('all-pinned')
else:raise AssertionError('all-pinned accepted')
import shutil
for label,d,names in [('original',o,['events','receipt.json','configuration.json','runtime-support.json']),('native',n,['entry.json','entry.gdb','receipt.json','gdb.stdout','gdb.stderr'])]:
 for name in names:shutil.copy2(d/name,out/(label+'-'+name))
proof=dict(scope='Fresh bounded first eviction ENTRY observation, separate from0186 complete-output proof. Actual16used Page owners from new_page calloc4136/16 fixed files_initialize pool; fullraw retained, only defined24B headers/cursor identity/full list links compared. No payload equality or return observer claim.',configuration_sha256=sha(root/'functions.json'),native_sha256=sha(root/'build/native/CVPACK.EXE'),original_roots=oldroots,native_roots=new['roots'],matched=a,selected_actual=dict(original=old[a['selected']]['address'],native=new['nodes'][b['selected']]['address'],flags=a['owners'][a['selected']]['flags']),native_current_module=new['current_module'],controls=controls,raw_sha256={p.name:sha(p)for p in out.iterdir()if p.is_file()})
(out/'proof.json').write_text(json.dumps(proof,indent=2)+'\n');print('16owner definedheader/fullLRUlinks match; selected',a['selected'],'flags',a['owners'][a['selected']]['flags'],'controls',controls)
