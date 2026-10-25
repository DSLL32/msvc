"""Original-only debugger RPCs: stopped reads/masks, entry traps and resume."""
import struct

from ipc import ProtocolError

CONTEXT_NAMES=("eip","eax","ecx","edx","ebx","esp","ebp","esi","edi",
               "eflags","cs","ss","ds","es","fs","gs")


def words(values):
    if any(not isinstance(value,int) or not 0<=value<=0xffffffff for value in values):
        raise ProtocolError("broker DWORD outside bounds")
    return struct.pack("<%dI"%len(values),*values)


class Broker:
    def __init__(self,client):
        self.client=client
        self.events=[]

    def fixed(self,operation,payload,size):
        response=self.client.request(operation,payload)
        if len(response)!=size:
            self.client.failed=True
            raise ProtocolError("broker response length mismatch")
        return response

    def event(self):
        packet=struct.unpack("<9I",self.fixed(10,b"",36))
        result=dict(zip(("code","pid","tid","base","entry","teb","exception",
                         "exception_address","first_chance"),packet))
        self.events.append(result)
        return result

    def context(self):
        return dict(zip(CONTEXT_NAMES,struct.unpack("<16I",self.fixed(11,b"",64))))

    def read(self,address,size):
        if not isinstance(size,int) or not 1<=size<=512 or address+size>0x100000000:
            raise ProtocolError("broker read outside bounds")
        return self.fixed(12,words((address,size)),size)

    def trap(self,address,expected):
        if not isinstance(expected,int) or not 0<=expected<=255:
            raise ProtocolError("broker expected opcode outside byte bounds")
        return struct.unpack("<I",self.fixed(13,words((address,expected)),4))[0]

    def consume(self,index):
        self.fixed(14,words((index,)),0)

    def loader(self):
        self.fixed(17,b"",0)

    def stop(self):
        self.fixed(18,b"",0)

    def loader_prefix(self):
        """One fixed loader prefix, ending before its CF launch CALL.

        Caller pins the original loader and verifies loaded code/data first.
        Ordinary setup binds the actual payload allocation before mask writes.
        """
        index=struct.unpack("<I",self.fixed(21,b"",4))[0]
        event=self.event()
        if (event["code"]!=1 or event["exception"]!=0x80000003 or
                not event["first_chance"] or
                event["exception_address"]!=event["base"]+0x10763):
            raise ProtocolError("missing stopped payload loader launch")
        actual=self.context()
        if actual["eip"]!=event["base"]+0x10764:
            raise ProtocolError("payload launch breakpoint/context mismatch")
        return index,event,actual

    def run_setup(self, base, size, entry):
        self.fixed(27, words((base, size, entry)), 0)

    def write(self, address, data):
        if not isinstance(data, bytes) or not 0 < len(data) <= 4088:
            raise ProtocolError('bounded payload write required')
        self.fixed(28, words((address, len(data))) + data, 0)

    def run(self):
        packet = struct.unpack('<25I', self.fixed(29, b'', 100))
        event = dict(zip(('code', 'pid', 'tid', 'exception', 'exception_address',
                          'first_chance'), packet[:6]))
        self.events.append(event)
        if event['code'] == 5:
            event['exit_code'] = packet[3]
            return event, None
        if event['code'] != 1:
            raise ProtocolError('unexpected ordinary debugger event')
        return event, dict(zip(CONTEXT_NAMES, packet[9:]))

    def region(self,address):
        packet=struct.unpack("<7I",self.fixed(20,words((address,)),28))
        result=dict(zip(("base","allocation_base","allocation_protect","size","state",
                         "protect","type"),packet))
        if (not result["size"] or result["base"]+result["size"]>0x100000000 or
                not result["base"]<=address<result["base"]+result["size"]):
            raise ProtocolError("invalid observed memory region")
        return result

    def entry(self,rva,expected):
        first=self.event()
        if first["code"]!=3 or first["entry"]!=first["base"]+rva:
            raise ProtocolError("unexpected initial process/entry event")
        index=self.trap(first["entry"],expected)
        for _ in range(100):
            self.loader();event=self.event()
            if event["pid"]!=first["pid"] or event["tid"]!=first["tid"]:
                raise ProtocolError("foreign startup process/thread")
            if event["code"]==1 and event["exception_address"]==first["entry"]:
                if event["exception"]!=0x80000003 or not event["first_chance"]:
                    raise ProtocolError("unexpected entry exception")
                before=self.context()
                if before["eip"]!=first["entry"]+1:
                    raise ProtocolError("entry trap context mismatch")
                self.consume(index);after=self.context()
                expected_context=dict(before);expected_context["eip"]=first["entry"]
                if after!=expected_context:
                    raise ProtocolError("owned trap rewind changed unrelated raw context")
                return first,after
        raise ProtocolError("startup event budget exhausted")
