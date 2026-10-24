"""Decode the actually consumed CL scalar for the distinct Jobs family only."""
import re,struct

def first_scan(read,phase):
    assert read('mre-observer.txt')==b'CL_MRE_JOBS_RESPONSE_V1\n'
    assert re.fullmatch(r'(?:mre-[1-6]-(?:before|after(?:-copy-after)?)|(?:jobs|lower|compare)-(?:before|after))',phase)
    raw=read('observer/'+phase+'-first-dependency-scan.bin')
    assert len(raw)==4,'missing/incorrect scalar width'
    value=struct.unpack('<I',raw)[0]
    assert value in(0,1),'unrecognized first-dependency-scan state'
    return value

def close_commit(read,phase,arguments):
    assert phase=='mre-6-before'and len(arguments)==2
    assert type(arguments[1])is int and arguments[1]==first_scan(read,phase),'Close commit differs from real CL scalar'
