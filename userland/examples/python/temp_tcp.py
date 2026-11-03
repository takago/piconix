# Pico W TCP temperature server, port 9000.
# One temperature reading per connection.
server = socket(1)
if server >= 0:
    rc = listen(server, 9000)
    if rc == 0:
        print("TCP temperature server: 9000")
        while True:
            client = accept(server)
            if client >= 0:
                sensor = open("/dev/adc/temp", 0)
                if sensor >= 0:
                    temp = read(sensor, 32)
                    close(sensor)
                    send(client, temp)
                close(client)
            msleep(20)
    close(server)
