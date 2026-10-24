"""Defined image-helper formals, ownership and state; offline comparison only."""
import json,re,struct
NAMES={0x4ec70:'checksum_image_file',0x4f4f0:'make_import_section_read_only',0x4f880:'map_bind_image',0x5b1b2:'recon_UnmapViewOfFile',0x5b19a:'recon_CloseHandle',0x516b0:'recon_filelength',0x5bbb0:'recon_fileno'}
REGISTERS=['ebx','esi','edi','ebp']
def compare(left,right,complete=False):
    left=[r for r in left if r.get('target') in NAMES and r['event'] in ['entry','return','frontier']];right=right['events'];assert left and len(left)==len(right),('inventory',len(left),len(right))
    active=[{},{}];owners=[None,None]
    for a,b in zip(left,right):
        name=NAMES[a['target']];assert (a['event'],a['phase'],name)==(b['event'],b['phase'],b['name'])
        av=a['arguments'];bv=b['formal_words'];n=4 if name=='map_bind_image' else (1 if name in ['make_import_section_read_only','recon_UnmapViewOfFile','recon_CloseHandle','recon_filelength','recon_fileno'] else 3);assert len(av)==len(bv)==n
        for e,args in [(a,av),(b,bv)]:
            assert len(e['workspace'])==9
            if name=='checksum_image_file':assert args==[e['names_address']+(32 if e['phase']==9 else 0),e['output_address']+4,0 if e['phase'] in [6,8] else e['output_address']+12]
            elif name=='map_bind_image':assert args==[e['names_address'],e['workspace_address'],0,0]
            elif name=='make_import_section_read_only':assert args==[e['workspace_address']]
            elif name=='recon_UnmapViewOfFile':assert args==[e['workspace'][4]]
            elif name=='recon_CloseHandle':assert args==[e['workspace'][3]]
            elif name=='recon_fileno':assert args[0] and e['stream_descriptor']==3
            else:assert args==[3]
        assert a['names']==b['names'] and a['outputs']==b['outputs'] and a['errors']==b['errors']
        assert a['workspace_guards'][:8]==b['workspace_guards'][:8]=='a5'*4
        assert a['workspace_guards'][-32:]==b['workspace_guards'][-32:]=='a5'*16
        for k in ['fp_control','fp_status','fp_tag']:assert a[k]&0xffff==b[k]&0xffff,(k,a[k],b[k])
        assert a['fp_control']&0xffff==0x27f and a['fp_status']&0xffff==0 and a['fp_tag']&0xffff==0xffff
        assert bytes.fromhex(a['fp_raw'])[28:108]==bytes(80)
        assert len(re.findall(r'Empty\s+0x00000000000000000000',b['fp_raw']))==8
        assert a['mapping_live']==b['mapping_live']
        for i in [0,1,2,7]:assert a['workspace'][i]==b['workspace'][i]
        for side,e in enumerate([a,b]):
            if e['phase']==1 and e['event']=='return':owners[side]=e['workspace'][:]
            if owners[side] is not None:assert e['workspace']==owners[side],('ownedworkspace mutation',e['phase'])
        if a['mapping_live']:
            for e in [a,b]:
                w=e['workspace'];assert w[4] and w[5]==w[4]+128 and w[6]==w[8] and w[8]==w[5]+248
                assert w[7]==4 and w[3] not in [0,0xffffffff]
            assert a['headers']==b['headers'] and a['sections']==b['sections']
        if a['event']=='entry':active[0][name]=a;active[1][name]=b
        elif a['event']=='return':
            if name=='recon_fileno':assert a['eip'] in [0x4ecaa,0x4ed25] and b['actual_caller']=='checksum_image_file'
            if name=='recon_filelength':assert a['eip']==0x4ed2e and b['actual_caller']=='checksum_image_file'
            if name!='make_import_section_read_only':assert a['eax']==b['eax']
            for e,table in zip([a,b],active):
                before=table.pop(name);popped=8 if name in ['recon_UnmapViewOfFile','recon_CloseHandle'] else 4
                assert e['esp']==before['esp']+popped and all(e[r]==before[r] for r in REGISTERS)
    if complete:
        assert not active[0] and not active[1] and right[-1]['phase']==9
        assert right[-1]['errors']==[2,2,2] and right[-1]['eax']==1
        assert len([e for e in right if e['event']=='entry' and e['name']=='checksum_image_file'])==6
        assert len([e for e in right if e['event']=='entry' and e['name']=='recon_fileno'])==10
        assert len([e for e in right if e['event']=='entry' and e['name']=='recon_filelength'])==5
    return dict(status='pass',matched_observations=len(left),complete=complete,scope='Actual formals/ownededges/guards/liveheaders/errors/CW/SW/tags/emptyregisters strict; voidhelperEAX unconsumed, no cross-imageaddress claim')
