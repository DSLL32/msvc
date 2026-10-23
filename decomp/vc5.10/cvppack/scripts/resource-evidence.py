#!/usr/bin/env python3
"""Extract only the three encountered resource strings for original evidence."""
import json
from pathlib import Path
import re
import struct
from binary import PE, u16, u32
from state import BINARY, ROOT, sha


def resources_from(pe):
    directory_rva = u32(pe.data,pe.optional+112)
    root = pe.offset(pe.base+directory_rva)
    def child(directory,identifier):
        offset = root+directory
        count = u16(pe.data,offset+12)+u16(pe.data,offset+14)
        for i in range(count):
            name,value = struct.unpack_from('<II',pe.data,offset+16+i*8)
            if name==identifier:return value & 0x7fffffff
        raise AssertionError(('resource absent',directory,identifier))
    resources=[]
    for identifier in (8008,8009,1029):
        type_directory=child(0,6)
        language_directory=child(type_directory,identifier//16+1)
        entry=child(language_directory,0x409)
        rva,size,codepage,reserved=struct.unpack_from('<4I',pe.data,root+entry)
        pointer=pe.offset(pe.base+rva)
        for index in range(identifier%16):
            pointer += 2+2*u16(pe.data,pointer)
        length=u16(pe.data,pointer)
        raw=pe.data[pointer+2:pointer+2+length*2]
        text=raw.decode('utf-16le')
        resources.append(dict(id=identifier,language=0x409,type=6,block=identifier//16+1,
            record_file_offset=pointer,length_utf16=length,utf16le=raw.hex(),text=text,
            ansi_bytes=text.encode('ascii').hex(),nul_terminated_ansi=(text.encode('ascii')+b'\0').hex(),
            codepage=codepage))
    return resources


def main():
    target = ROOT/'toolchains/5.10/bin/CVPACK.EXE'
    resources=resources_from(PE(target))
    events=(BINARY/'cases/help/resource-provider-original/events').read_text().splitlines()
    calls=[]
    for i,line in enumerate(events):
        if not re.match(r'^(?:resource|getproc|loadlibrary)-(?:call|return) ',line):continue
        record=dict(event=line)
        j=i+1
        while j<len(events) and not re.match(r'^(?:resource|getproc|loadlibrary)-(?:call|return) |^exit ',events[j]):
            if events[j].startswith('last-error '):record['last_error']=int(events[j].split()[1]);break
            j+=1
        calls.append(record)
    result=dict(schema=1,target_sha256=sha(target),kind='encountered-only-resource-evidence',
        resources=resources,raw_api_capture='cases/help/resource-provider-original/events',
        raw_api_capture_sha256=sha(BINARY/'cases/help/resource-provider-original/events'),calls=calls,
        note='LastError is actual child NT i386 TEB.LastErrorValue at+0x34; TEB address came from CREATE_PROCESS_DEBUG_EVENT. This script is original-analysis only, not native runtime/resource loading.')
    (BINARY/'cases/help/resources.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__':main()
