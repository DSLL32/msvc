"""Read-only typed state inspection inside the native debugger."""
import gdb
import hashlib
import json

def wide_bytes(pointer):
    result=bytearray()
    for offset in range(4177):
        word=bytes(gdb.selected_inferior().read_memory(pointer+2*offset,2))
        result.extend(word)
        if word==b'\0\0':return result.hex()
    raise AssertionError('unterminated preprocessor string')

def capture_preprocessor(path):
    specification=json.loads(path.read_text());state={}
    for row in specification['globals']:
        value=gdb.parse_and_eval(row['name']);address=int(value.address)
        record=dict(address=address,bytes=bytes(gdb.selected_inferior().read_memory(address,row['bytes'])).hex())
        if row['kind']=='wide-string':record['text']=wide_bytes(int(value))
        state[row['name']]=record
    tables={}
    for label in ['options','letters']:
        table=specification[label];records=[]
        for index in range(table['rows']):
            value=gdb.parse_and_eval(table['name']+'[%d]'%index)
            record=dict(destination=int(value['destination']))
            if label=='options':
                record.update(pattern=wide_bytes(int(value['pattern']))if int(value['pattern'])else None,
                    keep=int(value['keep_scanning']),action=int(value['action']),reserved=int(value['reserved']))
            else:record.update(letter=int(value['letter']),action=int(value['action']))
            records.append(record)
        tables[label]=dict(address=int(gdb.parse_and_eval('&'+table['name'])),rows=records)
    return dict(schema_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),globals=state,tables=tables)
