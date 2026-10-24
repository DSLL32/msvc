#!/usr/bin/env python3
"""Compare the two fixed read-only metadata-owner observations offline."""
import argparse,hashlib,json,re
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def compare(original,native):
    log=(original/'events').read_text();state=json.loads((native/'state.json').read_text())
    assert state['status']=='natural-context-proved'
    def match(pattern):
        value=re.search(pattern,log,re.M);assert value,pattern;return value.groups()
    pages=[]
    for index,start,marks,bits in re.findall(r'^metadata-page (\d+) start ([0-9a-f]+) bytes0000effc marks ([0-9a-f]+) bits (\d+)$',log,re.M):
        pages.append(dict(index=int(index),start=int(start,16),marks=int(marks,16),bits=int(bits),bytes=0xeffc))
    blocks=[]
    for index,address,allocation,data,used,next,last in re.findall(r'^metadata-arena-block (\d+) address ([0-9a-f]+) allocation (\d+) data ([0-9a-f]+) used (\d+) next ([0-9a-f]+) last (\d+)$',log,re.M):
        assert int(index)==len(blocks)
        blocks.append(dict(address=int(address,16),allocation=int(allocation),data=int(data,16),used=int(used),next=int(next,16),last=int(last)))
    assert blocks and blocks[-1]['last']==1 and blocks[-1]['next']==0
    assert len(pages)==len(state['pages']) and len(blocks)==len(state['arena_blocks'])
    assert [(p['index'],p['bits'],p['bytes'])for p in pages]==[(p['index'],p['bits'],p['bytes'])for p in state['pages']]
    assert [(b['allocation'],b['used'],b['last'])for b in blocks]==[(b['allocation'],b['used'],b['last'])for b in state['arena_blocks']]
    def owner(address,size,side):
        if not address:return None
        for index,page in enumerate(side['pages']):
            if page['start']<=address and address+size<=page['start']+page['bytes']:
                return ['permanent',index,address-page['start']]
        for index,block in enumerate(side['blocks']):
            if block['data']<=address and address+size<=block['data']+block['used']:
                assert all(not(address<page['start']+page['bytes']and address+size>page['start'])for page in side['pages'])
                return ['temporary',index,address-block['data']]
        raise AssertionError(('address has no bounded owner',hex(address),size))
    sides=[dict(pages=pages,blocks=blocks),dict(pages=state['pages'],blocks=state['arena_blocks'])]
    marker,esp,call,context,symbol,count=match(r'^metadata-context marker ([0-9a-f]+) esp ([0-9a-f]+) call ([0-9a-f]+) context ([0-9a-f]+) symbol ([0-9a-f]+) arguments3 pages (\d+)$')
    assert int(count)==len(pages) and int(marker,16)==0x2c220
    assert owner(int(call,16),20,sides[0])==owner(state['call'],20,sides[1])
    assert owner(int(context,16),36,sides[0])==owner(state['context'],36,sides[1])
    assert owner(int(symbol,16),4,sides[0])==owner(state['symbol'],4,sides[1])
    assert match(r'^metadata-string callee [0-9a-f]+ (.*)$')[0]==state['callee']=='metadata_sink'
    arguments=[]
    rows=re.findall(r'^metadata-argument (\d+) address ([0-9a-f]+) type ([0-9a-f]+) next ([0-9a-f]+) expression ([0-9a-f]+) previous ([0-9a-f]+) owned20 outside-all-pages$',log,re.M)
    assert len(rows)==len(state['arguments'])==3
    for row,actual in zip(rows,state['arguments']):
        index,*addresses=row;assert int(index)==actual['index']
        projected={}
        for name,address in zip(['address','type','next','expression','previous'],addresses):
            size=20 if name in ('address','next','previous')else 8 if name=='type'else 1
            value=owner(int(address,16),size,sides[0]);assert value==owner(actual[name],size,sides[1]);projected[name]=value
        arguments.append(projected)
    vectors={}
    for label,name in [('defines','pch_saved_defines'),('directories','pch_saved_include_directories')]:
        address,count,capacity,arena,items=match(r'^metadata-vector '+label+r' ([0-9a-f]+) count (\d+) capacity (\d+) arena (\d+) items ([0-9a-f]+)$')
        actual=state['vectors'][name];assert (int(count),int(capacity),int(arena))==(actual['count'],actual['capacity'],actual['arena'])
        assert owner(int(address,16),12,sides[0])==owner(actual['address'],12,sides[1])
        assert owner(int(items,16),4*int(count),sides[0])==owner(actual['items'],4*int(count),sides[1])
        values=[]
        for index in range(int(count)):
            pointer,text=match(r'^metadata-string '+label+'-'+str(index)+r' ([0-9a-f]+) (.*)$')
            assert text==actual['names'][index];value=owner(int(pointer,16),len(text)+1,sides[0]);assert value==owner(actual['values'][index],len(text)+1,sides[1]);values.append(value)
        vectors[label]=dict(count=int(count),capacity=int(capacity),arena=int(arena),names=actual['names'],references=values)
    stream,pending,flags,template=match(r'^metadata-browser stream ([0-9a-f]+) pending (\d+) flags ([0-9a-f]+) template(.*)$')
    assert (int(pending),int(flags,16),[int(v,16)for v in template.split()])==(state['browser']['pending'],state['browser']['flags'],state['browser']['template'])
    names=['expression_file','symbol_file','line_file','global_file','debug_file','pch_file','browser_output']
    for index,name in enumerate(names):
        size,data=match(r'^buffer '+str(index)+r' (\d+) ([0-9a-f]*)$');assert int(size)*2==len(data)and data==state['buffers'][name]
    work=Path(json.loads((native/'receipt.json').read_text())['work'])
    for path in (original/'work').iterdir():
        assert path.is_file()and (work/path.name).is_file()and path.read_bytes()==(work/path.name).read_bytes(),path.name
    return dict(status='pass',scope='Read-only natural owner proof; no helper called and no new accepted function.',pages=len(pages),arena_allocations=[dict(bytes=b['allocation'],used=b['used'])for b in blocks],arguments=arguments,vectors=vectors,browser=dict(pending=int(pending),flags=int(flags,16),template=state['browser']['template']),files_compared=len(list((original/'work').iterdir())),original_events_sha256=sha(original/'events'),native_state_sha256=sha(native/'state.json'),original_receipt_sha256=sha(original/'receipt.json'),native_receipt_sha256=sha(native/'receipt.json'),limitations=['No recursive expression traversal, mutating helper or parser continuation was attempted.','Saved definition names are genuinely NUL-cleared by accepted PCH macro reconciliation.','Browser template zeros were observed in live process memory, not derived from PE raw offset zero.'])
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=compare(a.original,a.native);a.output.write_text(json.dumps(result,indent=2)+'\n');print('metadata natural context: matching complete bounded owners, pending buffers and files')
