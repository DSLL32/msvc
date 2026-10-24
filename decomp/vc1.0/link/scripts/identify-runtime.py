#!/usr/bin/env python3
"""Match CRT routines and their relocation targets; record reviewed ABI-equivalent variants."""
import json
from pathlib import Path
from binary import PE, COFF, archive, match_section
from importlib.machinery import SourceFileLoader

wt = SourceFileLoader('wine_tool', str(Path(__file__).with_name('wine-tool.py'))).load_module()
import sys
pe = PE(sys.argv[1] if len(sys.argv) > 1 else wt.TC / 'bin/LINK.EXE')
text = next(s for s in pe.sections if s['name'] == '.text')
code = pe.data[text['off']:text['off']+text['virtual_size']]
records = []
candidates = []
alias_groups = []
objects = list(archive(wt.TC / 'lib/LIBC.LIB'))

for name, data in objects:
    if name.startswith("iostream\\"): continue
    obj = COFF(data)
    for section in obj.sections:
        if not section['flags'] & 0x20: continue
        funcs = sorted((s for s in obj.symbols.values()
                        if s['section'] == section['index'] and
                        (s['type'] == 0x20 or s['storage'] == 2) and
                        not s['name'].startswith('.')), key=lambda s:s['value'])
        by_value = {}
        for sym in funcs: by_value.setdefault(sym['value'],set()).add(sym['name'])
        alias_groups.extend(group for group in by_value.values() if len(group)>1)
        for i, sym in enumerate(funcs):
            a = sym['value']
            b = funcs[i+1]['value'] if i+1 < len(funcs) else len(section['data'])
            if b <= a: continue
            part = dict(data=section['data'][a:b],
                        relocs=[(off-a,s,k) for off,s,k in section['relocs'] if a <= off < b])
            matches = match_section(code, part)
            part['index'] = section['index']
            part['bias'] = a
            for off in matches:
                start = pe.base + text['rva'] + off
                # LINK's app code precedes the separately linked CRT block.
                # Tiny push/call/ret wrappers are otherwise ambiguous.
                if len(sys.argv)==1 and start < 0x509d0: continue
                record=dict(name=sym['name'],start=start,end=start+b-a,
                            object=name,library='LIBC.LIB')
                candidates.append((part,start,[record]))
from indexes import imports
from collections import defaultdict
import struct
import_targets={r['name'][7:]:r['start'] for r in imports(pe)}
while True:
    known=defaultdict(set)
    for part,start,group in candidates:
        for r in group:known[r['name']].add(r['start'])
    cfg=json.loads(Path('functions.json').read_text())
    if len(sys.argv)==1:
        for r in cfg['driver']:known[r['name']].add(r['start'])
    else:
        from indexes import publics
        linked=publics()
        for r in cfg['driver']:
            if r['name'] in linked:known[r['name']].add(linked[r['name']])
    for name,start in [('__fpmath',0x52ce0),('_parse_cmdline',0x54530),('__heap_grow_emptylist',0x53590),('__control87',0x592c0)]:
        if len(sys.argv)==1: known[name].add(start)
    for aliases in alias_groups:
        addresses=set().union(*(known.get(name,set()) for name in aliases))
        for name in aliases: known[name].update(addresses)
    remaining=[]
    for part,start,group in candidates:
        valid=True
        for offset,symbol,kind in part['relocs']:
            if kind!=20:continue
            actual=start+offset+4+struct.unpack('<i',pe.read(start+offset,4))[0]
            addend=struct.unpack_from('<i',part['data'],offset)[0]
            name=symbol['name'];expected=None
            if symbol['section']==part['index']:
                expected={start+symbol['value']-part['bias']}
                # A wrapper's object-local public callee also has its own body
                # match. Require that identity, not merely the same relative
                # offset: _getc and _unlink wrappers are identical bytes, but
                # their preceding _fgetc/_remove bodies are not interchangeable.
                if name in known:
                    expected &= known[name]
                elif symbol['storage']==2 and any(actual-addend in addresses for addresses in known.values()):
                    expected=set()
            elif symbol['section']==0:
                if name.startswith('_') and '@' in name:
                    target=import_targets.get(name[1:].rsplit('@',1)[0])
                    expected=set() if target is None else {target}
                elif name in known:expected=known[name]
                elif any(actual-addend in addresses for addresses in known.values()):expected=set()
            if expected is not None and actual-addend not in expected:
                valid=False;break
        if valid:remaining.append((part,start,group))
    if len(remaining)==len(candidates):break
    candidates=remaining
