"""Refresh the concise current README status after an accepted checkpoint."""
import json
from pathlib import Path
import re

BINARY=Path(__file__).resolve().parents[1]


def refresh():
    config=json.loads((BINARY/'functions.json').read_text())
    status=json.loads((BINARY/'validation.json').read_text())
    cache=json.loads((BINARY/'cases/native-traces.json').read_text())
    coverage=status['coverage']
    path=BINARY/'README.txt';text=path.read_text()
    text=re.sub(r'PARTIAL binary reconstruction: \d+ accepted frontiers, including \d+ frontend',
        'PARTIAL binary reconstruction: %d accepted frontiers, including %d frontend'%(len(config['accepted']),len(cache['accepted_application'])),text,count=1)
    name=status['active_case'];terminal=config['cases'][name]['terminal']
    complete=[key for key,value in config['cases'].items() if value['terminal']=='exit:0']
    description='\nComplete cases: '+', '.join(complete)+'.\n'
    if terminal!='exit:0':
        address=next(r['start'] for r in config['driver'] if r['name']==terminal)
        description+='Active case %s stops at %s, original VA 0x%x.\n'%(name,terminal,address)
    else:description+='Next: seed the next approved C++ queue case.\n'
    text=re.sub(r'(functions and seven older CRT functions reconstructed as source\.).*?(All unimplemented callees remain meaningful named INT3 stubs\.)',
        lambda match:match[1]+description+match[2],text,count=1,flags=re.S)
    text=re.sub(r'Accepted original extents cover \d+ / \d+ code-section bytes \([0-9.]+%\)\.',
        'Accepted original extents cover %d / %d code-section bytes (%.6f%%).'%(coverage['accepted_code_bytes'],coverage['total_executable_code_bytes'],coverage['accepted_code_percent']),text,count=1)
    entries=cache['cases']
    paragraphs=[]
    for key,entry in entries.items():
        terminal=entry['terminal'];suffix='exit 0' if terminal['kind']=='exit' else 'its named terminal trap'
        paragraphs.append('%s expects %d application first entries and %s.'%(key,len(entry['expected']),suffix))
    text=re.sub(r'cpp_basic expects \d+\napplication first entries.*?(?=\n\ncases/original-entry-references)',
        '\n'.join(paragraphs),text,count=1,flags=re.S)
    text=re.sub(r'ordered \d+-entry acceptance provenance','ordered %d-entry acceptance provenance'%len(config['accepted']),text,count=1)
    path.write_text(text)
