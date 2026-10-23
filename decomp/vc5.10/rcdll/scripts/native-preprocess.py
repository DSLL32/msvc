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
        if row['kind']=='address-identity':record['identity_address']=int(gdb.parse_and_eval('&'+row['identity']))
        if row['kind']=='wide-string':record['text']=wide_bytes(int(value))
        if row['kind']in ['crt-file','crt-buffered-file']and int(value):
            record['file_state']=bytes(gdb.selected_inferior().read_memory(int(value),32)).hex()
            file=value.dereference();base=int(file['_base']);size=int(file['_bufsiz'])
            if base:
                assert 0<size<=65536;record['file_buffer']=bytes(gdb.selected_inferior().read_memory(base,size)).hex()
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
    macros=[]
    for bucket in range(256):
        node=gdb.parse_and_eval('rc_macro_buckets[%d]'%bucket)
        while int(node):
            assert len(macros)<3,'unknown macro-table case'
            value=node.dereference();name=int(value['name']);text=int(value['text']);name_bytes=wide_bytes(name)
            macros.append(dict(bucket=bucket,pointer=int(node),next=int(value['next']),name=name,text=text,
                parameters=int(value['parameters']),flags=int(value['flags']),reserved=int(value['reserved']),
                record=bytes(gdb.selected_inferior().read_memory(int(node)-8,24)).hex(),
                name_block=bytes(gdb.selected_inferior().read_memory(name-8,len(bytes.fromhex(name_bytes))+10)).hex(),
                text_block=bytes(gdb.selected_inferior().read_memory(text-8,16)).hex()))
            node=value['next']
    return dict(macros=macros,schema_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),globals=state,tables=tables)
