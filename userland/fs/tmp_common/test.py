# Read the RP2040 on-chip temperature sensor five times.
sensor = open("/dev/adc/temp", 0)
if sensor < 0:
    print("temperature sensor unavailable")
    exit()
print("RP2040 temperature (C):")
for i in range(5):
    temp = read(sensor, 32)
    if temp == -1:
        print("temperature read failed")
        close(sensor)
        exit()
    write(1, temp)
    msleep(1000)
close(sensor)
