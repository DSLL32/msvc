"""Typed register factory's actual dispatch/class data and field ownership."""
import json,re,struct
from state import BINARY,TARGET,sha

def verify(config):
    name='@new_function_register@4'
    if name not in config['accepted']+config.get('trial',[]):return None
    row=next(r for r in config['driver']if r['name']==name)
    raw=(BINARY/row['register_node_model']).read_bytes();assert sha(raw)==row['register_node_model_sha256'];model=json.loads(raw);assert model['target_sha256']==sha(TARGET.read_bytes())
    for file,digest in model['source_files'].items():assert sha((BINARY/file).read_bytes())==digest,'register factory source/field owner changed'
    from data_evidence import reader
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    evidence=[];families=[]
    for side,path in [('original',TARGET),('rebuilt',BINARY/'build/c2.exe')]:
        r=reader(path);s=model['dispatch'][side];base=0 if side=='original'else maps[name]
        table,classes=base+s['table'],base+s['classes'];targets=struct.unpack('<%dI'%len(s['targets']),r(table,4*len(s['targets'])));expected=[base+x for x in s['targets']]
        assert list(targets)==expected,'register factory actual code target identity differs'
        mapping=r(classes,13);assert list(mapping)==s['class_bytes'],'register factory class data changed'
        assert all(x<len(targets)for x in mapping)
        kinds=['default']*256
        for kind in range(3,16):kinds[kind]=s['families'][mapping[kind-3]]
        families.append(kinds);evidence.append({'side':side,'table':table,'classes':classes,'table_sha256':sha(r(table,len(targets)*4)),'classes_sha256':sha(mapping),'kind_families':kinds})
    assert families[0]==families[1],'original/rebuilt kind dispatch meanings differ'
    return {'model_sha256':sha(raw),'dispatch':evidence,'new_field':{'offset':40,'bytes':4,'meaning':'free-register next link; read only when actual corresponding free-list is nonNULL'},'next':'Actual setECX/numberEDX formals at named maskedINT3 add_register_to_set; body not accepted.'}
