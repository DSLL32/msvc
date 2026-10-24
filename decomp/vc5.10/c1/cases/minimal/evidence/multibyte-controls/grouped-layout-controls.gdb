set pagination off
python
import sys,copy,json,time
sys.path.insert(0,'/root/msvc2/decomp/vc5.10/c1/scripts')
import modeled_reader as m
m.configure('/root/.ai-tmp/msvc2/vc5.10/c1/grouped-layout-analysis','/root/msvc2/decomp/vc5.10/c1/cases/minimal/field-map.json')
original=copy.deepcopy(m._field_map); rejected=[];started=time.perf_counter()
for label in ('offset','extent','field-name'):
    m._field_map=copy.deepcopy(original)
    row=m._field_map['modeled_initialized'][-1]
    if label=='offset':row['native_offset']+=1
    elif label=='extent':row['bytes']-=1
    else:row['native_field']='c1_data.scanner.missing'
    try:m.schema()
    except ValueError:rejected.append(label)
    else:raise AssertionError('accepted bad schema '+label)
m._field_map=original
schema=m.schema()
for path in ('c1_bss.cleanup_callbacks[32]','c1_data.scanner.classes[256]','c1_data.missing'):
    try:m.locate(path,schema)
    except ValueError:rejected.append(path)
    else:raise AssertionError('accepted bad field path '+path)
result={'status':'PASS','kind':'static native ELF/DWARF schema guards; target never executed','rejected':rejected,'seconds':time.perf_counter()-started}
open('/root/.ai-tmp/msvc2/vc5.10/c1/grouped-layout-analysis/controls.json','w').write(json.dumps(result,indent=2)+'\n')
print(result)
end
quit
