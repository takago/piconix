# Pico W UDP temperature server, port 9000.
# Reply to the sender of each request.
server = socket(2)
if server >= 0:
    rc = bind(server, 9000)
    if rc == 0:
        print("UDP temperature server: 9000")
        while True:
            request = recvfrom(server, 64)
            if request != -1:
                if len(request) > 0:
                    sensor = open("/dev/adc/temp", 0)
                    if sensor >= 0:
                        temp = read(sensor, 32)
                        close(sensor)
                        if temp != -1:
                            sendto(server, temp, "peer", 0)
            msleep(20)
    close(server)
