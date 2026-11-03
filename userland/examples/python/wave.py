# PicoNIX: light-controlled wave
wave = [3,4,5,6,5,4,3,2]
fb = [0,0,0,0,0,0,0,0]
fd = open("/dev/gpio/fb", 2)
ioctl(fd, FB_ENABLE, 0)
i2c_write(0x23, bytes([1]))
msleep(100)
i2c_write(0x23, bytes([0x10]))
msleep(200)

phase = 0
for n in range(10000):
    data = i2c_read(0x23, 2)
    lux = (data[0]*256+data[1])*5/6
    delay = 350 - lux/3
    if delay < 35:
        delay = 35
    for y in range(8):
        x = wave[(y+phase)%8]
        fb[y] = 1 << (7-x)
    write(fd, fb)
    phase = (phase+1)%8
    msleep(delay)

i2c_write(0x23, bytes([0]))
close(fd)
