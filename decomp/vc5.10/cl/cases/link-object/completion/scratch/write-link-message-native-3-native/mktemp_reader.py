"""Strict one-invocation native mktemp observation, independent of output."""
import os,struct,tempfile

class MktempObservation:
    size=568
    def __init__(self,template,suffix,prestate):
        self.template=template;self.suffix=suffix;self.prestate=tuple(prestate)
        self.file=tempfile.TemporaryFile();self.file.write(bytes(self.size));self.file.flush()
    @property
    def fd(self):return self.file.fileno()
    def environment(self):
        return dict(RECON_MKTEMP_TEMPLATE=self.template,RECON_MKTEMP_SUFFIX=self.suffix,
                    RECON_MKTEMP_PRESTATE=','.join(str(x) for x in self.prestate),RECON_MKTEMP_FD=str(self.fd))
    def read(self,pid):
        self.file.seek(0);raw=self.file.read()
        if len(raw)!=self.size:raise ValueError('malformed mktemp receipt size')
        fields=struct.unpack('<6I2i4I260s260s',raw)
        magic,version,owner,count,argument,result,err0,err1,dos0,dos1,last0,last1,before,after=fields
        if (magic,version,owner,count)!=(0x31544d52,1,pid,1) or not argument or result!=argument:
            raise ValueError('missing/wrong-owner/duplicate mktemp observation or result alias')
        def text(value):
            prefix,zero,tail=value.partition(b'\0')
            if not zero or any(tail):raise ValueError('malformed mktemp string')
            return prefix.decode('ascii')
        before,after=text(before),text(after)
        if before!=self.template or not before.endswith('XXXXXX') or after!=before[:-6]+self.suffix:
            raise ValueError('mktemp template/result mismatch')
        if (err0,dos0,last0)!=self.prestate or (err1,dos1,last1)!=(err0,2,2):
            raise ValueError('mktemp error transition mismatch')
        return dict(count=count,argument=argument,result=result,before=before,after=after,
                    before_errno=err0,after_errno=err1,before_doserrno=dos0,after_doserrno=dos1,
                    before_last_error=last0,after_last_error=last1)
    def close(self):self.file.close()
    def __enter__(self):return self
    def __exit__(self,*unused):self.close()
