"""Actual constructor owners/defined fields; offline packet comparison only."""
import re
from case_paths import PROGRAM
NAMES={0x20a10:'insert_other_input',0x26b40:'main'}
def compare(left,right,complete=False):
    left=[e for e in left if e.get('target')in NAMES and e['event']in['entry','return','frontier']];right=right['events'];assert left and len(left)==len(right),('inventory',len(left),len(right));active=[{},{}]
    for a,b in zip(left,right):
        name=NAMES[a['target']];assert(a['event'],a['phase'],name)==(b['event'],b['phase'],b['name'])
        phase=a['phase'];count=min(phase,4)+(name=='insert_other_input'and a['event']=='return')
        normalized=[]
        for side,e in enumerate([a,b]):
            args=e['arguments'];assert len(args)==(3 if phase<4 else 2)
            if phase<4:assert args[:2]==[e['names_address']+32*phase]*2 and args[2]&255==phase&1
            else:
                assert args[0]==4 and args[1] and len(e['argv'])==5
                texts=[bytes.fromhex(s).rstrip(b'\0').decode()if s else ''for s in e['argv_names']]if side==0 else e['argv_names']
                assert texts==([PROGRAM,'-dump','/nologo','/headers','']if e['event']=='entry'else['-dump','/nologo','/headers','',''])
                assert all(bool(p)==bool(s)for p,s in zip(e['argv'],texts))
            assert len(e['nodes'])==count
            addresses=[node['address']for node in e['nodes']];assert len(set(addresses))==count and all(addresses)
            def index(p):
                if not p:return -1
                assert p in addresses,('unownednode',p);return addresses.index(p)
            graph=[]
            for i,node in enumerate(e['nodes']):
                raw=node['raw'];assert len(raw)==4 and raw[:2]==[e['names_address']+32*i]*2
                graph.append([i*32,i*32,index(raw[3])])
                # raw[2] is retained, explicitly unwritten and unconsumed; no equality claim.
            assert len(e['list'])==3 and len(e['cursors'])==2
            assert e['list'][2]==count
            normalized.append([index(e['list'][0]),index(e['list'][1]),count,*map(index,e['cursors']),graph])
            if count==0:assert e['list']==[0,0,0] and e['cursors']==[0,0]
            if e['event']=='return'and phase<4:assert e['eax']==addresses[-1]
            if e['event']=='return'and phase==4:assert e['eax']==0
            assert e['errors']==[0,0,0]
            assert e['fp_control']&0xffff==0x27f and e['fp_status']&0xffff==0 and e['fp_tag']&0xffff==0xffff
            if side==0:assert bytes.fromhex(e['fp_raw'])[28:108]==bytes(80)
            else:assert len(re.findall(r'Empty\s+0x00000000000000000000',e['fp_raw']))==8
            if e['event']=='entry':active[side][phase]=e
            elif e['event']=='return':
                before=active[side].pop(phase);assert e['esp']==before['esp']+4 and all(e[r]==before[r]for r in ['ebx','esi','edi','ebp'])
        assert normalized[0]==normalized[1],('definedgraph',normalized)
        assert a['names']==b['names'] and bytes.fromhex(a['names'])[-16:]==b'\xa5'*16
    if complete:
        assert len(left)==10 and not active[0]and not active[1] and [(e['phase'],e['event'])for e in right]==[(i,k)for i in range(5)for k in ['entry','return']]
        assert normalized[0]==[1,2,4,0,3,[[0,0,2],[32,32,3],[64,64,-1],[96,96,0]]]
    return dict(status='pass',matched_observations=len(left),complete=complete,scope='Initialactualformals/ownednode0,4,12/graph/count/cursors/namesguards/argv/returnESP+nonvolatiles/errors/CW+SW+tags+emptyregisters; rawunwrittennode8 and writablepostcallparameters retained, not consumedoutputs/equalityclaims.')
