#!/usr/bin/env python3
"""Check the small Python interpreter's file/socket/I2C error conventions."""
from pathlib import Path
p=Path(__file__).resolve().parents[2]
s=(p/'userland/apps/python.c').read_text()
assert 'if(rc<0){setvar(name,-1);return;}' in s  # read, recv, recvfrom
assert 'if(rc>size){error=1;return;}' in s
assert 'seqnew(name,1,values,rc);return;' in s
assert 'if(is_udp&&rc>=0)' in s # no peer change on failed recvfrom
assert 'if(rc<0)setvar(name,-1);' in s # I2C read and xfer
# i2c_write is an integer-valued expression and forwards transfer's result directly.
assert 'return transfer(fd,addr,payload,len,0,result);' in s
for call in ('socket(AF_INET,type,0)', 'connect(fd,(const struct sockaddr *)&addr,sizeof addr)',
             'bind_udp(fd,(unsigned)port)','tcp_listen(fd,(unsigned)port)',
             'open(path,flags)', 'lseek(fd,offset,whence)'):
    assert f'int rc={call};return rc<0?-1:rc;' in s or f'int32_t rc=(int32_t){call};return rc<0?-1:rc;' in s,call
assert 'int rc=accepting?tcp_accept(fd):tcp_status(fd);return rc<0?-1:rc;' in s
assert 'return rc<0?-1:rc;' in s
assert 'i2c_try_' not in s
for name in ('temp_tcp.py','temp_udp.py'):
    d=(p/'userland/examples/python'/name).read_text()
    assert ('!= -1' in d or '== -1' in d), name
for lang in ('ja','en'):
    d=(p/f'docs/{lang}/development/apps/python/PYTHON.md').read_text()
    assert 'EAGAIN' in d and '-1' in d
print('PASS: Python I/O return conventions and sample guards')
