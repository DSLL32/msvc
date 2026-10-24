"""Defined parser/global/owned mapping state, offline actual-packet comparison."""
import re
NAMES={0x4faf0:'parse_bind_arguments',0x4f880:'map_bind_image',0x5b1b2:'recon_UnmapViewOfFile',0x5b19a:'recon_CloseHandle'}
def compare(left,right,complete=False):
    left=[e for e in left if e.get('target')in NAMES and e['event']in['entry','return','frontier']];right=right['events'];assert left and len(left)==len(right),('inventory',len(left),len(right));active=[{},{}];mapped=[None,None]
    for a,b in zip(left,right):
        name=NAMES[a['target']];assert(a['event'],a['phase'],name)==(b['event'],b['phase'],b['name']);phase=a['phase'];logical=[]
        for side,e in enumerate([a,b]):
            args=e['arguments'];state=e['workspace'];g=e['globals'];assert len(g)==5 and len(state)==9
            if name=='parse_bind_arguments':
                vector,argc={0:(0,1),1:(1,5),5:(2,3),6:(3,3)}[phase];assert args==[argc,e['vectors_address']+24*vector]
            elif name=='map_bind_image':assert args==[g[0],e['workspace_address'],0,g[2]]and g[4]==0
            elif name=='recon_UnmapViewOfFile':assert args==[state[4]]
            else:assert args==[state[3]]
            def offset(p):
                if not p:return -1
                o=p-e['names_address'];assert 0<=o<1024,('foreignname',o);return o
            vectors=[offset(p)for p in e['vectors']];assert vectors==[0,-1,-1,-1,-1,-1,256,288,320,352,384,-1,512,544,576,-1,-1,-1,768,800,832,-1,-1,-1]
            logical.append([offset(g[0]),g[1],g[2],offset(g[3]),g[4]])
            if phase==0:assert g==[0,0,0,0,0]
            assert e['errors']==[0,0,0]
            assert e['fp_control']&0xffff==0x27f and e['fp_status']&0xffff==0 and e['fp_tag']&0xffff==0xffff
            if side==0:assert bytes.fromhex(e['fp_raw'])[28:108]==bytes(80)
            else:assert len(re.findall(r'Empty\s+0x00000000000000000000',e['fp_raw']))==8
            assert e['workspace_guards'][:8]=='a5'*4 and e['workspace_guards'][-32:]=='a5'*16
            if phase==2 and e['event']=='return':mapped[side]=state[3:]
            if mapped[side]is not None:assert state[3:]==mapped[side]
            if e['mapping_live']:
                assert state[3]not in [0,0xffffffff]and state[4]and state[5]==state[4]+128 and state[6]==state[8]==state[5]+248 and state[7]==4
            if e['event']=='entry':active[side][phase]=e
            elif e['event']=='return':
                before=active[side].pop(phase);assert e['esp']==before['esp']+(8 if phase in[3,4]else 4)and all(e[r]==before[r]for r in ['ebx','esi','edi','ebp'])
        assert logical[0]==logical[1]
        assert a['names']==b['names']and bytes.fromhex(a['names'])[-16:]==b'\xa5'*16
        assert a['mapping_live']==b['mapping_live']==int(phase==2 and a['event']=='return'or phase==3 and a['event']!='return')
        if a['mapping_live']:assert a['headers']==b['headers']and a['sections']==b['sections']
        if a['event']=='return':assert a['eax']==b['eax']
    if complete:
        assert len(left)==14 and not active[0]and not active[1]and[(e['phase'],e['event'])for e in right]==[(i,k)for i in range(7)for k in ['entry','return']]
        assert [e['eax']for e in right if e['event']=='return']==[0,1,1,1,1,1,0]and logical[0]==[832,1,0,352,1]
    return dict(status='pass',matched_observations=len(left),complete=complete,scope='Actualtypedformals/allownedargv+strings/definedglobals/initializedBoundImage12..32+realheaderssections/guards/ESP+nonvolatiles/returns/errors/CW+SW+tags+emptyregs strict. RawBoundImage0,4,8 and postcallbyvalueparameter slots retained, no invented outputequality.')
