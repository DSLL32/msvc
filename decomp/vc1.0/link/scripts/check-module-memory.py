#!/usr/bin/env python3
"""Host-only provenance guards; no target execution or original capture."""
import copy,importlib.util,json
from pathlib import Path
spec=importlib.util.spec_from_file_location('module_memory_proof',Path(__file__).with_name('module-memory-proof.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
p=json.loads((m.BINARY/'cases/editor-rebase-bind/capture/module-memory-proof.json').read_text())
result=m.validate(p);negatives=[]
for name,mutate in [
 ('VA_as_RVA',lambda p:p.update(original_rva=p['original_va'])),
 ('wrong_base',lambda p:p.update(image_base=0)),
 ('consistently_shifted_coordinates',lambda p:p.update(original_va=p['original_va']+4,original_rva=p['original_rva']+4)),
 ('wrong_extent',lambda p:p.update(bytes=32)),
 ('wrong_owner',lambda p:p['global_definition'].update(owner='src/editor-bind.c')),
 ('wrong_original',lambda p:p.update(original_sha256='0'*64)),
 ('wrong_host',lambda p:p['host_source'].update(sha256='0'*64))]:
 q=copy.deepcopy(p);mutate(q)
 try:m.validate(q)
 except AssertionError:negatives.append(name)
 else:raise AssertionError('Accepted invalid proof: '+name)
print(json.dumps(dict(positive=result,rejected=negatives,seconds='unmeasured; host-only static guards'),indent=2))
