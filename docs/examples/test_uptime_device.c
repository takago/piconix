/* Host-side test for the tutorial's optional virtual-driver module. */
#include "uptime_device.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

int main(void) {
    uint32_t pos = 0;
    char result[40] = {0};
    size_t used = 0;
    int n;
    while ((n = uptime_device_read(result + used, 3u, &pos, 12345u)) > 0) {
        used += (size_t)n;
        assert(used < sizeof(result));
    }
    assert(n == 0);
    assert(!strcmp(result, "12.345 s\n"));
    assert(pos == strlen(result));
    assert(uptime_device_read(result, 0, &pos, 12345u) == 0);
    puts("PASS: uptime virtual device short reads and EOF");
    return 0;
}
