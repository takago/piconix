#!/usr/bin/env python3
"""Check that Tiny Python I2C calls use caller-owned file descriptors."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
src = (root / "userland/apps/python.c").read_text()
assert "static int transfer(int fd,int addr," in src
helper = src.split("static int transfer(int fd,int addr,", 1)[1].split("static uint32_t last_peer_ip", 1)[0]
assert 'open("/dev/i2c/1"' not in helper
assert "close(fd)" not in helper
assert "ioctl(fd,OS_I2C_TRANSFER," in helper
assert "transfer(fd,addr,payload,len,0,result)" in src
assert "transfer(fd,addr,values,len,r,result)" in src
for name in ("userland/examples/python/fbwave.py", "userland/fs/tmp_common/fbwave.py"):
    script = (root / name).read_text()
    assert 'i2c = open("/dev/i2c/1", 2)' in script
    assert "i2c_write(i2c," in script and "i2c_read(i2c," in script
    assert "close(i2c)" in script
    assert "i2c_try_" not in script and "data != -1" in script
assert "i2c_try_read" not in src and "i2c_try_write" not in src
assert "if(rc<0)return -1;" in src
print("PASS: caller-owned I2C descriptors in Python and BH1750 examples")
