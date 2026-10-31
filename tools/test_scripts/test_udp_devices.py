#!/usr/bin/env python3
"""Structural checks for the restricted UDP device service."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
k=(root/'kernel/kernel.c').read_text()
n=(root/'kernel/net_service_remote_devctl.c').read_text()
entry=(root/'kernel/net.c').read_text()
assert 'net_service_remote_devctl_poll();' in entry
l=(root/'kernel/lwipopts.h').read_text()
assert 'device_process_pending();' in n
assert 'udp_bind(device_pcb,IP_ANY_TYPE,2000)' in n
assert 'net_device_request(device_req,device_req_len' in n
assert 'int net_device_request(' in n
assert 'int net_device_request(' not in k
assert 'MEMP_NUM_UDP_PCB 3' in l
assert 'gpio_driver_ioctl(pin,enable ? OS_GPIO_OUTPUT : OS_GPIO_INPUT)' in n
assert 'gpio_driver_rw(0,pin,&value,1,1)' in n
assert 'gpio_driver_rw(0,pin,out,1,0)' in n
assert 'if(!pin_gpio(pin) || pin>=23u) return -1;' in n
assert 'pwm_parse_value(which,p,n,&x)' in k
assert 'int pwm_parse_value(' in (root/'kernel/pwm.c').read_text()
assert 'pwm_parse_value(which,req+payload,n-payload,&value)' in n
assert 'if(tasks[current].fd_pos[fd]==0)' in k
assert 'adc_driver_read(dev==DEV_ADC_END' in k
assert 'fb_ioctl(enable ? OS_FB_ENABLE : OS_FB_DISABLE)' in n
print('PASS: UDP device protocol and deferred lwIP handling')