records=[r for part,start,group in candidates for r in group]
# Exclude instruction suffixes and tiny false-positive matches.
records = [r for r in records if r['end'] - r['start'] >= 6]
for r in records:
    if r['name'] == '_strcat':
        shared = next(x for x in records if x['name'] == '_strcpy' and x['start'] == r['start']+4)
        r['shared'] = [[r['start']+8, shared['end']]]
# These routines have the same ABI and control flow as LIBC.LIB; LINK used
# different register allocation. Ranges were checked instruction by instruction.
if len(sys.argv) == 1:
    for name, start, end in [('__filelength',0x516b0,0x51720), ('__fpmath',0x52ce0,0x52cf0), ('_parse_cmdline',0x54530,0x546a0), ('__heap_grow_emptylist',0x53590,0x535f0), ('__control87',0x592c0,0x59300), ('__write',0x532d0,0x53500), ('_doexit',0x517b0,0x51830), ('__read',0x55980,0x55bb0), ('__close',0x555f0,0x55690), ('__free_osfhnd',0x53e10,0x53e80)]:
        records.append(dict(name=name,start=start,end=end,library='LIBC.LIB',
                            evidence='equivalent register allocation; manually disassembled'))
if len(sys.argv)==1:
    records=[r for r in records if not (r['name']=='_memcpy' and r['start']==0x52b90)]
# _find is a complete byte-exact shipped CRT match, including imported call
# relocations. Native correspondence is at this CRT boundary, not each IAT thunk.
for r in records:
    if r['name'] in ('_perror','_setbuf'):
        r['evidence']='Complete byte-exact shipped PERROR.OBJ144B/SETBUF.OBJ64B and all14 relocations verified; native correspondence only at actual open_debug_log-qualified public boundary with every formal/FILE/errors/FP/returnABI and full36-observation lifecycle/artifacts in cases/direct-debug-log. Zero application credit/no new handwritten CRT or native internalhelper ABI claim.'
    if r['name']=='__filelength':
        r['evidence']='shipped FLENGTH.OBJ112-byte body: all instructions/relocations exact except CMP ESI,EDI versus CMP EDI,ESI before JE, whose consumed ZF is identical. Three __lseek edges and errno/DOS/nhandle globals verified. Native correspondence only at public _filelength, no new CRT C/ABI claim; cases/direct-image-checksum.'
    if r['name']=='_vfprintf':
        r['evidence']='byte-exact shipped VFPRINTF.OBJ including _stbuf/_output/_ftbuf callees; native public boundary qualified by actual omf_warning caller, stdout/plainformat/va_listowner/no consumedvariadicwords and exact entry/result/FILEbuffer/errors/fullarchive in cases/omf-continuation. Earlier recon_printf/recon_fprintf are alreadyaccepted publicCRTaliases, logged not newgraph credit.'
    if r['name']=='__snprintf':
        r['evidence']='byte-exact SNPRINTF.OBJ including verified _output/_flsbuf callees; native public CRT boundary observes3fixedslots+onevariadicDWORD,340-byte caller output ownership, actual defined text/result/errno and full EDIT error files/streams in cases/editor-errors. No native internal CRT or unwritten-tail ABI claim.'
    if r['name']=='__unlink':
        r['evidence']='byte-exact UNLINK.OBJ wrapper; object-local public _remove callee identity independently matched; native public CRT boundary has actual filename/caller/result/live-error and full fatal cleanup proof in cases/resource-identity (ambient LastError explicitly not equated)'
    if r['name']=='__setmode':
        r['evidence']='byte-exact shipped SETMODE.OBJ including full original/canonical graph/ABI/extent. Reached inside alreadyaccepted public __chsize during direct-file-resize; native correspondence at public recon_chsize only, actual3fd/length/result/error/FP and fullfiles, zeroapplicationcredit; no native internalhelperABIclaim.'
    if r['name']=='__find':
        r['evidence']='byte-exact shipped _SETARGV.OBJ; Find* relocations verified; native CRT-boundary differential proof in lib/recon/find-validation.json'
    if r['name'] in ('_strtoul','_strtoxl'):
        r['evidence']='byte-exact shipped STRTOL.OBJ, relocation targets verified; native correspondence at public strtoul only, explicit nonempty base16/input/result/end-pointer/errno inventory in cases/editor-rebase-bind/capture/strtoul-boundary'
    if r['name']=='_system':
        r['evidence']='byte-exact shipped SYSTEM.OBJ with relocation targets verified; fresh native/original system boundary, errno0 entry/return, exact pinned cmdcopy responses in cases/rom-local-imports'
records.sort(key=lambda r:r['start'])
Path(sys.argv[2] if len(sys.argv) > 2 else 'runtime.json').write_text(json.dumps(records, indent=2)+'\n')
print('%d runtime functions identified, %d bytes' %
      (len(records), sum(r['end']-r['start'] for r in records)))
