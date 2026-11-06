# Pico W UDP temperature server, port 9000.
# recvfrom() blocks until a datagram arrives.
server = socket(2)
if server < 0:
    print("UDP: socket failed")
    exit()

if bind(server, 9000) != 0:
    print("UDP: bind failed")
    close(server)
    exit()

print("UDP temperature server: 9000")
while True:
    request = recvfrom(server, 64)
    if request == -1:
        print("UDP: receive failed")
        break
    if len(request) == 0:
        continue

    sensor = open("/dev/adc/temp", 0)
    if sensor < 0:
        continue

    temp = read(sensor, 32)
    close(sensor)
    if temp == -1:
        continue

    sendto(server, temp, "peer", 0)

close(server)
