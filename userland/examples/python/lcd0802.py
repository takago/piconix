# I2C 8x2 LCD at address 0x3e (controller-specific).
fd = open("/dev/i2c/1", 2)
if fd < 0:
    print("I2C open failed")
    exit()
# Control byte 0x00: commands; 0x40: display data.
rc = i2c_write(fd, 0x3e, bytes([0,0x38,0x39,0x14,0x70,0x56,0x6c]))
if rc < 0:
    print("LCD init failed")
    close(fd)
    exit()
msleep(200)
rc = i2c_write(fd, 0x3e, bytes([0,0x38,0x0d,0x01]))
if rc < 0:
    print("LCD command failed")
    close(fd)
    exit()
msleep(2)
rc = i2c_write(fd, 0x3e, bytes([0x40,72,101,108,108,111]))
if rc < 0:
    print("LCD line 1 failed")
    close(fd)
    exit()
msleep(1)
rc = i2c_write(fd, 0x3e, bytes([0,0xc0]))
if rc < 0:
    print("LCD cursor failed")
    close(fd)
    exit()
msleep(2)
rc = i2c_write(fd, 0x3e, bytes([0x40,40,111,95,111,41]))
if rc < 0:
    print("LCD line 2 failed")
close(fd)
