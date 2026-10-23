#!/usr/bin/env python3
"""Join independently generated CRT identities into the mask index."""
import json
from pathlib import Path

project=Path(__file__).resolve().parents[1]
config=json.loads((project/'functions.json').read_text())
records=json.loads((project/'runtime.json').read_text())
for record in records:
    record['native_boundary']='external CRT startup contract; no application coverage'
    if record['name']=='_mainCRTStartup':
        record['support_closure']=['___setargv','__setdefaultprecision','__matherr','__setargv',
            'import:_except_handler3','import:_XcptFilter','import:_initterm','import:_controlfp']
config['runtime']=records
(project/'functions.json').write_text(json.dumps(config,indent=2)+'\n')
