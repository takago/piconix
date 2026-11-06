# RPR-0521RS proximity / ambient light sensor (0x38).
fd = open("/dev/i2c/1", 2)
if fd < 0:
    print("I2C open failed")
    exit()
if i2c_write(fd, 0x38, bytes([0x40,0x80])) < 0:
    print("RPR init failed")
    close(fd)
    exit()
if i2c_write(fd, 0x38, bytes([0x41,0xc6])) < 0:
    print("RPR mode failed")
    close(fd)
    exit()
reg = bytes([0x44])
for n in range(10000):
    msleep(250)
    data = i2c_xfer(fd, 0x38, reg, 6)
    if data == -1:
        print("RPR read failed")
        break
    ps = (data[1]&15)*256 + data[0]
    als0 = data[3]*256 + data[2]
    als1 = data[5]*256 + data[4]
    print("PS ALS0 ALS1:", ps, als0, als1)
close(fd)
