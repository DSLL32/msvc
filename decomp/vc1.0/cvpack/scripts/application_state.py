"""Complete existing typed application-global comparison, shared by acceptors."""
import json
from state import BINARY
from objects import pointer_identity

def compare_globals(config,pe,memory,state,original_objects,native_objects,definitions,events,symbol_proof):
    matched={}
    for item in json.loads((BINARY/'data.json').read_text()):
        raw=bytes(memory[item['original']+i]for i in range(item['size']))
        actual=state['globals'][item['name']]
        if item.get('kind')=='pointer-structure':
            expected=bytearray(raw);observed=bytearray.fromhex(actual)
            assert len(expected)==len(observed)==item['size']
            for offset in item['pointer_fields']:
                assert pointer_identity(original_objects,int.from_bytes(expected[offset:offset+4],'little'))==pointer_identity(native_objects,int.from_bytes(observed[offset:offset+4],'little')),'global structure pointer owner differs'
                expected[offset:offset+4]=observed[offset:offset+4]=b'\0'*4
            assert expected==observed,'global structure defined bytes differ'
            expected=actual
        elif item.get('kind')=='callback-table':
            from callback_table import describe
            expected=describe(item,raw,config['functions'])
        elif item.get('kind')=='string-pointer-array':
            expected=[]
            for index in range(item['size']//4):
                pointer=int.from_bytes(raw[index*4:index*4+4],'little')
                value=[]
                while memory[pointer]:value.append(memory[pointer]);pointer+=1
                expected.append(bytes(value).hex())
        elif item.get('kind')=='pointer-array':
            assert item['size']%4==0 and len(actual)==item['size']//4,'global pointer-array extent differs'
            expected=[]
            for index,entry in enumerate(actual):
                value=int.from_bytes(raw[index*4:index*4+4],'little')
                assert entry['null']==(value==0)and (entry['address']==0)==(value==0),'global pointer-array null state differs'
                identity=dict(null=not value)
                if value:
                    old_id=pointer_identity(original_objects,value);new_id=pointer_identity(native_objects,entry['address'])
                    assert old_id==new_id,('global pointer-array owner differs',item['name'],index,old_id,new_id)
                    identity['object']=old_id
                expected.append(identity)
            actual=expected
        elif isinstance(actual,dict):
            value=int.from_bytes(raw,'little')
            assert actual['null']==(value==0) and (actual['address']==0)==(value==0)
            expected={'null':value==0}
            if value:
                targets=[r for r in config['functions']if r['original']==value]
                owners=[dict(name=r['name'],offset=value-r['original'])for r in json.loads((BINARY/'data.json').read_text())if r['original']<=value<r['original']+r['size']]
                if targets:
                    assert len(targets)==1 and actual['function']==targets[0]['native']
                    expected['function']=targets[0]['native']
                elif item['name']=='symbol_input'and state['frames'][0]in('link_symbol_scopes','record_symbol_offsets','create_symbol_reference'):
                    from completed_symbol_input import compare as compare_completed_cursor
                    expected['object']=compare_completed_cursor(config,pe,item['name'],value,actual['address'],original_objects,native_objects,state,definitions,events,symbol_proof)
                elif owners:
                    assert len(owners)==1
                    owner=owners[0]
                    assert actual['address']==state['global_addresses'][owner['name']]+owner['offset']
                    expected['global']=owner
                else:
                    old_id=pointer_identity(original_objects,value)
                    new_id=pointer_identity(native_objects,actual['address'])
                    assert old_id==new_id,('global object pointer differs',item['name'],old_id,new_id)
                    expected['object']=old_id
            actual=expected
        else:expected=raw.hex()
        assert actual==expected,('application state differs',item['name'],expected,actual)
        matched[item['name']]=actual
    return matched
