"""Concrete scope-link stores into the already-defined608-byte symbol prefix."""
import hashlib,json
from state import BINARY

def validate(p):
    r=p['scope_link_writes'];assert r['source']=='cases/hello-debug-dll/evidence/0094-end_symbol_block/native-symbol-output.json','scope-link preimage owner differs'
    path=BINARY/r['source'];assert hashlib.sha256(path.read_bytes()).hexdigest()==r['sha256'],'scope-link frozen preimage changed'
    rows=json.loads(path.read_text());assert len(rows)==1 and rows[0]['frontier']=='link_symbol_scopes'
    before=bytes.fromhex(rows[0]['postbytes'])[:608];assert len(before)==608 and hashlib.sha256(before).hexdigest()==r['prefix_sha256'],'scope-link preimage extent/content differs'
    assert r['writes']==[[8,0,108],[116,0,292],[120,0,296],[304,0,492],[308,0,496],[504,0,604]],'unreviewed scope-link field/target'
    assert r['procedures']==[108,296,496]and r['ends']==[292,492,604],'scope-link topology differs'
    return r,before

def compare(p,after):
    r,before=validate(p);expected=bytearray(before)
    for at,prior,value in r['writes']:
        assert int.from_bytes(expected[at:at+4],'little')==prior,'scope-link initial field differs'
        expected[at:at+4]=value.to_bytes(4,'little')
    for proc,end in zip(r['procedures'],r['ends']):
        assert int.from_bytes(before[proc+2:proc+4],'little')==0x205 and int.from_bytes(before[end+2:end+4],'little')==6,'scope-link target record kind differs'
        assert before[proc+4:proc+8]==b'\0'*4,'scope-link procedure parent was not top-level'
    assert len(after)>=608 and after[:608]==expected,'scope-link defined field/prefix writes differ'
    return dict(procedures=r['procedures'],ends=r['ends'],writes=r['writes'],defined=608)
