# PicoNIX: light-controlled wave (fixed speed without BH1750)
wave = [3,4,5,6,5,4,3,2]
fb = [0,0,0,0,0,0,0,0]
fd = open("/dev/gpio/fb", 2)
if fd >= 0:
    ioctl(fd, FB_ENABLE, 0)
i2c = open("/dev/i2c/1", 2)
ready = 0
if i2c >= 0:
    ready = i2c_write(i2c, 0x23, bytes([1]))
    if ready >= 0:
        ready = 1
    if ready > 0:
        msleep(100)
        ready = i2c_write(i2c, 0x23, bytes([0x10]))
        if ready >= 0:
            ready = 1
        if ready > 0:
            msleep(200)

phase = 0
for n in range(10000):
    delay = 150
    if ready > 0:
        data = i2c_read(i2c, 0x23, 2)
        if data != -1:
            lux = (data[0]*256+data[1])*5/6
            delay = 350 - lux/3
            if delay < 35:
                delay = 35
        else:
            ready = 0
    for y in range(8):
        x = wave[(y+phase)%8]
        fb[y] = 1 << (7-x)
    if fd >= 0:
        write(fd, fb)
    phase = (phase+1)%8
    msleep(delay)

if i2c >= 0:
    if ready > 0:
        off = i2c_write(i2c, 0x23, bytes([0]))
    close(i2c)
if fd >= 0:
    close(fd)
