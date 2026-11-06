# PicoNIX: BH1750-controlled LED wave (fixed speed as fallback).
wave = [3,4,5,6,5,4,3,2]
fb = [0,0,0,0,0,0,0,0]
fd = open("/dev/gpio/fb", 2)
if fd < 0:
    print("fbwave: framebuffer unavailable")
    exit()
if ioctl(fd, FB_ENABLE, 0) < 0:
    print("fbwave: framebuffer setup failed")
    close(fd)
    exit()
i2c = open("/dev/i2c/1", 2)
ready = False
if i2c >= 0:
    if i2c_write(i2c, 0x23, bytes([1])) >= 0:
        msleep(100)
        if i2c_write(i2c, 0x23, bytes([0x10])) >= 0:
            ready = True
            msleep(200)
phase = 0
for n in range(10000):
    delay = 150
    if ready:
        data = i2c_read(i2c, 0x23, 2)
        if data != -1:
            lux = (data[0]*256+data[1])*5/6
            delay = 350 - lux/3
            if delay < 35:
                delay = 35
        else:
            ready = False
    for y in range(8):
        x = wave[(y+phase)%8]
        fb[y] = 1 << (7-x)
    write(fd, fb)
    phase = (phase+1)%8
    msleep(delay)
if i2c >= 0:
    if ready:
        off = i2c_write(i2c, 0x23, bytes([0]))
    close(i2c)
close(fd)
