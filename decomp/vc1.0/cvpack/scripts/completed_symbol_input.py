"""Identity-only completed-loop cursor; never widens generic pointer resolution."""
import hashlib,json,re
from state import BINARY

def validate(config,original,rebuilt=None,linked=None):
    ref=config['symbol_output_observations'];path=BINARY/ref['evidence']
    assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale completed cursor proof'
    p=json.loads(path.read_text())['completed_input']
    assert {k:v for k,v in p.items()if k not in('original_loop','scope','successors','successor_call')}==dict(global_name='symbol_input',owner='type_data',extent=4746,offset=4746,frontier=0x1bd60,caller_return=0x25238,identity_only=True),'unreviewed one-past cursor identity'
    assert p['successors']==[dict(original=0x1d840,native='record_symbol_offsets',caller_return=0x25277)],'unreviewed one-past successor'
    r=p['successor_call'];assert r['start']==0x25262 and r['end']==0x2527a and original.read(r['start'],r['end']-r['start']).hex()==r['bytes'],'completed cursor successor call changed'
    r=p['original_loop'];assert r['start']==0x2520a and r['end']==0x25238 and original.read(r['start'],r['end']-r['start']).hex()==r['bytes'],'completed cursor original loop changed'
    assert original.read(0x2520a,0x19)==bytes.fromhex('8bc525ffff00000105e4350300391de43503000f82d4feffff'),'completed cursor original add/test differs'
    if rebuilt is not None:
        from capstone import Cs,CS_ARCH_X86,CS_MODE_32
        code=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(rebuilt.read(linked['_pack_new_types'],0x500),linked['_pack_new_types']))
        call=next(i for i,r in enumerate(code)if r.mnemonic=='call'and r.op_str=='0x%x'%linked['_link_symbol_scopes'])
        successor=next(i for i,r in enumerate(code)if r.mnemonic=='call'and r.op_str=='0x%x'%linked['_record_symbol_offsets'])
        assert code[successor-5].bytes==bytes.fromhex('8b4510')and code[successor-4].bytes==bytes.fromhex('668b00')and code[successor-3].bytes==bytes.fromhex('50'),'completed successor module argument differs'
        assert code[successor-2].bytes==code[call-2].bytes and code[successor-1].bytes==code[call-1].bytes,'completed successor output/size arguments differ'
        cursor=linked['_symbol_input'].to_bytes(4,'little');output=linked['_symbol_output'].to_bytes(4,'little')
        assert code[call-5].bytes==b'\xa1'+output and code[call-4].bytes[:2]==b'\x2b\x45','completed cursor output subtraction differs'
        size_slot=code[call-3].bytes;assert size_slot[:2]==b'\x89\x45'and code[call-2].bytes==b'\xff\x75'+size_slot[2:],'completed cursor size argument def-use differs'
        assert code[call-1].bytes==b'\xff\x75'+code[call-4].bytes[2:],'completed cursor start argument differs'
        add=code[call-7];assert add.bytes==b'\x01\x05'+cursor and code[call-8].bytes[:3]==b'\x0f\xb7\x45','completed cursor length add differs'
        jump=code[call-6];assert jump.mnemonic=='jmp';target=int(jump.op_str,16);test=next(i for i,r in enumerate(code)if r.address==target)
        assert code[test].bytes[:2]==b'\x8b\x45'and code[test+1].bytes==b'\x39\x05'+cursor and code[test+2].mnemonic=='jae'and int(code[test+2].op_str,16)==code[call-5].address,'completed cursor unsigned end test differs'
    return p

def compare(config,pe,name,old_pointer,new_pointer,old,new,state,definitions,events,symbol_proof):
    p=validate(config,pe)
    profiles=[dict(native='link_symbol_scopes',original=p['frontier']),*p['successors']]
    profile=next((r for r in profiles if r['native']==state['frames'][0]),None)
    assert name==p['global_name']and profile,'one-past cursor outside exact global/frontier'
    assert re.search(r'^exception 80000003 va %08x '%profile['original'],events,re.M),'one-past original frontier differs'
    definition=definitions[p['owner']]
    assert definition['size']==p['extent']and definition['defined_ranges']==[[0,4746]],'one-past allocation lacks complete requested/read extent'
    assert definition['write_evidence']==[dict(site=0x17dc9,offset=0,length=4746)],'one-past allocation defining read differs'
    assert symbol_proof and symbol_proof['defined']==608 and symbol_proof['native']['completed_input']['end']==new_pointer,'one-past native loop-exit proof missing'
    fields=re.search(r'^registers (.*)$',events,re.M)[1].split();registers={k:int(v,16)for k,v in zip(fields[::2],fields[1::2])}
    assert registers['ebx']==old_pointer,'one-past original end-register differs'
    for pointer,walks,row in((old_pointer,old,symbol_proof['original']),(new_pointer,new,symbol_proof['native'])):
        owners=walks[p['owner']];assert len(owners)==1,'one-past cursor owner missing/ambiguous'
        owner=owners[0];assert len(bytes.fromhex(owner['bytes']))==p['extent']and owner['address']==row['input'],'one-past cursor actual owner/extent differs'
        assert pointer==owner['address']+p['offset'],'one-past cursor actual offset differs'
    assert state['globals']['type_capacity']==(4746).to_bytes(4,'little').hex(),'one-past requested capacity differs'
    return dict(name=p['owner'],offset=p['offset'],identity_only=True,original_address=old_pointer,native_address=new_pointer)
