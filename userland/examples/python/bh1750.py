# BH1750 illuminance sensor (I2C address 0x23).
fd = open("/dev/i2c/1", 2)
if fd < 0:
    print("I2C open failed")
    exit()
if i2c_write(fd, 0x23, bytes([1])) < 0:
    print("BH1750 power on failed")
    close(fd)
    exit()
if i2c_write(fd, 0x23, bytes([0x10])) < 0:
    print("BH1750 mode failed")
    close(fd)
    exit()
for n in range(10000):
    msleep(200)
    data = i2c_read(fd, 0x23, 2)
    if data == -1:
        print("BH1750 read failed")
        break
    lux = (data[0]*256 + data[1])*5/6
    print("lux:", lux)
i2c_write(fd, 0x23, bytes([0]))
close(fd)
