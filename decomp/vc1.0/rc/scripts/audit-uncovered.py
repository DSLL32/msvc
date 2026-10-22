#!/usr/bin/env python3
"""Inventory the native executable-byte complement, excluding false CRT aliases.

Ranges below are native disassembly extents, not one-byte pending stubs or
archive-candidate extents. This is analysis metadata, never executable recovery.
"""
from pathlib import Path
import json
from collections import defaultdict
import importlib.machinery
import reference_indexes as compare

GROUPS = {
 'verbose-public': [(0x8110,0x8166,'wide_to_ansi'),(0xdb80,0xdb86,'RtlUnicodeStringToAnsiString thunk')],
 'PATH-public': [(0xed20,0xed3a,'spawnvp'),(0xed50,0xeedb,'spawnvpe'),(0xeef0,0xef71,'getpath')],
 'exception-and-nonlocal-jump': [(0xdbc8,0xde37,'packed SEH2/unwind/setjmp/longjmp/raiser group'),
   (0xde50,0xde56,'RtlUnwind thunk'),(0x11bc0,0x11ced,'XcptFilter'),(0x11cf0,0x11d1b,'xcptlookup'),
   (0x11d30,0x11d36,'UnhandledExceptionFilter thunk'),(0xe200,0xe212,'quick exit')],
 'startup-fatal-and-unused-exit': [(0x11930,0x1193b,'floating-format trap'),(0x11ab0,0x11ad1,'amsg_exit'),
   (0x11b20,0x11b46,'FF_MSGBANNER'),(0x11b50,0x11ba8,'NMSG_WRITE'),
   (0xe220,0xe22f,'cexit'),(0xe230,0xe23f,'c_exit')],
 'stdio-lowio-conditional-or-unused': [(0xf8e0,0xf93f,'commit'),(0xf950,0xf956,'FlushFileBuffers thunk'),
   (0xfae0,0xfb74,'open_osfhandle'),(0xfb90,0xfb96,'SetStdHandle thunk'),(0xff10,0xff1e,'getc wrapper'),
   (0x10620,0x1063a,'open wrapper'),(0x10a30,0x10b79,'chsize'),(0x10b90,0x10b96,'SetEndOfFile thunk'),
   (0x10ba0,0x10c31,'setmode'),(0x12660,0x1266e,'unlink wrapper')],
 'locale-and-unused-ctype': [(0xdeb0,0xdf6f,'isctype'),(0xdf80,0xdf86,'GetStringTypeW thunk'),
   (0xdf90,0xdf96,'MultiByteToWideChar thunk'),(0x13340,0x13374,'isalpha mask103'),
   (0x13380,0x133b2,'isupper mask1'),(0x133c0,0x133f2,'islower mask2'),(0x13400,0x13432,'isdigit mask4'),
   (0x13440,0x13474,'isxdigit mask80'),(0x134c0,0x134f2,'ispunct mask10'),(0x13500,0x13534,'isalnum mask107'),
   (0x13540,0x13574,'isprint mask157'),(0x13580,0x135b4,'isgraph mask117'),(0x135c0,0x135f2,'iscntrl mask20'),
   (0x13600,0x13611,'isascii'),(0x13620,0x13628,'toascii'),(0x13630,0x13677,'identifier-character helper'),
   (0x13680,0x136c7,'identifier-character helper'),(0x136e0,0x137df,'mbtowc'),(0x13880,0x13888,'tolower wrapper'),
   (0x13a20,0x13a26,'LCMapStringW thunk'),(0x13a30,0x13a36,'WideCharToMultiByte thunk'),
   (0x13a40,0x13a48,'toupper wrapper'),(0x13de0,0x13e7e,'wctomb')],
 'unused-formatter-and-string-helpers': [(0xf3d0,0xf3fb,'whitespace helper'),(0xf530,0xf58d,'copy basename helper'),
   (0x117d0,0x117e0,'get_short_arg'),(0x13d30,0x13d66,'wcscat')]
}
# Only recognized alignment bytes outside the decoded function ranges count as
# padding. Zero-fill at the .text tail also remains included in the denominator.
PADDING = [bytes.fromhex(s) for s in ('8da42400000000','8d9b00000000','0500000000',
 '8d642400','8d4900','8d7600','8bff','90','00')]


def main():
    pe,_,cfg,index,_=compare.indexes();base=pe.base;accepted=set(cfg['accepted'])
    code=set();restored=set()
    for s in pe.sections:
        if s['flags']&0x20:code.update(range(base+s['rva'],base+s['rva']+s['virtual_size']))
    for r in index:
        if r['name'] in accepted:
            for a,b in [(r['start'],r['end'])]+r.get('shared',[]):restored.update(range(a,b))
    restored &= code;uncovered=code-restored;classified=set();groups={}
    for name,ranges in GROUPS.items():
        fields=[];count=0
        for a,b,label in ranges:
            region=set(range(base+a,base+b))&uncovered
            assert not region&classified,(name,label,'overlap')
            classified|=region;count+=len(region)
            fields.append(dict(start=base+a,end=base+b,label=label,uncovered_bytes=len(region)))
        groups[name]=dict(bytes=count,ranges=fields)
    remaining=sorted(uncovered-classified);padding=set();unknown=[];n=0
    while n<len(remaining):
        a=remaining[n];limit=a+1
        while n+limit-a<len(remaining) and remaining[n+limit-a]==limit:limit+=1
        cursor=a
        while cursor<limit:
            pattern=next((p for p in PADDING if cursor+len(p)<=limit and pe.read(cursor,len(p))==p),None)
            if pattern:padding.update(range(cursor,cursor+len(pattern)));cursor+=len(pattern)
            else:unknown.append(cursor);cursor+=1
        n+=limit-a
    audit=importlib.machinery.SourceFileLoader('gap_graph',str(Path(__file__).with_name('audit-graph.py'))).load_module()
    xrefs=defaultdict(set)
    for r in index:
        if r['name'] not in accepted:continue
        desc=audit.describe(pe,r)
        for address in desc['calls']|desc['references']:
            if address in uncovered:xrefs[address].add(r['name'])
    report=dict(accepted_code_extent_bytes=len(restored),total_code_extent_bytes=len(code),uncovered_bytes=len(uncovered),
                groups=groups,recognized_padding_outside_functions=len(padding),unclassified_bytes=len(unknown),
                unclassified_addresses=[hex(a) for a in unknown],
                uncovered_incoming_edges={hex(a):sorted(names) for a,names in sorted(xrefs.items())},
                note='Native disassembly extent inventory, not branch coverage. No alias candidate or one-byte pending stub is counted as a function.')
    Path('uncovered-code.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Uncovered %d: functions/regions %d, recognized padding %d, unclassified %d.'%
          (len(uncovered),len(classified),len(padding),len(unknown)))
    for name,g in groups.items():print(name,g['bytes'])
if __name__=='__main__':main()
