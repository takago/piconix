# Pico W TCP temperature server, port 9000.
# accept() blocks until a client connects.
server = socket(1)
if server < 0:
    print("TCP: socket failed")
    exit()

if listen(server, 9000) != 0:
    print("TCP: listen failed")
    close(server)
    exit()

print("TCP temperature server: 9000")
while True:
    client = accept(server)
    if client < 0:
        print("TCP: accept failed")
        break

    sensor = open("/dev/adc/temp", 0)
    if sensor < 0:
        close(client)
        continue

    temp = read(sensor, 32)
    close(sensor)
    if temp != -1:
        send(client, temp)

    close(client)

close(server)
