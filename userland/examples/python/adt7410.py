# ADT7410 temperature sensor (I2C address 0x48).
# Default 13-bit mode: 1/16 C per LSB.
fd = open("/dev/i2c/1", 2)
if fd < 0:
    print("I2C open failed")
    exit()
reg = bytes([0])
for n in range(10000):
    data = i2c_xfer(fd, 0x48, reg, 2)
    if data == -1:
        print("ADT7410 read failed")
        break
    raw = data[0]*256 + data[1]
    if raw >= 32768:
        raw = raw - 65536
    temp10 = raw*10/128
    print("temperature x10 C:", temp10)
    msleep(250)
close(fd)
